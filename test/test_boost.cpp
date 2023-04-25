#include "gtest/gtest.h"
#include <iostream>
#include <thread>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/sync/named_upgradable_mutex.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>

#include <signal.h>
TEST(test_core_dump, test_core_dump)
{
  raise(SIGABRT);
}

namespace BI = boost::interprocess;

TEST(test_boost, create_shared_memory)
{
  using namespace BI;
  try
  {
    std::string name("hello_world_dawn");
    std::vector<shared_memory_object> object_vec;
    for (int i = 0; i < 1;i++)
    {
      auto tmp_name = name + std::to_string(i);

      shared_memory_object segment(open_or_create, tmp_name.c_str(), read_write);
      segment.truncate(256);
      mapped_region region(segment, read_write);
      object_vec.push_back(std::move(segment));
      auto mem = reinterpret_cast<char*>(region.get_address());
      std::memset(mem, 0x1, 256);
    }
  }
  catch (const interprocess_exception &e)
  {
    std::cout << "exception " << std::endl;
    std::cout << e.what() << std::endl;
  }

  while (1)
  {
    std::this_thread::yield();
  }
}

TEST(test_boost, create_shared_memory_read)
{
  using namespace BI;
  try
  {
    char* mem;
    shared_memory_object segment(open_or_create, "hello_world_dawn0", read_write);
    segment.truncate(256);
    mapped_region region(segment, read_write);
    mem = reinterpret_cast<char*>(region.get_address());
    for (int i = 0; i < 256; i++)
    {
      std::cout << (int)mem[i] << std::endl;
    }
  }
  catch (const interprocess_exception &e)
  {
    std::cout << "exception " << std::endl;
    std::cout << e.what() << std::endl;
  }

}


TEST(test_boost, create_named_semaphore_shm)
{
  using namespace BI;
  named_semaphore semaphore(open_or_create, "hello_world_dawn", 0);
  // shared_memory_object segment(open_or_create, "hello_world_dawn", read_write);
  // segment.truncate(256);
  // mapped_region region(segment, read_write);
  // auto mem = reinterpret_cast<char*>(region.get_address());

}

TEST(test_boost, boost_version)
{
  std::cout << BOOST_VERSION << std::endl;
}

TEST(test_boost, test_upgradable_mutex_shared1)
{
  using namespace BI;
  named_upgradable_mutex upgradable_mut(open_or_create, "hello_world_dawn");
  std::cout << "try to lock sharable" << std::endl;
  upgradable_mut.lock_sharable();
  std::cout << "end " << std::endl;
  while (1)
  {
    std::this_thread::yield();
  }
}

TEST(test_boost, test_upgradable_mutex_shared2)
{
  using namespace BI;
  named_upgradable_mutex upgradable_mut(open_or_create, "hello_world_dawn");
  std::cout << "try to lock sharable" << std::endl;
  upgradable_mut.lock_sharable();
  std::cout << "end " << std::endl;
  while (1)
  {
    std::this_thread::yield();
  }
}

TEST(test_boost, test_upgradable_mutex_upgrade)
{
  using namespace BI;
  named_upgradable_mutex upgradable_mut(open_or_create, "hello_world_dawn");
  std::cout << "try to lock upgrade" << std::endl;
  // upgradable_mut.lock_upgradable();
  if (upgradable_mut.try_lock_upgradable() == false)
  {
    std::cout << "failed to lock upgrade" << std::endl;
  }
  std::cout << "end " << std::endl;
  while (1)
  {
    std::this_thread::yield();
  }
}

TEST(test_boost, test_sharable_lock)
{
  using namespace BI;
  named_upgradable_mutex::remove("hello_world_dawn");
  named_upgradable_mutex upgradable_mut(open_or_create, "hello_world_dawn");
  sharable_lock<named_upgradable_mutex>   lock(upgradable_mut, defer_lock);
  std::cout << "try to lock sharable" << std::endl;
  lock.lock();
  std::cout << "end" << std::endl;
  while (1)
  {
    std::this_thread::yield();
  }
}

TEST(test_boost, test_upgradable_lock)
{
  using namespace BI;
  remove("hello_world_dawn");
  named_upgradable_mutex upgradable_mut(open_or_create, "hello_world_dawn");
  std::cout << "try to lock upgradable" << std::endl;

  upgradable_lock<named_upgradable_mutex>   lock(upgradable_mut, defer_lock);

  lock.lock();
  std::cout << "lock" << std::endl;
  lock.unlock();
  named_upgradable_mutex::remove("hello_world_dawn");
  std::cout << "remove mutex" << std::endl;
  while (1)
  {
    std::this_thread::yield();
  }
}


