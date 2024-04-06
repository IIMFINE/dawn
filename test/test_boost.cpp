#include <signal.h>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_recursive_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/sync/named_sharable_mutex.hpp>
#include <boost/interprocess/sync/named_upgradable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>
#include <future>
#include <iostream>
#include <thread>

#include "gtest/gtest.h"
#include "test_helper.h"

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
        for (int i = 0; i < 1; i++)
        {
            auto tmp_name = name + std::to_string(i);

            shared_memory_object segment(open_or_create, tmp_name.c_str(), read_write);
            segment.truncate(256);
            mapped_region region(segment, read_write);
            object_vec.push_back(std::move(segment));
            auto mem = reinterpret_cast<char *>(region.get_address());
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
        char *mem;
        shared_memory_object segment(open_or_create, "hello_world_dawn0", read_write);
        segment.truncate(256);
        mapped_region region(segment, read_write);
        mem = reinterpret_cast<char *>(region.get_address());
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
    // std::cout << BOOST_VERSION << std::endl;
    std::cout << "Using Boost "
              << BOOST_VERSION / 100000 << "."      // major version
              << BOOST_VERSION / 100 % 1000 << "."  // minor version
              << BOOST_VERSION % 100                // patch level
              << std::endl;
}

TEST(test_boost, test_upgradable_mutex_shared1)
{
    using namespace BI;
    named_upgradable_mutex upgradable_mut(open_or_create, "hello_world_dawn");
    std::cout << "try to lock sharable" << std::endl;
    upgradable_mut.lock_sharable();
    std::cout << "end " << std::endl;
    std::cout << "try to lock sharable" << std::endl;
    upgradable_mut.lock_sharable();
    std::cout << "end " << std::endl;
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
    std::cout << "try to lock shared" << std::endl;
    upgradable_mut.lock_sharable();
    std::cout << "end shared" << std::endl;

    std::cout << "try to lock upgrade" << std::endl;
    upgradable_mut.lock_upgradable();
    std::cout << "end " << std::endl;

    while (1)
    {
        std::this_thread::yield();
    }
}

TEST(test_boost, test_sharable_lock)
{
    using namespace BI;
    named_upgradable_mutex upgradable_mut(open_or_create, "hello_world_dawn");

    std::cout << "try to lock sharable" << std::endl;
    upgradable_mut.lock_sharable();
    std::cout << "lock" << std::endl;
    while (1)
    {
        std::this_thread::yield();
    }
}

TEST(test_boost, test_sharable_lock_multi_thread)
{
    using namespace BI;
    named_upgradable_mutex upgradable_mut(open_or_create, "hello_world_dawn");
    auto func = [&]()
    {
        std::cout << "try to lock sharable" << std::endl;
        upgradable_mut.lock_sharable();
        std::cout << "lock" << std::endl;
        while (1)
        {
            std::this_thread::yield();
        }
    };

    auto future1 = std::async(std::launch::async, func);
    auto future2 = std::async(std::launch::async, func);
    auto future3 = std::async(std::launch::async, func);
    while (1)
    {
        /* code */
        std::this_thread::yield();
    }
}

TEST(test_boost, test_shared_mutex_shared_lock)
{
    using namespace BI;
    named_sharable_mutex mut(open_or_create, "hello_world_dawn");
    std::cout << "try to lock sharable" << std::endl;
    mut.lock_sharable();
    std::cout << "lock" << std::endl;
    std::cout << "try to lock sharable" << std::endl;
    mut.lock_sharable();
    std::cout << "lock" << std::endl;
    std::cout << "try to lock sharable" << std::endl;
    mut.lock_sharable();
    std::cout << "lock" << std::endl;
    mut.unlock_sharable();
    std::cout << "unlock" << std::endl;
    while (1)
    {
        /* code */
        std::this_thread::yield();
    }
}

TEST(test_boost, test_shared_mutex_lock)
{
    using namespace BI;
    named_sharable_mutex mut(open_or_create, "hello_world_dawn");
    std::cout << "try to lock " << std::endl;
    mut.lock();
    std::cout << "lock" << std::endl;
    std::cout << "try to lock " << std::endl;
    while (1)
    {
        /* code */
        std::this_thread::yield();
    }
}

