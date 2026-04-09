/**
 * syslog_receiver.cpp
 *
 * Cross-platform UDP/TCP syslog listener (RFC 3164 + RFC 5424).
 * Windows  → Winsock2
 * Linux/macOS → POSIX sockets
 *
 * Design:
 *   udp_listener()  – recvfrom loop, select()-based stop check
 *   tcp_listener()  – accept loop, per-connection handler thread
 *   parse_syslog()  – extracts host/sourcetype from the syslog envelope
 *   Events are pushed directly into Pipeline::ingest() in batches of 1.
 */

#include "ingest/syslog_receiver.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

// ─── Platform socket helpers ───────────────────────────────────────────────────
// socket_t is already typedef'd in syslog_receiver.h (via the platform block
// that also pulls in winsock2.h / POSIX headers).  Here we only need the
// small inline helpers that don't belong in a public header.

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   using socklen_t = int;           // not defined by winsock2 by default
   static inline int  close_socket(SOCKET s)    { return closesocket(s); }
   static inline bool socket_would_block() {
       int e = WSAGetLastError();
       return e == WSAEWOULDBLOCK || e == WSAEINTR;
   }
#  define INVALID_SOCK  INVALID_SOCKET
#  define SOCK_ERR      SOCKET_ERROR
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <cerrno>
   static inline int  close_socket(int s)        { return ::close(s); }
   static inline bool socket_would_block()        { return errno == EAGAIN || errno == EINTR; }
#  define INVALID_SOCK  (-1)
#  define SOCK_ERR      (-1)
#endif

