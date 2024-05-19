#include "sync/posixRobustMutex.h"
#include "setLogger.h"
#include "type.h"

#include <errno.h>

#include <cstring>
#include <stdexcept>

namespace dawn
{

    PosixRobustMutex::PosixRobustMutex()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
        pthread_mutex_init(&mutex_, &attr);
        pthread_mutexattr_destroy(&attr);

        bool expect_initialized_flag = false;
        is_initialized_.compare_exchange_strong(expect_initialized_flag, true, std::memory_order_release, std::memory_order_relaxed);
    }

    PosixRobustMutex::~PosixRobustMutex()
    {
        pthread_mutex_destroy(&mutex_);
    }

    bool PosixRobustMutex::isInitialized() const
    {
        return is_initialized_;
    }

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
            LINUX_ERROR_PRINT();
            return false;
        }

        if (pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST))
        {
            LINUX_ERROR_PRINT();
            pthread_mutexattr_destroy(&attr);
            return false;
        }

        if (pthread_mutex_init(&mutex_, &attr))
        {
            LINUX_ERROR_PRINT();
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
        return pthread_mutex_lock(&mutex_);
    }

    void PosixRobustMutex::unlock()
    {
        pthread_mutex_unlock(&mutex_);
    }

    int PosixRobustMutex::tryLock()
    {
        return pthread_mutex_trylock(&mutex_);
    }

    bool PosixRobustMutex::recover()
    {
        auto result = pthread_mutex_consistent(&mutex_);
        if (result == 0)
        {
            return PROCESS_SUCCESS;
        }

        throw std::runtime_error("recover failed" + std::string(strerror(result)));
    }
}  // namespace dawn