TEST(test_boost, test_shared_mutex_shared_lock_recursive)
{
    using namespace BI;
    named_sharable_mutex mut(open_or_create, "hello_world_dawn");
    uint32_t time = 1000;
    for (uint32_t i = 0; i < time; i++)
    {
        std::cout << "try to lock " << std::endl;
        sharable_lock<named_sharable_mutex> lock1(mut);
        std::cout << "lock" << std::endl;
        sharable_lock<named_sharable_mutex> lock2;
        // lock2 = std::move(lock1);
        // lock2.swap(lock1);
        std::cout << "try to lock " << std::endl;
        sharable_lock<named_sharable_mutex> lock3(mut);
        std::cout << "lock" << std::endl;
        // lock2 = std::move(lock3);
        // lock2.swap(lock3);
        // lock2.unlock();
        std::cout << "unlock" << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "end" << std::endl;
    while (1)
    {
        /* code */
        std::this_thread::yield();
    }
}

TEST(test_boost, test_shared_mutex_scoped_lock)
{
    using namespace BI;
    named_sharable_mutex mut(open_or_create, "hello_world_dawn");
    uint32_t time = 1000;
    for (uint32_t i = 0; i < time; i++)
    {
        std::cout << "try to lock " << std::endl;
        scoped_lock<named_sharable_mutex> lock1(mut);
        std::cout << "lock" << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "end" << std::endl;
    while (1)
    {
        /* code */
        std::this_thread::yield();
    }
}

TEST(test_boost, test_upgradable_lock)
{
    using namespace BI;
    named_upgradable_mutex name_mut(open_or_create, "hello_world_dawn");
    std::cout << "try to lock upgradable" << std::endl;

    upgradable_lock<named_upgradable_mutex> lock1(name_mut);

    std::cout << "upgradable lock 1" << std::endl;

    upgradable_lock<named_upgradable_mutex> lock2(name_mut);

    std::cout << "upgradable lock 2" << std::endl;

    while (1)
    {
        std::this_thread::yield();
    }
}

TEST(test_boost, test_scoped_lock_upgradable)
{
    using namespace BI;
    named_upgradable_mutex name_mut(open_or_create, "hello_world_dawn");
    std::cout << "try to lock scoped" << std::endl;

    scoped_lock<named_upgradable_mutex> lock(name_mut);

    std::cout << "lock" << std::endl;
    while (1)
    {
        std::this_thread::yield();
    }
}

struct lock_guard
{
    lock_guard(BI::named_mutex &&mutex) : lock(mutex)
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
        lock_guard lock_g(std::move(mutex));
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

TEST(test_boost, test_scoped_lock_multi_thread)
{
    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, "test_dawn_scoped");
    auto func = [&]()
    {
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex, boost::interprocess::defer_lock);
        lock.lock();
        std::cout << "**lock** " << std::endl;
        int i = 10;
        while (i--)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    };

    auto future1 = std::async(std::launch::async, func);
    auto future2 = std::async(std::launch::async, func);
    auto future3 = std::async(std::launch::async, func);
    auto future4 = std::async(std::launch::async, func);
    int i = 10;
    while (i--)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

TEST(test_boost, test_named_recursive_mutex)
{
    boost::interprocess::named_recursive_mutex mutex(boost::interprocess::open_or_create, "test_recursive_mutex");
    auto func = [&]()
    {
        // boost::interprocess::scoped_lock<boost::interprocess::named_recursive_mutex> lock(mutex);
        // std::cout << "**lock** " << std::endl;
        // boost::interprocess::scoped_lock<boost::interprocess::named_recursive_mutex> lock2(mutex);
        // std::cout << "**lock2** " << std::endl;
        // boost::interprocess::scoped_lock<boost::interprocess::named_recursive_mutex> lock3(mutex);
        // std::cout << "**lock3** " << std::endl;
        // lock3.unlock();
        mutex.lock();
        std::cout << "**lock**" << std::endl;
        mutex.lock();
        std::cout << "**lock2**" << std::endl;
        mutex.lock();
        std::cout << "**lock3**" << std::endl;
        int i = 10;
        while (i--)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    };

    auto future1 = std::async(std::launch::async, func);
    auto future2 = std::async(std::launch::async, func);
    auto future3 = std::async(std::launch::async, func);
    auto future4 = std::async(std::launch::async, func);
    int i = 10;
    while (i--)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

TEST(test_boost, test_named_recursive_mutex2)
{
    boost::interprocess::named_recursive_mutex mutex(boost::interprocess::open_or_create, "test_recursive_mutex");
    auto func = [&]()
    {
        boost::interprocess::scoped_lock<boost::interprocess::named_recursive_mutex> lock(mutex, boost::interprocess::defer_lock);
        lock.lock();
        std::cout << "**lock1** " << std::endl;
        lock.lock();
        std::cout << "**lock2** " << std::endl;
        lock.lock();
        std::cout << "**lock3** " << std::endl;
        int i = 10;
        while (i--)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    };

    auto future1 = std::async(std::launch::async, func);
    auto future2 = std::async(std::launch::async, func);
    auto future3 = std::async(std::launch::async, func);
    auto future4 = std::async(std::launch::async, func);
    int i = 10;
    while (i--)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

TEST(test_boost, test_mutex_remove)
{
    if (boost::interprocess::named_mutex::remove("hello_world_dawn_scoped"))
    {
        std::cout << "remove" << std::endl;
    }
}

struct test_anonymous
{
    std::mutex mutex_;
    BI::interprocess_mutex mutex_1_;
    BI::interprocess_mutex mutex_2_;
};

TEST(test_boost, test_deadlock_1)
{
    using namespace BI;
    shared_memory_object shm(BI::open_or_create, "test_mutex", read_write);
    shm.truncate(sizeof(test_anonymous));
    mapped_region region(shm, read_write);
    auto addr = region.get_address();
    auto test_anonymous_ptr = new (addr) test_anonymous;

    scoped_lock<interprocess_mutex> lock1(test_anonymous_ptr->mutex_1_);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    scoped_lock<interprocess_mutex> lock2(test_anonymous_ptr->mutex_2_);

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
    shared_memory_object shm(BI::open_or_create, "test_mutex", read_write);
    shm.truncate(sizeof(test_anonymous));
    mapped_region region(shm, read_write);
    auto addr = region.get_address();
    auto test_anonymous_ptr = static_cast<test_anonymous *>(addr);

    scoped_lock<interprocess_mutex> lock2(test_anonymous_ptr->mutex_2_);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    scoped_lock<interprocess_mutex> lock1(test_anonymous_ptr->mutex_1_);

    std::cout << "end " << std::endl;
    while (1)
    {
        std::this_thread::yield();
    }
}

TEST(test_boost, test_lock_recurse)
{
    using namespace BI;
    shared_memory_object shm(BI::open_or_create, "test_mutex_recurse", read_write);
    shm.truncate(sizeof(test_anonymous));
    mapped_region region(shm, read_write);
    auto addr = region.get_address();
    auto test_anonymous_ptr = new (addr) test_anonymous;

    scoped_lock<interprocess_mutex> lock2(test_anonymous_ptr->mutex_2_);
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
    shared_memory_object shm(BI::open_or_create, "test_mutex_release", read_write);
    shm.truncate(sizeof(test_anonymous));
    mapped_region region(shm, read_write);
    auto addr = region.get_address();
    auto test_anonymous_ptr = new (addr) test_anonymous;

    scoped_lock<interprocess_mutex> lock2(test_anonymous_ptr->mutex_2_);
    // scoped_lock<interprocess_mutex>  lock1(test_anonymous_ptr->mutex_2_);
    lock2.unlock();
    lock2.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "unlock twice " << std::endl;
    while (1)
    {
        std::this_thread::yield();
    }
}

TEST(test_boost, test_shared_lock_cost)
{
    using namespace BI;
    using namespace dawn::test;
    named_sharable_mutex mut(open_or_create, "hello_world_dawn");
    uint32_t total_time = 100000;
    for (uint32_t i = 0; i < total_time; ++i)
    {
        {
            cyclesCounter counter("lock_shared");
            sharable_lock<named_sharable_mutex> lock2(mut);
        }
        total_time += cyclesCounter::getTimeSpan("lock_shared");
    }
    std::cout << "total_time: " << total_time / total_time << std::endl;
}

/// @brief result: std::mutex can not work at interprocess
TEST(test_boost, test_std_lock_in_boost_shm)
{
    using namespace BI;
    shared_memory_object shm(BI::open_or_create, "test_shm", read_write);
    shm.truncate(sizeof(test_anonymous));
    mapped_region region(shm, read_write);
    auto addr = region.get_address();
    auto test_anonymous_ptr = new (addr) test_anonymous;

    std::cout << "wait to lock" << std::endl;
    std::lock_guard<std::mutex> lock(test_anonymous_ptr->mutex_);
    std::cout << "lock end" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    while (1)
    {
        std::this_thread::yield();
    }
}
