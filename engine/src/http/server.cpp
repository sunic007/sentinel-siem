#include "http/server.h"
#include "search/executor.h"
#include "ingest/pipeline.h"
#include "common/metrics.h"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <chrono>

namespace sentinel::http {

struct HttpServer::Impl {
    httplib::Server svr;
};

HttpServer::HttpServer(uint16_t port)
    : port_(port), impl_(std::make_unique<Impl>()) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    setup_routes();
    running_ = true;
    server_thread_ = std::jthread([this](std::stop_token) {
        run();
    });
    spdlog::info("HTTP API server starting on port {}", port_);
}

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        impl_->svr.stop();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        spdlog::info("HTTP API server stopped");
    }
}

void HttpServer::run() {
    impl_->svr.listen("0.0.0.0", port_);
}

void HttpServer::setup_routes() {
    auto& svr = impl_->svr;

    // CORS headers for all responses
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
    });

    // OPTIONS preflight
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // POST /api/search — Execute SPL query
    svr.Post("/api/search", [this](const httplib::Request& req, httplib::Response& res) {
        std::string response;
        handle_search(req.body, response);
        res.set_content(response, "application/json");
    });

    // POST /api/ingest — Ingest events
    svr.Post("/api/ingest", [this](const httplib::Request& req, httplib::Response& res) {
        std::string response;
        handle_ingest(req.body, response);
        res.set_content(response, "application/json");
    });

    // GET /api/indexes — List indexes
    svr.Get("/api/indexes", [this](const httplib::Request&, httplib::Response& res) {
        std::string response;
        handle_indexes(response);
        res.set_content(response, "application/json");
    });

    // GET /api/health — Health check
    svr.Get("/api/health", [this](const httplib::Request&, httplib::Response& res) {
        std::string response;
        handle_health(response);
        res.set_content(response, "application/json");
    });
}

void HttpServer::handle_search(const std::string& body, std::string& response) {
    try {
        auto json = nlohmann::json::parse(body);
        std::string query = json.value("query", "");

        if (query.empty()) {
            response = R"({"error":"query is required"})";
            return;
        }

        spdlog::info("HTTP Search: {}", query);

        auto& executor = search::Executor::instance();
        auto result = executor.execute(query);

        nlohmann::json resp;
        resp["total_count"] = result.total_matched;
        resp["execution_time_ms"] = result.execution_time_ms;
        resp["columns"] = result.columns;

        resp["events"] = nlohmann::json::array();
        for (const auto& row : result.rows) {
            nlohmann::json event;
            for (const auto& [key, value] : row) {
                event[key] = value;
            }
            resp["events"].push_back(event);
        }

        // Build stats if present (for stats/timechart commands)
        if (!result.rows.empty() && result.columns.size() > 0) {
            resp["stats"]["rows"] = nlohmann::json::array();
            resp["stats"]["columns"] = result.columns;
            for (const auto& row : result.rows) {
                nlohmann::json stat_row = nlohmann::json::array();
                for (const auto& col : result.columns) {
                    auto it = row.find(col);
                    stat_row.push_back(it != row.end() ? it->second : "");
                }
                resp["stats"]["rows"].push_back(stat_row);
            }
        }

        response = resp.dump();
        common::Metrics::instance().increment("sentinel_search_total");

    } catch (const std::exception& e) {
        spdlog::error("Search error: {}", e.what());
        response = nlohmann::json({{"error", e.what()}}).dump();
    }
}

void HttpServer::handle_ingest(const std::string& body, std::string& response) {
    try {
        auto json = nlohmann::json::parse(body);
        std::string index = json.value("index", "main");
        std::string sourcetype = json.value("sourcetype", "json");
        std::string host = json.value("host", "unknown");
        std::string source = json.value("source", "http");

        auto& events_json = json["events"];
        std::vector<ingest::IngestEvent> events;

        for (const auto& ej : events_json) {
            ingest::IngestEvent event;
            event.index = index;
            event.sourcetype = ej.value("sourcetype", sourcetype);
            event.host = ej.value("host", host);
            event.source = ej.value("source", source);
            event.raw = ej.value("raw", ej.dump());
            event.timestamp_us = ej.value("timestamp_us", 0ULL);
            events.push_back(std::move(event));
        }

        auto& pipeline = ingest::Pipeline::instance();
        int64_t accepted = pipeline.ingest(events);

        response = nlohmann::json({
            {"accepted", accepted},
            {"rejected", static_cast<int64_t>(events.size()) - accepted},
            {"total_events", pipeline.total_events()},
        }).dump();

        common::Metrics::instance().increment("sentinel_ingest_total", accepted);

    } catch (const std::exception& e) {
        spdlog::error("Ingest error: {}", e.what());
        response = nlohmann::json({{"error", e.what()}}).dump();
    }
}

void HttpServer::handle_indexes(std::string& response) {
    auto& executor = search::Executor::instance();
    auto indexes = executor.list_indexes();

    nlohmann::json resp = nlohmann::json::array();
    for (const auto& idx : indexes) {
        resp.push_back({
            {"name", idx.name},
            {"total_events", idx.total_events},
            {"total_size_bytes", idx.total_size_bytes},
            {"segment_count", idx.segment_count},
        });
    }

    response = resp.dump();
}

void HttpServer::handle_health(std::string& response) {
    auto& pipeline = ingest::Pipeline::instance();
    auto& metrics = common::Metrics::instance();

    response = nlohmann::json({
        {"status", "ok"},
        {"version", "0.1.0"},
        {"total_events_ingested", pipeline.total_events()},
        {"searches_executed", metrics.get_counter("sentinel_search_total")},
        {"events_ingested_total", metrics.get_counter("sentinel_ingest_total")},
    }).dump();
}

} // namespace sentinel::http
