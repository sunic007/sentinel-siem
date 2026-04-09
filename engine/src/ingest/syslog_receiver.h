#pragma once

#include "ingest/pipeline.h"
#include <string>
#include <atomic>
#include <thread>
#include <cstdint>

// Platform socket type forward-declared to avoid pulling in Winsock headers here
#ifdef _WIN32
#  include <winsock2.h>
   using socket_t = SOCKET;
#else
   using socket_t = int;
#endif

namespace sentinel::ingest {

class SyslogReceiver {
public:
    SyslogReceiver(Pipeline& pipeline,
                   uint16_t udp_port = 514,
                   uint16_t tcp_port = 514);
    ~SyslogReceiver();

    void start();
    void stop();

    int64_t messages_received() const { return messages_received_.load(); }

private:
    Pipeline&  pipeline_;
    uint16_t   udp_port_;
    uint16_t   tcp_port_;

    std::atomic<bool>    running_{false};
    std::atomic<int64_t> messages_received_{0};

    std::jthread udp_thread_;
    std::jthread tcp_thread_;

    // Thread entry points
    void udp_listener(std::stop_token st);
    void tcp_listener(std::stop_token st);

    // Handle a single accepted TCP connection (runs in a detached thread)
    void handle_tcp_connection(socket_t sock, const std::string& peer_ip);

    // Syslog message parser (RFC 3164 + RFC 5424 best-effort)
    IngestEvent parse_syslog(const std::string& message);
};

} // namespace sentinel::ingest
