#pragma once

#include "ingest/pipeline.h"
#include <string>
#include <atomic>
#include <thread>
#include <cstdint>

namespace sentinel::ingest {

class SyslogReceiver {
public:
    SyslogReceiver(Pipeline& pipeline, uint16_t udp_port = 514, uint16_t tcp_port = 514);
    ~SyslogReceiver();

    void start();
    void stop();

    int64_t messages_received() const { return messages_received_.load(); }

private:
    Pipeline& pipeline_;
    uint16_t udp_port_;
    uint16_t tcp_port_;
    std::atomic<bool> running_{false};
    std::atomic<int64_t> messages_received_{0};
    std::jthread udp_thread_;
    std::jthread tcp_thread_;

    void udp_listener();
    void tcp_listener();
    IngestEvent parse_syslog(const std::string& message);
};

} // namespace sentinel::ingest
