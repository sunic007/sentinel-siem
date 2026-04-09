#include "common/config.h"
#include "common/logging.h"
#include "indexer/segment.h"
#include "ingest/pipeline.h"
#include "search/executor.h"
#include "http/server.h"
#include "spl/parser.h"

#include <spdlog/spdlog.h>
#include <csignal>
#include <atomic>

namespace {
    std::atomic<bool> g_running{true};

    void signal_handler(int signal) {
        spdlog::info("Received signal {}, shutting down...", signal);
        g_running = false;
    }
}

int main(int argc, char* argv[]) {
    // Setup signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Initialize logging
    sentinel::common::init_logging("sentinel-engine");
    spdlog::info("Sentinel SIEM Engine v0.1.0 starting...");

    // Load configuration
    sentinel::common::Config config;
    if (argc > 1) {
        config = sentinel::common::Config::from_file(argv[1]);
    } else {
        config = sentinel::common::Config::defaults();
    }

    spdlog::info("Data directory: {}", config.data_dir());
    spdlog::info("HTTP API port: {}", config.http_port());

    // Initialize components
    auto& ingest_pipeline = sentinel::ingest::Pipeline::instance();
    ingest_pipeline.configure(config);

    auto& search_executor = sentinel::search::Executor::instance();
    search_executor.configure(config);

    // Start ingest pipeline
    ingest_pipeline.start();
    spdlog::info("Ingest pipeline started");

    // Start HTTP API server
    sentinel::http::HttpServer http_server(config.http_port());
    http_server.start();

    // Main loop
    spdlog::info("Sentinel Engine is ready. Accepting connections.");
    spdlog::info("  HTTP API: http://0.0.0.0:{}", config.http_port());
    spdlog::info("  POST /api/search   - Execute SPL queries");
    spdlog::info("  POST /api/ingest   - Ingest events");
    spdlog::info("  GET  /api/indexes  - List indexes");
    spdlog::info("  GET  /api/health   - Health check");

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Graceful shutdown
    spdlog::info("Shutting down...");
    http_server.stop();
    ingest_pipeline.stop();
    spdlog::info("Sentinel Engine stopped");

    return 0;
}
