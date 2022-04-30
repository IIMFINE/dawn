#ifndef _THREAD_FUNC_H_
#define _THREAD_FUNC_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "type.h"
#include <map>
#include <sys/epoll.h>
#include <mutex>
#include <thread>
#include <vector>
#include <condition_variable>
#include <list>
#include "setLogger.h"
#include "funcWrapper.h"

namespace net
{
class threadPool;
class threadPoolManager;

class threadPool
{
public:

enum class ENUM_THREAD_STATUS
{
    STOP,
    RUN,
    EXIT
};

struct workQueue_t
{
    std::mutex queueMutex;
    std::vector<funcWarpper>  workQueue;
};

    threadPool();
    ~threadPool() = default;
    void init(uint32_t workThreadNum = 1);

    template<typename FUNC_T>
    void eventDriverThread(FUNC_T returnCb)
    {
        while(1)
        {
            if(runThreadFlag == ENUM_THREAD_STATUS::RUN)
            {
                auto workTask = returnCb();
                if(pushWorkQueue(workTask) != PROCESS_SUCCESS)
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

    template<typename FUNC_T>
    void setEventThread(FUNC_T returnCb)
    {
        threadList.push_back(new std::thread(&threadPool::eventDriverThread<FUNC_T>, this, returnCb));
    }

    template<typename FUNC_T>
    int pushWorkQueue(FUNC_T cb)
    {
        std::unique_lock<std::mutex> writeQueueLock(storeWorkQueue.queueMutex);
        storeWorkQueue.workQueue.emplace_back(cb);
        writeQueueLock.unlock();
        threadCond.notify_one();
        return PROCESS_SUCCESS;
    }

    void quitAllThreads();
    void haltAllThreads();
    void runAllThreads();
private:
    void popWorkQueue(std::vector<funcWarpper> &TL_processedQueue);
    void workThreadRun();
private:
    volatile ENUM_THREAD_STATUS                 runThreadFlag = ENUM_THREAD_STATUS::STOP;
    workQueue_t                                 storeWorkQueue;
    std::condition_variable                     threadCond;
    std::vector<std::thread*>                   threadList;
};

template<>
int threadPool::pushWorkQueue<int>(int cb);

template<>
int threadPool::pushWorkQueue<uint8_t>(uint8_t cb);

template<>
int threadPool::pushWorkQueue<char>(char cb);

class threadPoolManager
{
public:
    threadPoolManager() = default;
    ~threadPoolManager() = default;
    template<typename ...executeTask_t>
    void createThreadPool(int workThreadNum = 1, executeTask_t&&... executeTasks)
    {
        std::unique_ptr<threadPool>  uniPtr_spyThreadPool(new threadPool());
        uniPtr_spyThreadPool->init(workThreadNum);
        if(sizeof...(executeTasks) != 0)
        {
            std::initializer_list<int>{(uniPtr_spyThreadPool->setEventThread(std::forward<executeTask_t>(executeTasks)), 0)...};
        }
        spyThreadPoolGroup.push_back(std::move(uniPtr_spyThreadPool));
    }
    void threadPoolExecute();
    void threadPoolHalt();
    void threadPoolDestory();
private:
    std::vector<std::unique_ptr<threadPool>>         spyThreadPoolGroup;
};

};


#endif