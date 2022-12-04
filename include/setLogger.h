#ifndef _SET_LOGGER_H_
#define _SET_LOGGER_H_
#include <iostream>
#include <memory>
#include <fstream>

#ifdef USE_SPDLOG
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace dawn
{

/// @brief avoid multiple definition compile error.

struct logManager_t
{
    logManager_t();

    ~logManager_t() = default;

    static std::shared_ptr<spdlog::logger> init_spdlog();
    static std::shared_ptr<spdlog::logger> return_logger();
    std::shared_ptr<spdlog::logger> spd_logger_;
};

};

#ifndef NDEBUG
#define LOG_DEBUG(...) dawn::logManager_t::return_logger()->debug(__VA_ARGS__);
#else
#define LOG_DEBUG(...) dawn::logManager_t::return_logger()->debug(__VA_ARGS__);
#endif
#define LOG_INFO(...) dawn::logManager_t::return_logger()->info(__VA_ARGS__);
#define LOG_WARN(...) dawn::logManager_t::return_logger()->warn(__VA_ARGS__);
#define LOG_ERROR(...) dawn::logManager_t::return_logger()->error(__VA_ARGS__);

#else

#define LOG_DEBUG(...)
#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)

#endif

#endif