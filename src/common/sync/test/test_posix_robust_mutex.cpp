#include "gtest/gtest.h"

#include "common/interprocess_utilities/shm/shm_utilities.h"
#include "common/set_logger.h"
#include "common/sync/posix_robust_mutex.h"
#include "common/type.h"

TEST(test_posix_robust_mutex, intraprocess_lock)
{
    dawn::PosixRobustMutex mutex;
    LOG_INFO("mutex is initialized: {}", mutex.isInitialized());
    mutex.lock();
    LOG_INFO("mutex is locked");
    mutex.unlock();
}

TEST(test_posix_robust_mutex, intraprocess_deadlock)
{
    dawn::PosixRobustMutex mutex;

    std::thread thread(
        [&mutex]()
        {
            if (auto ret = mutex.lock() == 0)
            {
                LOG_INFO("mutex is locked");
            }
        });

    thread.join();
    if (mutex.lock() == EOWNERDEAD)
    {
        LOG_INFO("mutex is deadlock");
        EXPECT_EQ(mutex.recover(), PROCESS_SUCCESS);
    }
    mutex.unlock();
}
