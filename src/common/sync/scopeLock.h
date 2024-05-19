#ifndef __SCOPE_LOCK_H__
#define __SCOPE_LOCK_H__
namespace dawn
{
    struct PosixRobustMutex;

    template <typename MUTEX_T>
    struct ScopeLock
    {
        ScopeLock(MUTEX_T &mutex) : mutex_(mutex)
        {
            mutex_.lock();
        }

        ~ScopeLock()
        {
            mutex_.unlock();
        }

        bool lock()
        {
            mutex_.lock();
            return true;
        }

        bool unlock()
        {
            mutex_.unlock();
            return true;
        }

        MUTEX_T &mutex_;
    };

    template <>
    bool ScopeLock<PosixRobustMutex>::lock();

    template <>
    bool ScopeLock<PosixRobustMutex>::unlock();

}  // namespace dawn
#endif
