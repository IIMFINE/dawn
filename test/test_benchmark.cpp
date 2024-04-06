#include <x86intrin.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "common/baseOperator.h"
#include "gtest/gtest.h"
#include "test_helper.h"

using namespace dawn::test;

TEST(test_dawn, cycles_counter_correction)
{
    using namespace dawn;
    unsigned int loop_time = 0xfffffff;
    uint64_t total_time = 0;
    for (unsigned int i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("calculate");
        }
        total_time += cyclesCounter::getTimeSpan("calculate");
    }
    std::cout << "counter spend time " << total_time / loop_time << " cycles" << std::endl;
}

TEST(test_dawn, cycles_counter_correction_1)
{
    using namespace dawn;
    unsigned int loop_time = 0xfffffff;
    uint64_t total_time = 0;
    for (unsigned int i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("calculate");
            if (i > loop_time)
            {
            }
            else
            {
            }
        }
        total_time += cyclesCounter::getTimeSpan("calculate");
    }
    std::cout << "counter spend time " << total_time / loop_time << " cycles" << std::endl;
}

TEST(test_dawn, find_bit_map_benchmark)
{
    using namespace dawn;
    int number = 128;
    int offset = 7;
    int mask = (1 << offset) - 1;
    unsigned int loop_time = 0xfffffff;
    uint64_t total_time = 0;
    for (unsigned int i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("calculate");
            auto index = ((number + mask) & (~mask)) >> offset;
        }
        total_time += cyclesCounter::getTimeSpan("calculate");
    }
    std::cout << "calculate spend time " << total_time / loop_time << " cycles" << std::endl;
    total_time = 0;
    void* test_p = nullptr;
    for (unsigned int i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("calculate");
            getTopBitPosition(128);
        }
        total_time += cyclesCounter::getTimeSpan("calculate");
    }
    std::cout << "getTopBitPosition spend time " << total_time / loop_time << " cycles" << std::endl;
}

TEST(test_dawn, cpu_cycles)
{
    for (uint32_t i = 0; i < 0xfffff; i++)
    {
        auto cycles1 = __rdtsc();
        auto cycles2 = __rdtsc();
        std::cout << cycles2 - cycles1 << std::endl;
    }
}

TEST(test_dawn, test_func_jump)
{
    using namespace dawn;
    unsigned int loop_time = 0xfffffff;
    uint64_t total_time = 0;
    std::function<void(int)> test_func[2] = {
        [](int a)
        {
            if (a == 2)
            {
                std::cout << "hello world" << std::endl;
            }
            int b = 1;
        },
        [](int a)
        {
            if (a == 2)
            {
                std::cout << "hello world" << std::endl;
            }
        }};
    for (unsigned int i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("calculate");
            test_func[0](1);
        }
        total_time += cyclesCounter::getTimeSpan("calculate");
    }
    std::cout << "test_func_jump spend time " << total_time / loop_time << " cycles" << std::endl;

    total_time = 0;
    auto test_func_1 = [](int a)
    {
        if (a == 2)
        {
            std::cout << "hello world" << std::endl;
        }
        int b = 1;
    };
    for (unsigned int i = 0; i < loop_time; i++)
    {
        {
            cyclesCounter counter("calculate");
            if (i > loop_time)
            {
            }
            else
            {
                test_func_1(i < loop_time);
            }
        }
        total_time += cyclesCounter::getTimeSpan("calculate");
    }
    std::cout << "test_func_jump spend time " << total_time / loop_time << " cycles" << std::endl;
}
