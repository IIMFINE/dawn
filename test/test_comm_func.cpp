#include <chrono>
#include <future>
#include <thread>

#include "common/baseOperator.h"
#include "common/heap.h"
#include "common/memoryPool.h"
#include "common/threadPool.h"
#include "gtest/gtest.h"
#include "test_helper.h"

using namespace dawn::test;

TEST(test_dawn, test_thread_pool)
{
    using namespace dawn;
    {
        threadPool pool;
        pool.init();
        pool.runAllThreads();
        auto test_func = []() { std::cout << "test_func" << std::endl; };
        pool.pushWorkQueue(test_func);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "end" << std::endl;
}

TEST(test_dawn, alloca_mem_small)
{
    using namespace dawn;
    {
        auto p = allocMem(128);
        freeMem(p);
    }
}

TEST(test_dawn, alloca_mem_big)
{
    using namespace dawn;
    uint32_t loop_time = 0xfffff;

    for (uint32_t j = 1; j < loop_time; j++)
    {
        for (int i = 1; i <= 2048; i++)
        {
            auto p1 = allocMem(i);
            auto p2 = allocMem(i);
            auto p3 = allocMem(i);
            freeMem(p1);
            freeMem(p2);
            freeMem(p3);
        }
    }
}

TEST(test_dawn, dawn_multiple_thread_memory_pool_benchmark)
{
    using namespace dawn;
    volatile bool runFlag = false;
    uint32_t loop_time = 0xffff;
    bool exitFlag = false;
    allocMem(1);

    auto func = [&]()
    {
        auto loop_time_2 = loop_time;
        uint64_t total_time_span = 0;
        while (runFlag == false)
        {
            std::this_thread::yield();
        }
        for (uint32_t i = 1; i < loop_time; i++)
        {
            {
                cyclesCounter counter("dawn_alloc");
                for (int j = 1900; j < 2048; j++)
                {
                    auto p1 = allocMem(j);
                    auto p2 = allocMem(j);
                    freeMem(p1);
                    freeMem(p2);
                }
            }
            total_time_span += cyclesCounter::getTimeSpan("dawn_alloc");
        }
        uint64_t avg_time_span = total_time_span / loop_time_2;
        std::cout << "dawn memory pool spend time " << avg_time_span << " cycles" << std::endl;
        LOG_INFO("dawn memory pool spend time {} cycles", avg_time_span);
        exitFlag = true;
    };
    std::thread(func).detach();
    std::thread(func).detach();
    std::thread(func).detach();
    std::thread(func).detach();
    runFlag = true;
    while (exitFlag == false)
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

TEST(test_dawn, memory_pool_decWaterMark_func)
{
    using namespace dawn;
    uint32_t loop_time = 0xfffffff;
    unsigned long total_time_span = 0;
    auto& memoryPool = getThreadLocalPool();
    for (uint32_t i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("adjustWaterMark");
            memoryPool.decWaterMark(0);
        }
        total_time_span += cyclesCounter::getTimeSpan("adjustWaterMark");
    }
    LOG_INFO("dawn decWaterMark func spend time {} cycles", total_time_span / loop_time);

    std::cout << "dawn decWaterMark func spend time " << total_time_span / loop_time << " cycles" << std::endl;
}

TEST(test_dawn, memory_pool_getMemBlockFromQueue_func)
{
    using namespace dawn;
    uint32_t loop_time = 0xfffffff;
    unsigned long total_time_span = 0;
    auto& memoryPool = getThreadLocalPool();
    void* test_p = nullptr;
    for (uint32_t i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("getMemBlockFromQueue");
            test_p = memoryPool.getMemBlockFromQueue(0);
        }
        freeMem(test_p);
        total_time_span += cyclesCounter::getTimeSpan("getMemBlockFromQueue");
    }

    LOG_INFO("dawn getMemBlockFromQueue func spend time {} cycles", total_time_span / loop_time);
    std::cout << "dawn getMemBlockFromQueue func spend time " << total_time_span / loop_time << " cycles" << std::endl;
}

TEST(test_dawn, memory_pool_TL_allocMem_func)
{
    using namespace dawn;
    uint32_t loop_time = 0xfffffff;
    unsigned long total_time_span = 0;
    auto& memoryPool = getThreadLocalPool();
    void* test_p = nullptr;
    for (uint32_t i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("TL_allocMem");
            test_p = memoryPool.TL_allocMem(127);
        }
        freeMem(test_p);
        total_time_span += cyclesCounter::getTimeSpan("TL_allocMem");
    }

    LOG_INFO("dawn TL_allocMem func spend time {} cycles", total_time_span / loop_time);
    std::cout << "dawn TL_allocMem func spend time " << total_time_span / loop_time << " cycles" << std::endl;
}

