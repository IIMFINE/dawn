#include "setLogger.h"
#include <sstream>
#ifdef USE_SPDLOG
/// @brief avoid multiple definition compile error.

namespace dawn
{
  logManager_t::logManager_t()
  {
    init_spdlog();
  }

  logManager_t::~logManager_t()
  {
    spd_logger_->flush();
    spdlog::shutdown();
  }

  void logManager_t::init_spdlog(std::string_view prefix_filename)
  {
    char timestamp[100] = { 0 };
    std::stringstream filename;
    std::stringstream logger_name;
    auto time_info = logManager_t::return_timestamp();
    strftime(timestamp, sizeof(timestamp), "_%y%m%d_%H_%M_%S_", time_info);
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

    std::ifstream log_file(filename.str());

    if (log_file)
    {
      remove(filename.str().c_str());
    }

    spd_logger_ = spdlog::basic_logger_mt<spdlog::async_factory>(logger_name.str(), filename.str());

    spd_logger_->set_pattern("[%S:%F][%L][%t][%s:%#] %2v");
    spd_logger_->set_level(spdlog::level::debug);
    spdlog::set_default_logger(spd_logger_);
    spd_logger_->flush_on(spdlog::level::debug);
    spd_logger_->info("format [second:nanosecond][log level][thread id][source file][line] content");
  }

  logManager_t& logManager_t::return_log_manager()
  {
    static logManager_t logger;
    return logger;
  }

  std::shared_ptr<spdlog::logger> logManager_t::return_logger()
  {
    return logManager_t::return_log_manager().spd_logger_;
  }

  struct tm *logManager_t::return_timestamp()
  {
    time_t raw_time;
    struct tm *time_info;
    time(&raw_time);
    time_info = localtime(&raw_time);
    return time_info;
  }
};

#endif
