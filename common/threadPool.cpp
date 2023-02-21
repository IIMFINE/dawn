#include "threadPool.h"

namespace dawn
{

  threadPool::threadPool() : should_wakeup_(false)
  {
  }

  void threadPool::init(uint32_t workThreadNum)
  {
    for (uint32_t i = 0; i < workThreadNum; i++)
    {
      threadList_.push_back(new std::thread(std::bind(&threadPool::workThreadRun, this)));
    }
  }

  void threadPool::quitAllThreads()
  {
    runThreadFlag_ = ENUM_THREAD_STATUS::EXIT;
    threadCond_.notify_all();
    for (auto &threadItor : threadList_)
    {
      if (threadItor->joinable())
      {
        threadItor->join();
        delete threadItor;
      }
    }
    threadList_.clear();
  }

  void threadPool::runAllThreads()
  {
    runThreadFlag_ = ENUM_THREAD_STATUS::RUN;
  }

  void threadPool::haltAllThreads()
  {
    runThreadFlag_ = ENUM_THREAD_STATUS::STOP;
  }

  bool threadPool::popWorkQueue(funcWrapper &taskNode)
  {
    auto LF_stackNode = taskStack_.popNodeWithHazard();
    if (LF_stackNode != nullptr)
    {
      taskNode = std::move(LF_stackNode->elementVal_);
      --taskNumber_;
      ///@todo optimize to smart pointer
      delete LF_stackNode;
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }

  void threadPool::workThreadRun()
  {
    for (; runThreadFlag_ != ENUM_THREAD_STATUS::EXIT;)
    {
      std::unique_lock<std::mutex> stackLock(queueMutex_);
      // avoid spurious wakeup
      threadCond_.wait(stackLock, [this]()
                       {
                if((taskNumber_ > 0 && should_wakeup_.load() == true) || runThreadFlag_ == ENUM_THREAD_STATUS::EXIT)
                {
                    return true;
                }
                return false; });
      if (taskNumber_ == 1)
      {
        should_wakeup_ = false;
      }
      stackLock.unlock();

      funcWrapper taskNode;
      if (runThreadFlag_ != ENUM_THREAD_STATUS::EXIT)
      {
        for (; taskNumber_ != 0;)
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
    for (auto &it : spyThreadPoolGroup_)
    {
      it->runAllThreads();
    }
  }

  void threadPoolManager::threadPoolHalt()
  {
    for (auto &it : spyThreadPoolGroup_)
    {
      it->haltAllThreads();
    }
  }

  void threadPoolManager::threadPoolDestory()
  {
    for (auto &it : spyThreadPoolGroup_)
    {
      it->quitAllThreads();
    }
  }

};
