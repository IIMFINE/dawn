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
#include "calllBack.h"

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
    std::vector<callback_t>  workQueue;
};

    threadPool();
    ~threadPool() = default;
    void init(uint32_t workThreadNum = 1);
    void eventDriverThread(returnCallback_t returnCb);
    void setEventThread(returnCallback_t cb);
    int pushWorkQueue(callback_t cb);
    void quitAllThreads();
    void haltAllThreads();
    void runAllThreads();
private:
    void popWorkQueue(std::vector<callback_t> &TL_processedQueue);
    void workThreadRun();
private:
    volatile ENUM_THREAD_STATUS                 runThreadFlag = ENUM_THREAD_STATUS::STOP;
    workQueue_t                                 storeWorkQueue;
    std::condition_variable                     threadCond;
    std::vector<std::thread*>                   threadList;
};

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