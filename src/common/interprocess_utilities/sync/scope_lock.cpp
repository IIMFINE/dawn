#include "interprocess_utilities/sync/scope_lock.h"
#include "interprocess_utilities/sync/posix_robust_mutex.h"

#include <cerrno>

namespace dawn
{
namespace interprocess
{

template <>
bool ScopeLock<PosixRobustMutex>::lock()
{
    int ret = mutex_.lock();

    if (ret == 0)
    {
        return true;
    }

    if (ret == EOWNERDEAD)
    {
        if (!mutex_.recover())
        {
            return false;
        }
        return true;
    }

    return true;
}

template <>
void ScopeLock<PosixRobustMutex>::unlock()
{
    mutex_.unlock();
}

}  // namespace interprocess
}  // namespace dawn
