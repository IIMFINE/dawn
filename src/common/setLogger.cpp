#include "setLogger.h"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <sstream>
#include <vector>

#ifdef USE_SPDLOG
/// @brief avoid multiple definition compile error.

namespace dawn
{
constexpr const char *kSpdlogFmt = "[%H%M%S:%F][%L][%t][%s:%#] %v";

logManager_t::logManager_t() { init_spdlog(); }

logManager_t::~logManager_t()
{
    spd_logger_->flush();
    spdlog::shutdown();
}

void logManager_t::init_spdlog(std::string_view prefix_filename)
{
    char timestamp[100] = {0};
    std::stringstream filename;
    std::stringstream logger_name;
    auto time_info = logManager_t::return_timestamp();
    strftime(timestamp, sizeof(timestamp), "_%y%m%d_%H_", time_info);
    if (prefix_filename.empty())
    {
        filename << timestamp << ".log";
        logger_name << "dawn_logger";
    }
    else
    {
        filename << prefix_filename << timestamp << ".log";
        logger_name << prefix_filename << "dawn_logger";
    }

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    console_sink->set_pattern(kSpdlogFmt);

    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename.str(), 1024 * 1024 * 10, 3);

    std::vector<spdlog::sink_ptr> sinks{console_sink, rotating_sink};
    spd_logger_ = std::make_shared<spdlog::logger>(logger_name.str(), sinks.begin(), sinks.end());

    spdlog::set_default_logger(spd_logger_);
    spd_logger_->flush_on(spdlog::level::debug);
    spd_logger_->info(
        "format [second:nanosecond][log level][thread id][source file:line] "
        "content");
}

logManager_t &logManager_t::return_log_manager()
{
    static logManager_t logger;
    return logger;
}

std::shared_ptr<spdlog::logger> logManager_t::return_logger() { return logManager_t::return_log_manager().spd_logger_; }

struct tm *logManager_t::return_timestamp()
{
    time_t raw_time;
    struct tm *time_info;
    time(&raw_time);
    time_info = localtime(&raw_time);
    return time_info;
}
};  // namespace dawn

#endif
