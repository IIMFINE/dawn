#include "gtest/gtest.h"
#include "common/threadPool.h"
#include "test_helper.h"
#include "common/memoryPool.h"
#include <chrono>
#include <thread>

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

TEST(test_dawn, dawn_memory_pool_benchmark)
{
  using namespace dawn;
  volatile bool runFlag = false;
  uint32_t loop_time = 0xffffff;
  allocMem(1);

  auto func = [&]()
  {
    unsigned long total_time_span = 0;
    while (runFlag == false)
    {
      std::this_thread::yield();
    }
    for (uint32_t i = 0; i < loop_time; i++)
    {
      {
        timeCounter  counter("dawn_alloc");
        auto p = allocMem(128);
        freeMem(p);
      }
      total_time_span += timeCounter::getTimeSpan("dawn_alloc");
    }
    LOG_INFO("dawn memory pool spend time {} ns", total_time_span / loop_time);
  };
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  runFlag = true;
  std::this_thread::sleep_for(std::chrono::seconds(10));
}


TEST(test_dawn, malloc_benchmark)
{
  using namespace dawn;
  volatile bool runFlag = false;
  uint32_t loop_time = 0xffffff;
  allocMem(1);

  auto func = [&]()
  {
    unsigned long total_time_span = 0;
    while (runFlag == false)
    {
      std::this_thread::yield();
    }
    for (uint32_t i = 0; i < loop_time; i++)
    {
      {
        timeCounter  counter("malloc");
        auto p = malloc(128);
        free(p);
      }
      total_time_span += timeCounter::getTimeSpan("malloc");
    }
    LOG_INFO("dawn memory pool spend time {} ns", total_time_span / loop_time);
  };
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  runFlag = true;
  std::this_thread::sleep_for(std::chrono::seconds(10));
}

