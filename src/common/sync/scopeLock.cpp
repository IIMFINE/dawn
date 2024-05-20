#include "sync/scopeLock.h"
#include "sync/posixRobustMutex.h"

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
