#ifndef _TIMER_H_
#define _TIMER_H_

#include "heap.h"
#include "threadPool.h"
#include "funcWrapper.h"

namespace dawn
{
  enum class intervalUnit
  {
    MICROSECOND,
    MILLISECOND,
    SECOND
  };

  struct eventTimerCompanion
  {
    eventTimerCompanion() = default;
    ~eventTimerCompanion();
    bool registerEvent(funcWrapper callback, uint32_t interval, intervalUnit unit);
    bool unregisterEvent();
  };

  template<typename INTERVAL_UNIT>
  struct eventTimer
  {
    eventTimer() = default;
    ~eventTimer() = default;
    eventTimer(const eventTimer &timer) = delete;
    eventTimer& operator=(const eventTimer &timer) = delete;

    registerInstance();
    bool registerEvent();
  };

} //namespace dawn
#endif