namespace sentinel::ingest {

// ─── Winsock lifetime (no-op on non-Windows) ─────────────────────────────────
namespace {
struct WinsockGuard {
    WinsockGuard() {
#ifdef _WIN32
        WSADATA wd;
        int rc = WSAStartup(MAKEWORD(2, 2), &wd);
        if (rc != 0) {
            spdlog::error("WSAStartup failed: {}", rc);
        }
#endif
    }
    ~WinsockGuard() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

// singleton guard – lives for the duration of the process
WinsockGuard& winsock_guard() {
    static WinsockGuard g;
    return g;
}

// Set socket to non-blocking mode
bool set_nonblocking(socket_t sock) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

// Set SO_REUSEADDR
bool set_reuseaddr(socket_t sock) {
    int opt = 1;
    return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                      reinterpret_cast<const char*>(&opt), sizeof(opt)) == 0;
}

// Wait up to `timeout_ms` for data to be available on `sock`.
// Returns true if data is ready (or connection arrived), false on timeout.
bool wait_readable(socket_t sock, int timeout_ms = 500) {
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(sock, &rset);
    timeval tv{ timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
    int rc = select(static_cast<int>(sock) + 1, &rset, nullptr, nullptr, &tv);
    return rc > 0;
}
} // anonymous namespace

// ─── Constructor / destructor ─────────────────────────────────────────────────

SyslogReceiver::SyslogReceiver(Pipeline& pipeline,
                               uint16_t udp_port,
                               uint16_t tcp_port)
    : pipeline_(pipeline),
      udp_port_(udp_port),
      tcp_port_(tcp_port)
{
    winsock_guard(); // ensure WSAStartup called
}

SyslogReceiver::~SyslogReceiver() {
    stop();
}

// ─── Public API ───────────────────────────────────────────────────────────────

void SyslogReceiver::start() {
    running_ = true;
    spdlog::info("Syslog receiver starting on UDP:{} TCP:{}", udp_port_, tcp_port_);

    udp_thread_ = std::jthread([this](std::stop_token st) { udp_listener(st); });
    tcp_thread_ = std::jthread([this](std::stop_token st) { tcp_listener(st); });

    spdlog::info("Syslog receiver started");
}

void SyslogReceiver::stop() {
    if (!running_.exchange(false)) return; // already stopped

    if (udp_thread_.joinable()) {
        udp_thread_.request_stop();
        udp_thread_.join();
    }
    if (tcp_thread_.joinable()) {
        tcp_thread_.request_stop();
        tcp_thread_.join();
    }
    spdlog::info("Syslog receiver stopped. Messages received: {}",
                 messages_received_.load());
}

// ─── Syslog parsing ───────────────────────────────────────────────────────────
//
// Supports both RFC 3164 and RFC 5424 (best-effort):
//
//   RFC 3164:  <PRI>Mmm DD HH:MM:SS HOSTNAME TAG: MSG
//   RFC 5424:  <PRI>VERSION TIMESTAMP HOSTNAME APP-NAME PROCID MSGID SD MSG
//
// We care about: host, maybe facility/severity → sourcetype.

IngestEvent SyslogReceiver::parse_syslog(const std::string& message) {
    IngestEvent event;
    event.raw       = message;
    event.sourcetype = "syslog";
    event.source    = "syslog";
    event.index     = "main";

    if (message.empty()) return event;

    size_t pos = 0;
    int priority = -1;

    // ── Extract <PRI> ──────────────────────────────────────────────────────
    if (message[0] == '<') {
        size_t pri_end = message.find('>');
        if (pri_end != std::string::npos && pri_end < 6) {
            try {
                priority = std::stoi(message.substr(1, pri_end - 1));
            } catch (...) {}
            pos = pri_end + 1;
        }
    }

    if (priority >= 0) {
        int facility = priority >> 3;
        int severity = priority & 0x07;
        // Map facility → sourcetype hint
        const char* fac_names[] = {
            "kern","user","mail","daemon","auth","syslog","lpr","news",
            "uucp","cron","authpriv","ftp","ntp","audit","alert","clock",
            "local0","local1","local2","local3","local4","local5","local6","local7"
        };
        if (facility >= 0 && facility < 24) {
            event.sourcetype = std::string("syslog:") + fac_names[facility];
        }
        (void)severity; // could set level field
    }

    // ── RFC 5424 check: next char is a digit (VERSION) ────────────────────
    bool is_5424 = (pos < message.size() && std::isdigit(static_cast<unsigned char>(message[pos])));

    if (is_5424) {
        // <PRI>1 TIMESTAMP HOSTNAME APP-NAME PROCID MSGID SD MSG
        // skip VERSION
        pos = message.find(' ', pos);
        if (pos == std::string::npos) return event;
        ++pos;

        // TIMESTAMP
        size_t ts_end = message.find(' ', pos);
        if (ts_end == std::string::npos) return event;
        pos = ts_end + 1;

        // HOSTNAME
        size_t host_end = message.find(' ', pos);
        if (host_end != std::string::npos) {
            event.host = message.substr(pos, host_end - pos);
            if (event.host == "-") event.host = "unknown";
            pos = host_end + 1;
        }

        // APP-NAME (use as process/source hint)
        size_t app_end = message.find(' ', pos);
        if (app_end != std::string::npos) {
            std::string app = message.substr(pos, app_end - pos);
            if (app != "-") event.source = app;
        }

    } else {
        // RFC 3164: Mmm DD HH:MM:SS HOSTNAME TAG: MSG
        // Three tokens = timestamp, then HOSTNAME
        int spaces = 0;
        size_t hostname_start = pos;
        while (pos < message.size() && spaces < 3) {
            if (message[pos] == ' ') {
                ++spaces;
                if (spaces == 3) hostname_start = pos + 1;
            }
            ++pos;
        }

        // Hostname ends at next space
        size_t host_end = message.find(' ', hostname_start);
        if (host_end != std::string::npos) {
            event.host = message.substr(hostname_start, host_end - hostname_start);
            // Extract TAG (word before ':')
            size_t tag_start = host_end + 1;
            size_t colon = message.find(':', tag_start);
            if (colon != std::string::npos) {
                std::string tag = message.substr(tag_start, colon - tag_start);
                // strip PID like sshd[1234]
                size_t bracket = tag.find('[');
                if (bracket != std::string::npos) tag = tag.substr(0, bracket);
                if (!tag.empty()) event.source = tag;
            }
        }
    }

    messages_received_.fetch_add(1);
    return event;
}

// ─── UDP listener ─────────────────────────────────────────────────────────────

void SyslogReceiver::udp_listener(std::stop_token st) {
    socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCK) {
        spdlog::error("Syslog UDP: failed to create socket");
        return;
    }

    set_reuseaddr(sock);
    set_nonblocking(sock);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(udp_port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCK_ERR) {
        spdlog::error("Syslog UDP: bind on port {} failed", udp_port_);
        close_socket(sock);
        return;
    }

    spdlog::info("Syslog UDP listener bound to 0.0.0.0:{}", udp_port_);

    constexpr size_t BUF_SIZE = 65536;
    std::vector<char> buf(BUF_SIZE);

    while (!st.stop_requested()) {
        if (!wait_readable(sock, 500)) continue; // check stop every 500 ms

        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);
        int n = recvfrom(sock,
                         buf.data(),
                         static_cast<int>(buf.size() - 1),
                         0,
                         reinterpret_cast<sockaddr*>(&sender),
                         &sender_len);
        if (n <= 0) {
            if (!socket_would_block()) {
                spdlog::warn("Syslog UDP: recvfrom error");
            }
            continue;
        }

