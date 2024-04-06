#include <chrono>
#include <thread>

#include "common/interprocessUtilities/notify/interprocessCondition.h"
#include "common/type.h"
#include "gtest/gtest.h"

TEST(test_interprocess_condition, init)
{
    using namespace dawn;
    InterprocessCondition condition;
    std::string identity = "test";
    ASSERT_EQ(condition.init(identity), PROCESS_SUCCESS);
}

TEST(test_interprocess_condition, wait)
{
    using namespace dawn;
    InterprocessCondition condition;
    std::string identity = "test";
    ASSERT_EQ(condition.init(identity), PROCESS_SUCCESS);
    ASSERT_EQ(condition.wait(), PROCESS_SUCCESS);
}

TEST(test_interprocess_condition, wait_for)
{
    using namespace dawn;
    InterprocessCondition condition;
    std::string identity = "test";
    ASSERT_EQ(condition.init(identity), PROCESS_SUCCESS);

    std::thread t([&condition]()
                  { ASSERT_EQ(condition.wait_for(std::chrono::milliseconds(1000)), PROCESS_SUCCESS); });

    condition.notify_all();
    t.join();
}

TEST(test_interprocess_condition, wait_for_timeout)
{
    using namespace dawn;
    InterprocessCondition condition;
    std::string identity = "test";
    ASSERT_EQ(condition.init(identity), PROCESS_SUCCESS);
    std::thread t([&condition]()
                  { ASSERT_EQ(condition.wait_for(std::chrono::milliseconds(1000)), PROCESS_FAIL); });

    t.join();
}

TEST(test_interprocess_condition, notify)
{
    using namespace dawn;
    InterprocessCondition condition;
    std::string identity = "test";
    ASSERT_EQ(condition.init(identity), PROCESS_SUCCESS);
    ASSERT_EQ(condition.notify_all(), PROCESS_SUCCESS);
}

TEST(test_interprocess_condition, notify_wait_performance)
{
    using namespace dawn;
    InterprocessCondition condition;
    std::string identity = "test";

    std::chrono::high_resolution_clock::time_point start_notify;
    std::chrono::high_resolution_clock::time_point wait_notify;

    int times = 10000;

    // record elapsed time
    uint64_t total_elapsed_time = 0;

    ASSERT_EQ(condition.init(identity), PROCESS_SUCCESS);

    for (int i = 0; i < times; i++)
    {
        std::cout << "notify wait performance test " << i << std::endl;
        std::thread t([&condition, &start_notify]()
                      {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      start_notify = std::chrono::high_resolution_clock::now();
      ASSERT_EQ(condition.notify_all(), PROCESS_SUCCESS); });
        ASSERT_EQ(condition.wait(), PROCESS_SUCCESS);
        wait_notify = std::chrono::high_resolution_clock::now();
        t.join();
        std::cout << "elapsed time " << std::chrono::duration_cast<std::chrono::nanoseconds>(wait_notify - start_notify).count() << " ns" << std::endl;
        total_elapsed_time += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_notify - start_notify).count();
    }

    std::cout << "notify wait time " << total_elapsed_time / times << " ns" << std::endl;
}

TEST(test_interprocess_condition, wait_for_condition)
{
    using namespace dawn;
    InterprocessCondition condition;
    std::string identity = "test";
    ASSERT_EQ(condition.init(identity), PROCESS_SUCCESS);

    std::thread t([&condition]()
                  { ASSERT_EQ(condition.wait([]()
                                             { return true; }),
                              PROCESS_SUCCESS); });

    condition.notify_all();
    t.join();
}

TEST(test_interprocess_condition, wait_for_condition_false)
{
    using namespace dawn;
    InterprocessCondition condition;
    std::string identity = "test";
    ASSERT_EQ(condition.init(identity), PROCESS_SUCCESS);

    std::thread t([&condition]()
                  { ASSERT_EQ(condition.wait([]()
                                             { return false; }),
                              PROCESS_SUCCESS); });

    condition.notify_all();
    t.join();
}
