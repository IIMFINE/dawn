#include "gtest/gtest.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <errno.h>

#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

struct InotifyBenchmark
{

  void test_inotify_open_file()
  {
    constexpr const char filename[] = "/tmp/test_inotify_open_file";

    if (!fs::exists(filename))
    {
      std::ofstream ofs(filename);
      ofs.close();
    }

    int fd = inotify_init();
    ASSERT_NE(fd, -1);
    int wd = inotify_add_watch(fd, filename, IN_CREATE | IN_DELETE | IN_MODIFY);
    //print errors
    std::cout << strerror(errno) << std::endl;

    ASSERT_NE(wd, -1);
    char buffer[1024];
    auto times = run_times_;
    while (times--)
    {
      auto len = read(fd, buffer, sizeof(buffer));
      if (len == -1)
      {
        std::cout << "read error" << std::endl;
      }
      else
      {
        auto now = std::chrono::steady_clock::now();
        // std::cout << "start" << std::endl;
        elapsed_nano_.push_back(now - start_);
      }
    }

    int ret = inotify_rm_watch(fd, wd);
    ASSERT_EQ(ret, 0);
    close(fd);

  }

  void test_inotify_write_file()
  {
    constexpr const char filename[] = "/tmp/test_inotify_open_file";
    auto times = run_times_ + 1;
    while (times--)
    {
      start_ = std::chrono::steady_clock::now();
      std::ofstream ofs(filename);
      ofs.close();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }

  void print_elapsed_time()
  {
    //print average time
    std::chrono::steady_clock::duration sum = std::chrono::steady_clock::duration::zero();
    for (auto &d : elapsed_nano_)
    {
      sum += d;
    }
    std::cout << "average time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(sum).count() / elapsed_nano_.size() << " ns" << std::endl;
  }

  void start_test()
  {
    //use two thread to execute
    std::thread t1(&InotifyBenchmark::test_inotify_open_file, this);
    std::thread t2(&InotifyBenchmark::test_inotify_write_file, this);
    t1.join();
    t2.join();
    print_elapsed_time();
  }

  int                                                 run_times_ = 1000;
  std::chrono::steady_clock::time_point               start_;
  std::chrono::steady_clock::time_point               end_;
  std::vector<std::chrono::steady_clock::duration>    elapsed_nano_;
};

#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace bi = boost::interprocess;

struct BoostInterprocessTest
{

  void condition_notify()
  {
    auto times = run_times_ + 1;
    while (times--)
    {
      start_ = std::chrono::steady_clock::now();
      condition_.notify_all();
      // std::cout << "notify" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }

  void test_interprocess_condition()
  {
    auto times = run_times_;
    while (times--)
    {
      bi::scoped_lock<bi::interprocess_mutex> lock(mutex_);
      condition_.wait(lock);
      auto now = std::chrono::steady_clock::now();
      elapsed_nano_.push_back(now - start_);
      // std::cout << "start" << std::endl;
    }
    print_elapsed_time();
  }

  void print_elapsed_time()
  {
    //print average time
    std::chrono::steady_clock::duration sum = std::chrono::steady_clock::duration::zero();
    for (auto &d : elapsed_nano_)
    {
      sum += d;
    }
    std::cout << "average time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(sum).count() / elapsed_nano_.size() << " ns" << std::endl;
  }

  void start_test()
  {
    //use two thread to execute
    std::thread t1(&BoostInterprocessTest::condition_notify, this);
    std::thread t2(&BoostInterprocessTest::test_interprocess_condition, this);
    t1.join();
    t2.join();
  }

  int run_times_ = 10000;
  std::chrono::steady_clock::time_point               start_;
  std::vector<std::chrono::steady_clock::duration>    elapsed_nano_;
  bi::interprocess_condition  condition_;
  bi::interprocess_mutex    mutex_;
};


/// Spend more than 110us on invoking each time.
TEST(test_inotify, inotify_benchmark)
{
  InotifyBenchmark inotify_benchmark;
  inotify_benchmark.start_test();
}

//test boost interprocess condition performance*
//result, about 72000 ns
TEST(test_boost, interprocess_condition)
{
  //create BoostInterprocessTest in boost shared memory
  bi::shared_memory_object::remove("MySharedMemory");
  bi::managed_shared_memory segment(bi::create_only, "MySharedMemory", 65536);
  BoostInterprocessTest *test = segment.construct<BoostInterprocessTest>("test")();
  test->start_test();
}
