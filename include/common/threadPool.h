#ifndef _THREAD_FUNC_H_
#define _THREAD_FUNC_H_

#include "type.h"
#include <mutex>
#include <thread>
#include <vector>
#include <condition_variable>
#include <list>
#include "setLogger.h"
#include "funcWrapper.h"
#include "lockFree.h"

namespace dawn
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
      std::vector<funcWrapper> workQueue;
    };

    threadPool();
    ~threadPool() = default;
    void init(uint32_t workThreadNum = 1);

    template <typename FUNC_T>
    void eventDriverThread(FUNC_T returnCb)
    {
      while (1)
      {
        if (runThreadFlag_ == ENUM_THREAD_STATUS::RUN)
        {
          if (pushWorkQueue(returnCb()) != PROCESS_SUCCESS)
          {
            LOG_ERROR("push work task to queue wrong");
          }
          else
          {
            threadCond_.notify_one();
          }
        }
        else if (runThreadFlag_ == ENUM_THREAD_STATUS::STOP)
        {
          std::this_thread::yield();
        }
        else if (runThreadFlag_ == ENUM_THREAD_STATUS::EXIT)
        {
          LOG_WARN("event thread exit");
          break;
        }
      }
    }

    template <typename FUNC_T>
    void setEventThread(FUNC_T returnCb)
    {
      threadList_.push_back(new std::thread(&threadPool::eventDriverThread<FUNC_T>, this, returnCb));
    }

    template <typename FUNC_T>
    int pushWorkQueue(FUNC_T &&workTask)
    {
      return pushWorkQueueImpl(std::forward<FUNC_T>(workTask));
    }

    template <typename T, std::enable_if_t<dawn::is_callable<T>::value, int> = 0>
    int pushWorkQueueImpl(T &&workTask)
    {
      auto p_LF_node = new LF_node_t<funcWrapper>(std::forward<T>(workTask));
      taskStack_.pushNode(p_LF_node);
      should_wakeup_ = true;
      ++taskNumber_;
      return PROCESS_SUCCESS;
    }

    template <typename T, std::enable_if_t<!dawn::is_callable<T>::value, int> = 0>
    int pushWorkQueueImpl(T &&workTask)
    {
      return PROCESS_SUCCESS;
    }

    void quitAllThreads();
    void haltAllThreads();
    void runAllThreads();

    private:
    bool popWorkQueue(funcWrapper &taskNode);
    void workThreadRun();

    private:
    volatile ENUM_THREAD_STATUS runThreadFlag_ = ENUM_THREAD_STATUS::STOP;
    std::mutex queueMutex_;
    // a proximity number to identify the amount of tasks.
    volatile int taskNumber_;
    std::atomic_bool should_wakeup_;
    lockFreeStack<funcWrapper> taskStack_;
    std::condition_variable threadCond_;
    std::vector<std::thread *> threadList_;
  };

  class threadPoolManager
  {
    public:
    threadPoolManager() = default;
    ~threadPoolManager() = default;
    template <typename... executeTask_t>
    void createThreadPool(int workThreadNum = 1, executeTask_t &&...executeTasks)
    {
      std::unique_ptr<threadPool> uniPtr_spyThreadPool(new threadPool());
      uniPtr_spyThreadPool->init(workThreadNum);
      if (sizeof...(executeTasks) != 0)
      {
        std::initializer_list<int>{(uniPtr_spyThreadPool->setEventThread(std::forward<executeTask_t>(executeTasks)), 0)...};
      }
      spyThreadPoolGroup_.push_back(std::move(uniPtr_spyThreadPool));
    }
    void threadPoolExecute();
    void threadPoolHalt();
    void threadPoolDestory();

    private:
    std::vector<std::unique_ptr<threadPool>> spyThreadPoolGroup_;
  };

};

#endif