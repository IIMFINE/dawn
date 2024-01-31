#include "threadPool.h"

namespace dawn
{
  threadPool::~threadPool()
  {
    quitAllThreads();
  }

  void threadPool::init(uint32_t workThreadNum)
  {
    std::lock_guard<std::mutex>     lock(threadListMutex_);
    for (uint32_t i = 0; i < workThreadNum; i++)
    {
      threadList_.emplace_back(std::make_unique<std::thread>(std::bind(&threadPool::workThreadRun, this)));
    }
    threadList_.emplace_back(std::make_unique<std::thread>(std::bind(&threadPool::workThreadMaintainer, this)));
  }

  void threadPool::addWorkThread(uint32_t workThreadNum)
  {
    std::lock_guard<std::mutex>     lock(threadListMutex_);
    for (uint32_t i = 0; i < workThreadNum; i++)
    {
      threadList_.emplace_back(std::make_unique<std::thread>(std::bind(&threadPool::workThreadRun, this)));
    }
  }

  void threadPool::workThreadMaintainer()
  {
    int                     lastTaskNum = 0xffff;
    constexpr uint32_t      watchSpanTime = 2;
    for (; run_thread_flag_ != ENUM_THREAD_STATUS::EXIT;)
    {
      if (taskNumber_.load(std::memory_order_acquire) > lastTaskNum)
      {
        addWorkThread();
      }
      lastTaskNum = taskNumber_.load(std::memory_order_acquire);
      std::this_thread::sleep_for(std::chrono::seconds(watchSpanTime));
    }
  }

  void threadPool::quitAllThreads()
  {
    run_thread_flag_ = ENUM_THREAD_STATUS::EXIT;
    threadCond_.notify_all();
    std::lock_guard<std::mutex>     lock(threadListMutex_);
    for (auto &threadIter : threadList_)
    {
      if (threadIter->joinable())
      {
        threadIter->join();
      }
    }
    threadList_.clear();
  }

  void threadPool::runAllThreads()
  {
    run_thread_flag_ = ENUM_THREAD_STATUS::RUN;
  }

  void threadPool::haltAllThreads()
  {
    run_thread_flag_ = ENUM_THREAD_STATUS::STOP;
  }

  bool threadPool::popWorkQueue(FuncWrapper &taskNode)
  {
    auto LF_stackNode = taskStack_.popNodeWithHazard();
    if (LF_stackNode != nullptr)
    {
      taskNode = std::move(LF_stackNode->elementVal_);
      taskNumber_.fetch_sub(1, std::memory_order_release);
      ///@todo optimize to smart pointer
      delete LF_stackNode;
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }

  void threadPool::workThreadRun()
  {
    for (; run_thread_flag_ != ENUM_THREAD_STATUS::EXIT;)
    {
      std::unique_lock<std::mutex> stackLock(queueMutex_);
      // avoid spurious wakeup
      threadCond_.wait(stackLock, [this]() {
        if ((taskNumber_.load(std::memory_order_acquire) > 0 && should_wakeup_.load(std::memory_order_acquire) == true) || run_thread_flag_ == ENUM_THREAD_STATUS::EXIT)
        {
          return true;
        }
        return false;
        });
      if (taskNumber_.load(std::memory_order_acquire) == 1)
      {
        should_wakeup_.store(false, std::memory_order_release);
      }
      stackLock.unlock();
      FuncWrapper taskNode;
      if (run_thread_flag_ != ENUM_THREAD_STATUS::EXIT)
      {
        for (; taskNumber_.load(std::memory_order_acquire) != 0;)
        {
          if (popWorkQueue(taskNode) == PROCESS_SUCCESS)
          {
            taskNode();
          }
        }
      }
      else
      {
        return;
      }
    }
  }

  void threadPoolManager::threadPoolExecute()
  {
    for (auto &it : thread_pool_group_)
    {
      it->runAllThreads();
    }
  }

  void threadPoolManager::threadPoolHalt()
  {
    for (auto &it : thread_pool_group_)
    {
      it->haltAllThreads();
    }
  }

  void threadPoolManager::threadPoolDestroy()
  {
    for (auto &it : thread_pool_group_)
    {
      it->quitAllThreads();
    }
  }

};