        buf[n] = '\0';
        std::string message(buf.data(), static_cast<size_t>(n));

        // Strip trailing \r\n
        while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
            message.pop_back();
        }
        if (message.empty()) continue;

        IngestEvent ev = parse_syslog(message);
        // If no host extracted, use sender IP
        if (ev.host.empty() || ev.host == "unknown") {
            char sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sender.sin_addr, sender_ip, sizeof(sender_ip));
            ev.host = sender_ip;
        }

        pipeline_.ingest({ev});
    }

    close_socket(sock);
    spdlog::info("Syslog UDP listener stopped");
}

// ─── TCP listener ─────────────────────────────────────────────────────────────
//
// Each accepted connection is handled in a short-lived detached thread.
// The connection thread reads until EOF or error, splits on newlines (framing).

void SyslogReceiver::tcp_listener(std::stop_token st) {
    socket_t listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCK) {
        spdlog::error("Syslog TCP: failed to create socket");
        return;
    }

    set_reuseaddr(listen_sock);
    set_nonblocking(listen_sock);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(tcp_port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCK_ERR) {
        spdlog::error("Syslog TCP: bind on port {} failed", tcp_port_);
        close_socket(listen_sock);
        return;
    }

    if (listen(listen_sock, SOMAXCONN) == SOCK_ERR) {
        spdlog::error("Syslog TCP: listen failed");
        close_socket(listen_sock);
        return;
    }

    spdlog::info("Syslog TCP listener bound to 0.0.0.0:{}", tcp_port_);

    // Track active connection threads so we can join them on stop
    std::vector<std::thread> conn_threads;

    while (!st.stop_requested()) {
        if (!wait_readable(listen_sock, 500)) continue;

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        socket_t client_sock = accept(listen_sock,
                                      reinterpret_cast<sockaddr*>(&client_addr),
                                      &client_len);
        if (client_sock == INVALID_SOCK) {
            if (!socket_would_block()) {
                spdlog::warn("Syslog TCP: accept error");
            }
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        std::string peer_ip(client_ip);

        spdlog::debug("Syslog TCP: accepted connection from {}", peer_ip);

        // Handle connection in a short-lived thread
        conn_threads.emplace_back([this, client_sock, peer_ip]() {
            handle_tcp_connection(client_sock, peer_ip);
        });

        // Periodically reap finished threads
        conn_threads.erase(
            std::remove_if(conn_threads.begin(), conn_threads.end(),
                [](std::thread& t) {
                    if (t.joinable()) {
                        // try_join via native_handle is platform-specific;
                        // instead we join them all on stop, here just detach finished ones
                        t.detach();
                        return true;
                    }
                    return false;
                }),
            conn_threads.end());
    }

    close_socket(listen_sock);

    // Join any remaining threads (best-effort; they'll stop when socket closed)
    for (auto& t : conn_threads) {
        if (t.joinable()) t.join();
    }

    spdlog::info("Syslog TCP listener stopped");
}

void SyslogReceiver::handle_tcp_connection(socket_t sock, const std::string& peer_ip) {
    // Set a receive timeout of 30 s so we don't block forever
#ifdef _WIN32
    DWORD timeout_ms = 30000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
#else
    timeval tv{ 30, 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    constexpr size_t BUF_SIZE = 8192;
    std::vector<char> buf(BUF_SIZE);
    std::string remainder;

    while (true) {
        int n = recv(sock, buf.data(), static_cast<int>(buf.size()), 0);
        if (n <= 0) break;   // EOF or error / timeout

        remainder.append(buf.data(), static_cast<size_t>(n));

        // Split on newlines — each line is one syslog message
        size_t start = 0;
        while (true) {
            size_t nl = remainder.find('\n', start);
            if (nl == std::string::npos) break;

            std::string line = remainder.substr(start, nl - start);
            // Strip \r
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (!line.empty()) {
                IngestEvent ev = parse_syslog(line);
                if (ev.host.empty() || ev.host == "unknown") {
                    ev.host = peer_ip;
                }
                pipeline_.ingest({ev});
            }

            start = nl + 1;
        }

        // Keep unprocessed tail for next recv
        remainder = remainder.substr(start);

        // Protect against unbounded memory if no newlines
        if (remainder.size() > 1024 * 1024) {
            spdlog::warn("Syslog TCP: oversized message from {}, discarding", peer_ip);
            remainder.clear();
        }
    }

    close_socket(sock);
    spdlog::debug("Syslog TCP: connection from {} closed", peer_ip);
}

} // namespace sentinel::ingest
