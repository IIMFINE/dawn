#ifndef __SCOPE_LOCK_H__
#define __SCOPE_LOCK_H__
namespace dawn
{
namespace interprocess
{
struct PosixRobustMutex;

template <typename MUTEX_T>
struct ScopeLock
{
    ScopeLock(MUTEX_T &mutex) : mutex_(mutex) { lock(); }

    ~ScopeLock() { unlock(); }

    bool lock()
    {
        mutex_.lock();
        return true;
    }

    void unlock() { mutex_.unlock(); }

    MUTEX_T &mutex_;
};

template <>
bool ScopeLock<PosixRobustMutex>::lock();

template <>
void ScopeLock<PosixRobustMutex>::unlock();

}  // namespace interprocess
}  // namespace dawn

#endif  // __SCOPE_LOCK_H__
