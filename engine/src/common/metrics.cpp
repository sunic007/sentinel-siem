#include "common/metrics.h"
#include <sstream>

namespace sentinel::common {

Metrics& Metrics::instance() {
    static Metrics instance;
    return instance;
}

void Metrics::increment(const std::string& name, int64_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_[name].fetch_add(value, std::memory_order_relaxed);
}

void Metrics::set_gauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    gauges_[name].store(value, std::memory_order_relaxed);
}

int64_t Metrics::get_counter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = counters_.find(name);
    if (it == counters_.end()) return 0;
    return it->second.load(std::memory_order_relaxed);
}

double Metrics::get_gauge(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gauges_.find(name);
    if (it == gauges_.end()) return 0.0;
    return it->second.load(std::memory_order_relaxed);
}

std::string Metrics::to_prometheus() const {
    std::ostringstream oss;
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [name, value] : counters_) {
        oss << "# TYPE " << name << " counter\n";
        oss << name << " " << value.load(std::memory_order_relaxed) << "\n";
    }

    for (const auto& [name, value] : gauges_) {
        oss << "# TYPE " << name << " gauge\n";
        oss << name << " " << value.load(std::memory_order_relaxed) << "\n";
    }

    return oss.str();
}

} // namespace sentinel::common
