#include "setLogger.h"

#ifdef USE_SPDLOG
/// @brief avoid multiple definition compile error.

namespace dawn
{
constexpr const char* filename = "./log.txt";

logManager_t::logManager_t()
{
    init_spdlog();
}

void logManager_t::init_spdlog()
{
    std::ifstream log_file(filename);
    if(log_file)
    {
        remove(filename);
    }
    spd_logger_ = spdlog::basic_logger_mt<spdlog::async_factory>("dawn_logger", filename);
    ///@brief format [second:nanosecond][log level][thread id][function name] content
    spd_logger_->set_pattern("[%S:%F][%L][%t][%!] %2v");
    spd_logger_->set_level(spdlog::level::debug);
    spdlog::set_default_logger(spd_logger_);
    spd_logger_->flush_on(spdlog::level::err);
    spd_logger_->info("format [second:nanosecond][log level][thread id][function name] content");
}

std::shared_ptr<spdlog::logger> logManager_t::return_logger()
{
    static logManager_t logger;
    return logger.spd_logger_;
}

};

#endif