#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
 #include <unistd.h>
#include "net.h"
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <condition_variable>
#include "threadPool.h"
#include "setLogger.h"
namespace net
{


threadPool::threadPool()
{
    storeWorkQueue.workQueue.clear();
}

void threadPool::init(uint32_t workThreadNum)
{
    for(uint32_t i = 0; i < workThreadNum; i++)
    {
        threadList.push_back(new std::thread(std::bind(&threadPool::workThreadRun, this)));
    }
}

void threadPool::eventDriverThread(returnCallback_t returnCb)
{
    while(1)
    {
        if(runThreadFlag == ENUM_THREAD_STATUS::RUN)
        {
            auto workThreadTask = returnCb();
            if(pushWorkQueue(workThreadTask) != PROCESS_SUCCESS)
            {
                LOG4CPLUS_ERROR(DAWN_LOG, "push work task to queue wrong");
            }
        }
        else if(runThreadFlag == ENUM_THREAD_STATUS::STOP)
        {
            std::this_thread::yield();
        }
        else if(runThreadFlag == ENUM_THREAD_STATUS::EXIT)
        {
            LOG4CPLUS_WARN(DAWN_LOG, "event thread exit");
            break;
        }
    }
}

void threadPool::setEventThread(returnCallback_t returnCb)
{
    threadList.push_back(new std::thread(&threadPool::eventDriverThread, this, returnCb));
}

int threadPool::pushWorkQueue(callback_t cb)
{
    std::unique_lock<std::mutex> writeQueueLock(storeWorkQueue.queueMutex);
    storeWorkQueue.workQueue.push_back(cb);
    writeQueueLock.unlock();
    threadCond.notify_one();
    return PROCESS_SUCCESS;
}

void threadPool::quitAllThreads()
{
    runThreadFlag = ENUM_THREAD_STATUS::EXIT;
    for(auto threadItor : threadList)
    {
        if(threadItor->joinable())
        {
            threadItor->join();
            delete threadItor;
        }
    }
    threadList.clear();
}

void threadPool::runAllThreads()
{
    runThreadFlag = ENUM_THREAD_STATUS::RUN;
}

void threadPool::haltAllThreads()
{
    runThreadFlag = ENUM_THREAD_STATUS::STOP;
}

void threadPool::popWorkQueue(std::vector<callback_t> &TL_processedQueue)
{
    TL_processedQueue.swap(storeWorkQueue.workQueue);
}

void threadPool::workThreadRun()
{
    std::vector<callback_t>   TL_processedQueue;
    size_t  processedQueueSize = 0;
    for(;runThreadFlag != ENUM_THREAD_STATUS::EXIT;)
    {
        std::unique_lock<std::mutex> readQueueLock(storeWorkQueue.queueMutex);
        threadCond.wait(readQueueLock, [this]{return !storeWorkQueue.workQueue.empty();});  //avoid false wake
        popWorkQueue(TL_processedQueue);
        readQueueLock.unlock();
        processedQueueSize = TL_processedQueue.size();
        for(size_t i = 0; i < processedQueueSize; i++)
        {
            TL_processedQueue[i]();
        }
        processedQueueSize = 0;
        TL_processedQueue.clear();
    }
    LOG4CPLUS_WARN(DAWN_LOG, "the TL_processedQueue size "<< TL_processedQueue.size());
}

void threadPoolManager::threadPoolExecute()
{
    for(auto &it : spyThreadPoolGroup)
    {
        it->runAllThreads();
    }
}

void threadPoolManager::threadPoolHalt()
{
    for(auto &it : spyThreadPoolGroup)
    {
        it->haltAllThreads();
    }
}


void threadPoolManager::threadPoolDestory()
{
    for(auto &it : spyThreadPoolGroup)
    {
        it->quitAllThreads();
    }
}

};
