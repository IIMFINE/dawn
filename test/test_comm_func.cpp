#include "gtest/gtest.h"
#include "common/threadPool.h"

TEST(test_dawn, test_thread_pool)
{
  using namespace dawn;
  {
    threadPool pool;
    pool.init();
    pool.runAllThreads();
    auto test_func = []() {
      std::cout << "test_func" << std::endl;
    };
    pool.pushWorkQueue(test_func);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  std::cout << "end" << std::endl;
}

