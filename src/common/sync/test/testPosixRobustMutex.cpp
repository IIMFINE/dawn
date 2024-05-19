#include "gtest/gtest.h"

#include "common/sync/posixRobustMutex.h"

TEST(test_posxi_robust_mutex, intraprocess_lock)
{
    dawn::PosixRobustMutex mutex;
    mutex.lock();
    mutex.unlock();
}
