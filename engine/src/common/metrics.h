#pragma once

#include <string>
#include <atomic>
#include <unordered_map>
#include <mutex>

namespace sentinel::common {

class Metrics {
public:
    static Metrics& instance();

    void increment(const std::string& name, int64_t value = 1);
    void set_gauge(const std::string& name, double value);
    void observe_histogram(const std::string& name, double value);

    int64_t get_counter(const std::string& name) const;
    double get_gauge(const std::string& name) const;

    // Serialize metrics in Prometheus exposition format
    std::string to_prometheus() const;

private:
    Metrics() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::atomic<int64_t>> counters_;
    std::unordered_map<std::string, std::atomic<double>> gauges_;
};

} // namespace sentinel::common
