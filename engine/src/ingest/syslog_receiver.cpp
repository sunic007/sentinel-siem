#include "ingest/syslog_receiver.h"
#include <spdlog/spdlog.h>

namespace sentinel::ingest {

SyslogReceiver::SyslogReceiver(Pipeline& pipeline, uint16_t udp_port, uint16_t tcp_port)
    : pipeline_(pipeline), udp_port_(udp_port), tcp_port_(tcp_port) {}

SyslogReceiver::~SyslogReceiver() {
    stop();
}

void SyslogReceiver::start() {
    running_ = true;
    spdlog::info("Syslog receiver starting on UDP:{} TCP:{}", udp_port_, tcp_port_);

    // TODO: Implement actual UDP/TCP socket listeners
    // For now, this is a placeholder for the socket implementation
    spdlog::info("Syslog receiver started");
}

void SyslogReceiver::stop() {
    running_ = false;
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

IngestEvent SyslogReceiver::parse_syslog(const std::string& message) {
    IngestEvent event;
    event.raw = message;
    event.sourcetype = "syslog";
    event.index = "main";

    // Basic RFC 3164 parsing
    // Format: <PRI>TIMESTAMP HOSTNAME MSG
    size_t pos = 0;
    if (!message.empty() && message[0] == '<') {
        pos = message.find('>');
        if (pos != std::string::npos) {
            ++pos;
        }
    }

    // Skip timestamp (first ~15 chars after PRI)
    // Try to extract hostname
    size_t space1 = message.find(' ', pos);
    if (space1 != std::string::npos) {
        size_t space2 = message.find(' ', space1 + 1);
        if (space2 != std::string::npos) {
            event.host = message.substr(space1 + 1, space2 - space1 - 1);
        }
    }

    messages_received_.fetch_add(1);
    return event;
}

void SyslogReceiver::udp_listener() {
    // TODO: UDP socket implementation using platform APIs
}

void SyslogReceiver::tcp_listener() {
    // TODO: TCP socket implementation using platform APIs
}

} // namespace sentinel::ingest
