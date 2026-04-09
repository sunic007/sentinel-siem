#include "common/thread_pool.h"
#include <spdlog/spdlog.h>

namespace sentinel::common {

ThreadPool::ThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this](std::stop_token stop_token) {
            while (!stop_token.stop_requested()) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cv_.wait(lock, [this, &stop_token] {
                        return stopped_ || !tasks_.empty() || stop_token.stop_requested();
                    });

                    if ((stopped_ || stop_token.stop_requested()) && tasks_.empty()) {
                        return;
                    }

                    if (tasks_.empty()) continue;

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
    spdlog::debug("ThreadPool created with {} threads", num_threads);
}

ThreadPool::~ThreadPool() {
    shutdown();
}

size_t ThreadPool::pending() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_) return;
        stopped_ = true;
    }
    cv_.notify_all();

    for (auto& worker : workers_) {
        worker.request_stop();
        if (worker.joinable()) {
            worker.join();
        }
    }
    spdlog::debug("ThreadPool shut down");
}

} // namespace sentinel::common
