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
namespace net
{

threadPool::threadPool():
queueIndex(0),
workThreadNum(1),
curWorkQueue(nullptr)
{
    storeWorkQueue[0].workQueue.clear();
    storeWorkQueue[1].workQueue.clear();
    processWorkQueue.clear();
}

void threadPool::init(uint32_t threadNum)
{
    curWorkQueue = &storeWorkQueue[queueIndex];

    for(uint32_t i = 0; i < threadNum; i++)
    {
        threadManagerList.push_back(new std::thread(std::bind(&threadPool::workThreadRun, this)));
    }
}

int threadPool::pushWorkQueue(callback_t cb)
{
    std::unique_lock<std::mutex> writeQueueLock(curWorkQueue->queueMutex);
    curWorkQueue->workQueue.push_back(cb);
    writeQueueLock.unlock();
    threadCond.notify_one();
    return PROCESS_SUCCESS;
}

void threadPool::popWorkQueue(std::unique_lock<std::mutex> &&readQueueLock)
{
    processWorkQueue.swap(storeWorkQueue[queueIndex^1].workQueue);

    storeWorkQueue[queueIndex].queueMutex.lock();

    std::unique_lock<std::mutex> writeQueueLock(storeWorkQueue[queueIndex].queueMutex);

    queueIndex ^= 1;
    curWorkQueue = &storeWorkQueue[queueIndex];

    writeQueueLock.unlock();

    readQueueLock.unlock();
}

void threadPool::workThreadRun()
{
    for(;;)
    {
        std::unique_lock<std::mutex> readQueueLock(storeWorkQueue[queueIndex^1].queueMutex);
        threadCond.wait(readQueueLock, [this]{return storeWorkQueue[0].workQueue.size()||storeWorkQueue[1].workQueue.size();});  //avoid false wake
        popWorkQueue(std::move(readQueueLock));

        for(size_t i = 0; i < processWorkQueue.size(); i++)
        {
            processWorkQueue[i]();
        }
    }
    std::cout<< "warn: work thread quit"<<std::endl;
}



};