struct lock_guard
{
  lock_guard(BI::named_mutex &&mutex):
    lock(mutex)
  {

  }
  ~lock_guard()
  {
    std::cout << "destruct" << std::endl;
    lock.unlock();
    BI::named_mutex::remove("hello_world_dawn_scoped");
  }
  BI::scoped_lock<boost::interprocess::named_mutex> lock;
};

TEST(test_boost, test_scoped_lock)
{
  using namespace boost::interprocess;
  named_mutex mutex(open_or_create, "hello_world_dawn_scoped");
  {
    lock_guard      lock_g(std::move(mutex));
  }

  while (1)
  {
    std::this_thread::yield();
  }
}

TEST(test_boost, test_scoped_lock2)
{
  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, "hello_world_dawn_scoped");

  boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex, boost::interprocess::defer_lock);
  lock.lock();
  std::cout << "next " << std::endl;
  // boost::interprocess::named_mutex::remove("hello_world_dawn_scoped");
  while (1)
  {
    std::this_thread::yield();
  }
}

TEST(test_boost, test_mutex_remove)
{
  if (boost::interprocess::named_mutex::remove("hello_world_dawn_scoped"))
  {
    std::cout << "remove" << std::endl;
  }
}

TEST(test_boost, test_semaphore_wait)
{
  
}

struct test_anonymous
{
  BI::interprocess_mutex    mutex_1_;
  BI::interprocess_mutex    mutex_2_;
};

TEST(test_boost, test_deadlock_1)
{
  using namespace BI;
  shared_memory_object  shm(BI::open_or_create, "test_mutex", read_write);
  shm.truncate(sizeof(test_anonymous));
  mapped_region region(shm, read_write);
  auto addr = region.get_address();
  auto test_anonymous_ptr = new (addr) test_anonymous;

  scoped_lock<interprocess_mutex>  lock1(test_anonymous_ptr->mutex_1_);
  std::this_thread::sleep_for(std::chrono::seconds(5));
  scoped_lock<interprocess_mutex>  lock2(test_anonymous_ptr->mutex_2_);

  std::cout << "end " << std::endl;

  while (1)
  {
    /* code */
    std::this_thread::yield();
  }
}

TEST(test_boost, test_deadlock_2)
{
  using namespace BI;
  shared_memory_object  shm(BI::open_or_create, "test_mutex", read_write);
  shm.truncate(sizeof(test_anonymous));
  mapped_region region(shm, read_write);
  auto addr = region.get_address();
  auto test_anonymous_ptr = static_cast<test_anonymous*>(addr);

  scoped_lock<interprocess_mutex>  lock2(test_anonymous_ptr->mutex_2_);
  std::this_thread::sleep_for(std::chrono::seconds(5));
  scoped_lock<interprocess_mutex>  lock1(test_anonymous_ptr->mutex_1_);

  std::cout << "end " << std::endl;
  while (1)
  {
    std::this_thread::yield();
  }
}


TEST(test_boost, test_lock_recurse)
{
  using namespace BI;
  shared_memory_object  shm(BI::open_or_create, "test_mutex_recurse", read_write);
  shm.truncate(sizeof(test_anonymous));
  mapped_region region(shm, read_write);
  auto addr = region.get_address();
  auto test_anonymous_ptr = new (addr) test_anonymous;

  scoped_lock<interprocess_mutex>  lock2(test_anonymous_ptr->mutex_2_);
  // scoped_lock<interprocess_mutex>  lock1(test_anonymous_ptr->mutex_2_);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  std::cout << "end " << std::endl;
  while (1)
  {
    std::this_thread::yield();
  }
}

TEST(test_boost, test_lock_release_twice)
{
  using namespace BI;
  shared_memory_object  shm(BI::open_or_create, "test_mutex_release", read_write);
  shm.truncate(sizeof(test_anonymous));
  mapped_region region(shm, read_write);
  auto addr = region.get_address();
  auto test_anonymous_ptr = new (addr) test_anonymous;

  scoped_lock<interprocess_mutex>  lock2(test_anonymous_ptr->mutex_2_);
  // scoped_lock<interprocess_mutex>  lock1(test_anonymous_ptr->mutex_2_);
  lock2.unlock();
  lock2.unlock();
  std::this_thread::sleep_for(std::chrono::seconds(1));

  std::cout << "end " << std::endl;
  while (1)
  {
    std::this_thread::yield();
  }
}
