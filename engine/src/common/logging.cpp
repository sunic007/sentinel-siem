#include "common/logging.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace sentinel::common {

void init_logging(const std::string& logger_name, const std::string& log_level) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/" + logger_name + ".log",
        10 * 1024 * 1024,  // 10MB max size
        5                   // 5 rotated files
    );
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s:%#] %v");

    auto logger = std::make_shared<spdlog::logger>(
        logger_name,
        spdlog::sinks_init_list{console_sink, file_sink}
    );

    logger->set_level(spdlog::level::from_str(log_level));
    logger->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(logger);
    spdlog::info("Logging initialized: level={}", log_level);
}

} // namespace sentinel::common
