#include "interprocess_utilities/sync/posix_robust_mutex.h"

#include <stdexcept>

namespace dawn
{
namespace interprocess
{
PosixRobustMutex::PosixRobustMutex()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex_, &attr);
    pthread_mutexattr_destroy(&attr);

    bool expect_initialized_flag = false;
    is_initialized_.compare_exchange_strong(expect_initialized_flag, true, std::memory_order_release, std::memory_order_relaxed);
}

bool PosixRobustMutex::isInitialized() const { return is_initialized_; }

bool PosixRobustMutex::initialize()
{
    if (is_initialized_ == true)
    {
        return true;
    }

    bool expect_initializing_flag = false;

    if (!is_initializing_.compare_exchange_strong(expect_initializing_flag, true, std::memory_order_release, std::memory_order_relaxed))
    {
        return false;
    }

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr))
    {
        return false;
    }

    if (pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST))
    {
        pthread_mutexattr_destroy(&attr);
        return false;
    }

    if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))
    {
        pthread_mutexattr_destroy(&attr);
        return false;
    }

    if (pthread_mutex_init(&mutex_, &attr))
    {
        pthread_mutexattr_destroy(&attr);
        return false;
    }

    pthread_mutexattr_destroy(&attr);

    bool expect_initialized_flag = false;
    is_initialized_.compare_exchange_strong(expect_initialized_flag, true, std::memory_order_release, std::memory_order_relaxed);
    return true;
}

int PosixRobustMutex::lock()
{
    if (is_initialized_ == false)
    {
        throw std::runtime_error("mutex is not initialized");
    }

    return pthread_mutex_lock(&mutex_);
}

void PosixRobustMutex::unlock()
{
    if (is_initialized_ == false)
    {
        throw std::runtime_error("mutex is not initialized");
    }

    pthread_mutex_unlock(&mutex_);
}

int PosixRobustMutex::tryLock()
{
    if (is_initialized_ == false)
    {
        throw std::runtime_error("mutex is not initialized");
    }

    return pthread_mutex_trylock(&mutex_);
}

bool PosixRobustMutex::recover()
{
    if (is_initialized_ == false)
    {
        throw std::runtime_error("mutex is not initialized");
    }

    return pthread_mutex_consistent(&mutex_) == 0;
}

}  // namespace interprocess
}  // namespace dawn
