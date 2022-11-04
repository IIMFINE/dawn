#ifndef _SET_LOGGER_H_
#define _SET_LOGGER_H_

#ifdef USE_LOG4CPLUS
#include <log4cplus/log4cplus.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/initializer.h>

constexpr const char* LOG_CONFIG_PATH = "/mnt/win_share/myProject/tinyKafka/DAWN_streamMessageSystem/config/thirdParty/log4cplus/log4cplus_config/log4cplus.config";

static log4cplus::Logger DAWN_LOG = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("DAWN_LOG"));


class logManager_t
{
public:
    logManager_t()
    {
        // log4cplus::Initializer initializer;
        log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT(LOG_CONFIG_PATH));
    }
};

static logManager_t logManager;

#ifndef NDEBUG
#define LOG_DEBUG(...) LOG4CPLUS_DEBUG(DAWN_LOG, __VA_ARGS__);
#else
#define LOG_DEBUG(...)
#endif
#define LOG_INFO(...) LOG4CPLUS_INFO(DAWN_LOG, __VA_ARGS__);
#define LOG_WARN(...) LOG4CPLUS_WARN(DAWN_LOG, __VA_ARGS__);
#define LOG_ERROR(...) LOG4CPLUS_ERROR(DAWN_LOG, __VA_ARGS__);

#endif
#endif