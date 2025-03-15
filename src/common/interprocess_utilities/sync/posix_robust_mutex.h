#ifndef __POSIX_INTERPROCESS_ROBUST_MUTEX__
#define __POSIX_INTERPROCESS_ROBUST_MUTEX__

#include <pthread.h>

#include <atomic>

namespace dawn
{
namespace interprocess
{
struct PosixRobustMutex
{
    PosixRobustMutex();

    ~PosixRobustMutex() = default;

    bool isInitialized() const;

    bool initialize();

    int lock();

    void unlock();

    int tryLock();

    bool recover();

    pthread_mutex_t mutex_;
    std::atomic_bool is_initialized_{false};
    std::atomic_bool is_initializing_{false};
};

}  // namespace interprocess
}  // namespace dawn
#endif  // __POSIX_INTERPROCESS_ROBUST_MUTEX__
