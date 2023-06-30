#ifndef _TIMER_H_
#define _TIMER_H_

#include "heap.h"
#include "threadPool.h"
#include "funcWrapper.h"
#include "hazardPointer.h"

namespace dawn
{
  enum class intervalUnit
  {
    MICROSECOND,
    MILLISECOND,
    SECOND
  };

  struct eventTimerCompanion : public enable_shared_from_this<eventTimerCompanion>
  {
    eventTimerCompanion() = default;
    ~eventTimerCompanion();
    bool addEvent(funcWrapper callback, uint32_t interval, intervalUnit unit);
    bool unregisterEvent();
  };

  template<typename INTERVAL_UNIT>
  struct eventTimer
  {
    eventTimer() :
      isRunning_(true)
    {
      threadPool_ = threadPoolManager_.createThreadPool(2);
      timerCallbackMinHeap_ = std::make_unique<minHeap<uint32_t, funcWrapper>>();
    }
    ~eventTimer()
    {
      threadPoolManager_.threadPoolDestroy();
    }
    eventTimer(const eventTimer &timer) = delete;
    eventTimer& operator=(const eventTimer &timer) = delete;
    void activateTimerEventLoop()
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
              auto interval = std::chrono::duration_cast<INTERVAL_UNIT>(now - heapNode.value().first);
              if (interval.count() >= heapNode.value().second)
              {
              }
            }
          }
        }
        };
    }
    private:
    std::atomic<bool>                                   isRunning_;
    threadPoolManager                                   threadPoolManager_;
    std::shared_ptr<threadPool>                         threadPool_;
    std::unique_ptr<minHeap<uint32_t, funcWrapper>>     timerCallbackMinHeap_;
  };
} //namespace dawn
#endif
