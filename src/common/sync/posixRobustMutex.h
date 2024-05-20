#ifndef __POSIX_ROBUST_MUTEX__
#define __POSIX_ROBUST_MUTEX__

#include <pthread.h>

#include <atomic>

namespace dawn
{
struct PosixRobustMutex
{
    PosixRobustMutex();

    ~PosixRobustMutex();

    bool isInitialized() const;

    bool initialize();

    int lock();

    void unlock();

    int tryLock();

    bool recover();

    pthread_mutex_t mutex_;
    std::atomic_bool is_initialized_{false};
    std::atomic_bool is_initializing_{false};
    // std::atomic_bool is_locked_{false};
};

}  // namespace dawn

#endif
