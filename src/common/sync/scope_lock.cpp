#include "sync/scope_lock.h"
#include "sync/posix_robust_mutex.h"

namespace dawn
{
template <>
bool ScopeLock<PosixRobustMutex>::lock()
{
    return true;
}

template <>
bool ScopeLock<PosixRobustMutex>::unlock()
{
    return true;
}

}  // namespace dawn
