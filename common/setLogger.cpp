#include "setLogger.h"

#ifdef USE_SPDLOG
namespace dawn
{

/// @brief avoid multiple definition compile error.

constexpr const char* filename = "./log.txt";

logManager_t::logManager_t()
{
    std::ifstream log_file(filename);
    if(log_file)
    {
        remove(filename);
    }
    spd_logger_ = spdlog::basic_logger_mt<spdlog::async_factory>("dawn_logger", filename);
    // auto created_logger = std::make_shared<spdlog::basic_logger_mt<spdlog::async_factory>>("dawn_logger", filename);
    spd_logger_->set_pattern("[%S:%F][%L][%t][%!] %2v");
    spdlog::set_default_logger(spd_logger_);
}

std::shared_ptr<spdlog::logger> logManager_t::init_spdlog()
{
    std::ifstream log_file(filename);
    if(log_file)
    {
        remove(filename);
    }
    auto created_logger = spdlog::basic_logger_mt<spdlog::async_factory>("dawn_logger", filename);
    // auto created_logger = std::make_shared<spdlog::basic_logger_mt<spdlog::async_factory>>("dawn_logger", filename);
    created_logger->set_pattern("[%S:%F][%L][%t][%!] %2v");
    spdlog::set_default_logger(created_logger);
    return created_logger;
}

std::shared_ptr<spdlog::logger> logManager_t::return_logger()
{
    static logManager_t logger;
    return logger.spd_logger_;
}

};

#endif