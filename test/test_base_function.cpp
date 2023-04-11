#include <iostream>
#include <type_traits>
#include <vector>
#include <string>
#include <tuple>
#include <list>
#include <cstdint>
#include <inttypes.h>
#include <exception>
#include <functional>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <sched.h>
#include "threadPool.h"
#include "type.h"
#include "memoryPool.h"
#include <type_traits>
#include <string.h>
#include <vector>
#include "baseOperator.h"
#include "hazardPointer.h"
#include "setLogger.h"
#include <chrono>

int main()
{
  LOG_DEBUG("Hello world");
#if 0
    int test_time = 10;
    dawn::memPoolInit();
    auto testThread1 = [&](){
        auto time_left = std::chrono::nanoseconds(std::chrono::seconds(test_time));
        auto start = std::chrono::steady_clock::now();
        dawn::memoryNode_t *tempNode = nullptr;
        while(1)
        {
            for(int i = 1; i < 1000; i++)
            {
                tempNode = dawn::lowLevelAllocMem(i);
                if(tempNode == nullptr)
                {
                    LOG_INFO("cant alloc mem size is {}", i );
                    continue;
                }
                else
                {
                    LOG_DEBUG("alloc successfully mem size is {}", i );
                }
                if (dawn::lowLevelFreeMem(tempNode) == false)
                {
                    LOG_INFO("cant free mem ");
                }
            }
            auto now = std::chrono::steady_clock::now();
            auto elapse_time = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start);
            if(elapse_time > time_left)
            {
                return;
            }
        }
    };
    for(int i = 0; i< 200; i++)
    {
        std::thread a(testThread1);
        a.detach();
    }
    std::this_thread::sleep_for(std::chrono::seconds(test_time + 2));
    return 0;
#endif

#if 0
    std::atomic<test_mem*> head(new test_mem);
    head.load()->next = new test_mem;
    head.load()->next->next = nullptr;
    auto a = [&](){
        //pop item
        int64_t times = 0;
        while (1)
        {
            /* code */
            auto temp_node = head.load();
            while(1)
            {
                temp_node = head.load();
                if(temp_node != nullptr)
                {
                    if(head.compare_exchange_weak(temp_node, temp_node->next))
                    {
                        times++;
                        break;
                    }
                    else
                    {
                        LOG_DEBUG("alloc failed");
                        continue;
                    }
                }
            }
            if(times%10000)
            {
                LOG_WARN("alloc");
            }
            //push item
            while (1)
            {
                /* code */
                temp_node->next = head.load();
                if(head.compare_exchange_weak(temp_node->next, temp_node))
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
        }
    };
    for(int i = 0; i < 10; i++)
    {
        std::thread(a).detach();
    }
    while (1)
    {
        /* code */
        std::this_thread::yield();
    }

#endif

#if 0
    std::atomic<int> a(0);
    auto b = [&](){
        for(int i = 0; i < 100; i++)
        {
            // a.fetch_add(1, std::memory_order_relaxed);
            int c = a.load(std::memory_order_relaxed);
            do
            {
                /* code */
                c = a.load(std::memory_order_relaxed);
            } while (!a.compare_exchange_weak(c, c + 1, std::memory_order_release, std::memory_order_relaxed));
            
        }
    };
    for(int i = 0; i < 15; i++)
    {
        std::thread(b).detach();
    }
    sleep(5);
#endif

#if 0
    using namespace dawn;
    auto manager_ins = singleton<threadPoolManager>::getInstance();
    int a = 0;
    auto f1 = [&a](){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        a++;
        LOG_INFO("a {}", a);
        auto f2 = [&a](){
            if(a <= 10)
            {
                LOG_INFO("hello world a {}", a);
            }
        };
        return f2;
    };
    manager_ins->createThreadPool(2, f1);
    manager_ins->threadPoolExecute();
    std::this_thread::sleep_for(std::chrono::seconds(11));
    manager_ins->threadPoolDestory();
#endif
}
