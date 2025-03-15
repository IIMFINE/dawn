#ifndef _SET_LOGGER_H_
#define _SET_LOGGER_H_
#include <fstream>
#include <iostream>
#include <memory>
#include <string_view>

#ifdef USE_SPDLOG

// must be defined before include spdlog.h
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "spdlog/async.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

#define SET_LOGGER_FILENAME(name) dawn::logManager_t::return_log_manager().init_spdlog(name);

namespace dawn
{
struct logManager_t
{
    logManager_t();

    ~logManager_t();

    void init_spdlog(std::string_view prefix_filename = std::string_view{});
    void set_prefix_name();
    static logManager_t& return_log_manager();
    static std::shared_ptr<spdlog::logger> return_logger();
    static struct tm* return_timestamp();
    std::shared_ptr<spdlog::logger> spd_logger_;
};

};  // namespace dawn

#ifndef NDEBUG
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(dawn::logManager_t::return_logger(), __VA_ARGS__);
#else
#define LOG_DEBUG(...)
#endif
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(dawn::logManager_t::return_logger(), __VA_ARGS__);
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(dawn::logManager_t::return_logger(), __VA_ARGS__);
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(dawn::logManager_t::return_logger(), __VA_ARGS__);
#else
#define SET_LOGGER_FILENAME(name)
#define LOG_DEBUG(...)
#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)

#endif

#endif
