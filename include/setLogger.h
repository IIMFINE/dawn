#ifndef _SET_LOGGER_H_
#define _SET_LOGGER_H_

#ifdef USE_LOG4CPLUS
#include <log4cplus/log4cplus.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/initializer.h>


static log4cplus::Logger DAWN_LOG = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("DAWN_LOG"));

class logManager_t
{
public:
    logManager_t()
    {
        // log4cplus::Initializer initializer;
        log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT("/home/pan/share/myProject/tinyKafka/DAWN_streamMessageSystem/include/thirdParty/log4cplus/log4cplus_config/log4cplus.config"));

    }
};

static logManager_t logManager;
#endif
#endif