#ifndef _TIMER_H_
#define _TIMER_H_

#include "heap.h"
#include "threadPool.h"
#include "funcWrapper.h"
#include "hazardPointer.h"

#include <chrono>

namespace dawn
{
  struct intervalUnit
  {
    intervalUnit() = delete;
    virtual ~intervalUnit() = default;
    explicit intervalUnit(std::chrono::steady_clock::time_point timeout) :
      timeout_(timeout)
    {
    }

    bool operator<(const intervalUnit &other) const
    {
      if (timeout_ < other.timeout_)
      {
        return true;
      }
      else
      {
        return false;
      }
    }

    bool operator>(const intervalUnit &other) const
    {
      if (timeout_ > other.timeout_)
      {
        return true;
      }
      else
      {
        return false;
      }
    }

    std::chrono::steady_clock::time_point timeout_;
  };

  template<typename INTERVAL_T>
  struct concreteIntervalUnit : public intervalUnit
  {
    concreteIntervalUnit() = delete;
    ~concreteIntervalUnit() = default;

    concreteIntervalUnit(INTERVAL_T interval) :
      intervalUnit(std::chrono::steady_clock::now() + interval),
      interval_(interval)
    {
    }

    INTERVAL_T interval_;
  };


  // struct eventTimerCompanion : public enable_shared_from_this<eventTimerCompanion>
  // {
  //   eventTimerCompanion() = default;
  //   ~eventTimerCompanion();
  //   bool addEvent(funcWrapper callback);
  //   bool unregisterEvent();
  // };

  template<typename INTERVAL_UNIT>
  struct eventTimer
  {
    eventTimer() :
      isRunning_(true)
    {
      threadPool_ = threadPoolManager_.createThreadPool(2);
      threadPoolManager_.threadPoolExecute();
      timerCallbackMinHeap_ = std::make_unique<minHeap<intervalUnit, funcWrapper>>();
    }

    ~eventTimer()
    {
      threadPoolManager_.threadPoolDestroy();
    }

    eventTimer(const eventTimer &timer) = delete;
    eventTimer& operator=(const eventTimer &timer) = delete;
    void timerEventLoop()
    {
      auto func = [&]() {
        while (isRunning_.load(std::memory_order_acquire) == true)
        {
          {
            std::shared_lock<std::shared_mutex>   lock(timerCallbackMinHeap_->mutex_);
            auto heapNode = timerCallbackMinHeap_->top();
            if (heapNode.has_value())
            {
              auto now = std::chrono::steady_clock::now();
              if (now > heapNode.value()->first)
              {
                threadPool_->pushWorkQueue(std::move(heapNode.value()->second));
              }
            }
          }
        }
        };
    }

    template<typename INTERVAL_T>
    bool addEvent(funcWrapper &&cb, INTERVAL_T interval)
    {
      {
        std::unique_lock<std::shared_mutex> lock(timerCallbackMinHeap_->mutex_);
        timerCallbackMinHeap_->push(std::make_pair(std::chrono::steady_clock::now() + interval, std::move(cb)));
      }
    }

    private:
    std::atomic<bool>                                   isRunning_;
    threadPoolManager                                   threadPoolManager_;
    std::shared_ptr<threadPool>                         threadPool_;
    std::mutex                                          heapMutex_;
    std::unique_ptr<minHeap<uint32_t, funcWrapper>>     timerCallbackMinHeap_;
  };
} //namespace dawn
#endif
