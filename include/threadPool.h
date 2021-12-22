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

class threadPool
{

struct workQueue_t
{
    std::mutex queueMutex;
    std::vector<callback_t>  workQueue;
};

public:
    ~threadPool() = default;
    threadPool();
    void init(uint32_t threadNum = 1);
    int pushWorkQueue(callback_t cb);
    void popWorkQueue(std::unique_lock<std::mutex> &&curQueueWorkQueue);
    void workThreadRun();
    int producerThreadRun();
public: //todo change to private
    uint8_t                             queueIndex;
    uint32_t                            workThreadNum;
    workQueue_t                         *curWorkQueue;
    std::mutex                          queueIndexMutex;
    workQueue_t                         storeWorkQueue[2];  //use pingpong 
    std::vector<callback_t>             processWorkQueue;
    std::condition_variable             threadCond;
    std::list<std::thread*>            threadManagerList;
};

};


#endif