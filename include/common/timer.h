#ifndef _TIMER_H_
#define _TIMER_H_

#include "heap.h"
#include "threadPool.h"
#include "FuncWrapper.h"
#include "hazardPointer.h"
#include "setLogger.h"

#include <chrono>
#include <condition_variable>

namespace dawn
{
  struct IntervalUnit
  {
    IntervalUnit() = default;
    virtual ~IntervalUnit() = default;
    explicit IntervalUnit(std::chrono::steady_clock::time_point timeout) : timeout_(timeout)
    {
    }

    std::chrono::steady_clock::time_point getTimeOutValue()
    {
      return timeout_;
    }

    void setTimerOut(std::chrono::steady_clock::time_point timeout)
    {
      timeout_ = timeout;
    }

    /// @brief Return the interval time in nanoseconds.
    /// @return Return the interval time in nanoseconds.
    virtual std::chrono::nanoseconds getInterval() = 0;

    struct IntervalLessOpt
    {
      bool operator()(const IntervalUnit &lhs, const IntervalUnit &rhs) const
      {
        if (lhs.timeout_ < rhs.timeout_)
        {
          return true;
        }
        else
        {
          return false;
        }
      }

      bool operator()(const std::shared_ptr<IntervalUnit> &lhs, const std::shared_ptr<IntervalUnit> &rhs) const
      {
        if ((*lhs).timeout_ < (*rhs).timeout_)
        {
          return true;
        }
        else
        {
          return false;
        }
      }
    };

    std::chrono::steady_clock::time_point timeout_;
  };

  template <typename INTERVAL_T>
  struct ConcreteIntervalUnit : public IntervalUnit
  {
    ConcreteIntervalUnit() = delete;
    ~ConcreteIntervalUnit() = default;

    ConcreteIntervalUnit(INTERVAL_T interval) : IntervalUnit(std::chrono::steady_clock::now() + interval),
      interval_(interval)
    {
    }

    std::chrono::nanoseconds getInterval()
    {
      return std::chrono::duration_cast<std::chrono::nanoseconds>(interval_);
    }

    INTERVAL_T interval_;
  };

  struct EventTimer
  {
    EventTimer() :
      is_running_(false),
      min_interval_(std::chrono::seconds(1))
    {
      thread_pool_ = thread_pool_manager_.createThreadPool(1);
      timer_callback_min_heap_ = std::make_unique<MinHeap<std::shared_ptr<IntervalUnit>, FuncWrapper, IntervalUnit::IntervalLessOpt>>();
      thread_pool_->setEventThread([this]() {
        //It will stuck until timer destruct.
        this->timerEventLoop();
        return PROCESS_SUCCESS;
        });
    }

    ~EventTimer()
    {
      destroyTimer();
    }

    EventTimer(const EventTimer &timer) = delete;
    EventTimer &operator=(const EventTimer &timer) = delete;
    void timerEventLoop()
    {
      while (is_running_.load(std::memory_order_acquire) == true)
      {
        {
          std::unique_lock<std::mutex> lock(heap_mutex_);
          if (timer_callback_min_heap_->top().has_value())
          {
            auto now = std::chrono::steady_clock::now();
            if (now > timer_callback_min_heap_->top().value()->first->getTimeOutValue())
            {
              auto heapNode = timer_callback_min_heap_->pop();
              auto cb = heapNode.value()->second;
              thread_pool_->pushWorkQueue(std::move(cb));
              heapNode.value()->first->setTimerOut(now + std::chrono::duration_cast<std::chrono::nanoseconds>(heapNode.value()->first->getInterval()));

              timer_callback_min_heap_->push(std::move(*heapNode.value()));
              continue;
            }
          }
        }

        std::chrono::nanoseconds min_interval;

        {
          std::unique_lock<std::mutex> lock(min_interval_mutex_);
          min_interval = min_interval_;
        }

        // stuck until timeout or new event add.
        std::unique_lock<std::mutex> lock(cv_mutex_);
        cv_.wait_for(lock, min_interval);
      }
    }

    template <typename INTERVAL_T>
    bool addEvent(FuncWrapper &&cb, INTERVAL_T interval)
    {
      {
        std::unique_lock<std::mutex> lock(heap_mutex_);
        if (timer_callback_min_heap_->push(std::make_pair(std::make_shared<ConcreteIntervalUnit<INTERVAL_T>>(interval), std::move(cb))) == PROCESS_SUCCESS)
        {
          {
            std::unique_lock<std::mutex> lock(min_interval_mutex_);
            if (min_interval_ > std::chrono::duration_cast<std::chrono::nanoseconds>(interval))
            {
              min_interval_ = std::chrono::duration_cast<std::chrono::nanoseconds>(interval);
            }
          }

          cv_.notify_one();
          return PROCESS_SUCCESS;
        }
        else
        {
          return PROCESS_FAIL;
        }
      }
    }

    bool activateTimer()
    {
      is_running_.store(true, std::memory_order_release);
      thread_pool_manager_.threadPoolExecute();
      return PROCESS_SUCCESS;
    }

    bool deactivateTimer()
    {
      is_running_.store(false, std::memory_order_release);
      thread_pool_manager_.threadPoolHalt();
      cv_.notify_all();
      return PROCESS_SUCCESS;
    }

    bool destroyTimer()
    {
      is_running_.store(false, std::memory_order_release);
      thread_pool_manager_.threadPoolDestroy();
      cv_.notify_all();
      return PROCESS_SUCCESS;
    }

  private:
    std::atomic<bool> is_running_;
    threadPoolManager thread_pool_manager_;
    std::shared_ptr<threadPool> thread_pool_;
    std::mutex heap_mutex_;
    std::unique_ptr<MinHeap<std::shared_ptr<IntervalUnit>, FuncWrapper, IntervalUnit::IntervalLessOpt>> timer_callback_min_heap_;
    std::mutex cv_mutex_;
    std::condition_variable  cv_;
    std::mutex min_interval_mutex_;
    // the minimum interval time of the timer.
    std::chrono::nanoseconds min_interval_;
  };
} // namespace dawn
#endif
