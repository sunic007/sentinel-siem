#pragma once

#include "common/config.h"
#include <string>
#include <thread>
#include <atomic>
#include <memory>

namespace sentinel::http {

class HttpServer {
public:
    explicit HttpServer(uint16_t port = 8080);
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    void start();
    void stop();
    bool is_running() const { return running_.load(); }

private:
    uint16_t port_;
    std::atomic<bool> running_{false};
    std::jthread server_thread_;

    struct Impl;
    std::unique_ptr<Impl> impl_;

    void setup_routes();
    void run();

    // Route handlers
    void handle_search(const std::string& body, std::string& response);
    void handle_ingest(const std::string& body, std::string& response);
    void handle_indexes(std::string& response);
    void handle_health(std::string& response);
};

} // namespace sentinel::http