TEST(test_dawn, benchmark_GET_MEM_QUEUE_MASK)
{
    using namespace dawn;
    uint32_t loop_time = 0xffffffff;
    unsigned long total_time_span = 0;
    for (uint32_t i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("GET_MEM_QUEUE_MASK");
            int a = GET_MEM_QUEUE_MASK(127);
            (void)a;
        }
        total_time_span += cyclesCounter::getTimeSpan("GET_MEM_QUEUE_MASK");
    }

    LOG_INFO("dawn GET_MEM_QUEUE_MASK func spend time {} cycles", total_time_span / loop_time);
    std::cout << "dawn GET_MEM_QUEUE_MASK func spend time " << total_time_span / loop_time << " cycles" << std::endl;
}

TEST(test_dawn, dawn_single_thread_memory_pool_benchmark)
{
    using namespace dawn;
    uint32_t loop_time = 0xfffffff;
    allocMem(1);

    unsigned long total_time_span = 0;
    void* p = nullptr;
    for (uint32_t i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("dawn_alloc");
            p = allocMem(128);
        }
        freeMem(p);
        total_time_span += cyclesCounter::getTimeSpan("dawn_alloc");
    }
    LOG_INFO("dawn memory pool spend time {} cycles", total_time_span / loop_time);
    std::cout << "dawn memory pool spend time " << total_time_span / loop_time << " cycles" << std::endl;
}

/// @note Need to set CMAKE_BUILD_TYPE is Debug, otherwise malloc step will be
/// optimized.
TEST(test_dawn, malloc_func)
{
    using namespace dawn;
    uint32_t loop_time = 0xfffffff;
    unsigned long total_time_span = 0;
    void* test_p = nullptr;
    for (uint32_t i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("malloc");
            test_p = malloc(128);
        }
        *(int*)test_p = 0;
        free(test_p);
        total_time_span += cyclesCounter::getTimeSpan("malloc");
    }
    LOG_INFO("dawn malloc spend time {} cycles", total_time_span / loop_time);
    std::cout << "dawn malloc spend time " << total_time_span / loop_time << " cycles" << std::endl;
}

TEST(test_dawn, malloc_multiple_thread_benchmark)
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
                timeCounter counter("malloc");
                auto p = malloc(128);
                free(p);
            }
            total_time_span += timeCounter::getTimeSpan("malloc");
        }
        LOG_INFO("dawn malloc spend time {} ns", total_time_span / loop_time);
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

TEST(test_dawn, find_bit_position_benchmark)
{
    using namespace dawn;
    using namespace std::chrono;
    unsigned long total_time_span = 0;
    unsigned int loop_time = 0xfffffff;
    for (unsigned int i = 0; i < loop_time; i++)
    {
        {
            // auto num = rand() % 0x800;
            auto num = 128;
            cyclesCounter a("hello3");
            getTopBitPosition(num);
        }
        total_time_span += cyclesCounter::getTimeSpan("hello3");
    }

    std::cout << "loop find bit position spend " << total_time_span / loop_time << " cycles" << std::endl;

    LOG_INFO("loop find bit position spend {} cycles", total_time_span / loop_time);

    total_time_span = 0;
    for (unsigned int i = 0; i < loop_time; i++)
    {
        {
            // auto num = rand() % 0x800;
            auto num = 128;
            cyclesCounter a("hello2");
            getTopBitPosition_2(num);
        }
        total_time_span += cyclesCounter::getTimeSpan("hello2");
    }
    LOG_INFO("judgement find bit position spend {} cycles", total_time_span / loop_time);
    std::cout << "judgement find bit position spend " << total_time_span / loop_time << " cycles" << std::endl;
}

TEST(test_dawn, test_memory_benchmark)
{
    using namespace dawn;
    using namespace std::chrono;
    unsigned long total_time_span = 0;
    int loop_time = 0xfffffff;
    allocMem(1);
    void* test_p = nullptr;
    for (int i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("test1");
            test_p = allocMem(128);
        }
        total_time_span += cyclesCounter::getTimeSpan("test1");
        freeMem(test_p);
    }

    LOG_INFO("mempool spend average time {} cycles", total_time_span / loop_time);
    std::cout << "mempool spend average time " << total_time_span / loop_time << " cycles" << std::endl;
    total_time_span = 0;
    for (int i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("test2");
            test_p = malloc(128);
        }
        total_time_span += cyclesCounter::getTimeSpan("test2");
        free(test_p);
    }
    LOG_INFO("malloc spend average time {} cycles", total_time_span / loop_time);
    std::cout << "malloc spend average time " << total_time_span / loop_time << " cycles" << std::endl;
}
