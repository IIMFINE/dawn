#ifndef _TEST_HELPER_H_
#define _TEST_HELPER_H_
#include <x86intrin.h>

#include <chrono>
#include <map>

#include "common/setLogger.h"

namespace dawn
{
    namespace test
    {
        using namespace std::chrono;
        struct timeCounter final
        {
            timeCounter(std::string_view index)
            {
                timeIndex_ = index;
                startTime_ = high_resolution_clock::now();
            }

            ~timeCounter()
            {
                endTime_ = high_resolution_clock::now();
                timeCounter::timeSpan_[timeIndex_] = duration_cast<nanoseconds>(endTime_ - startTime_).count();
            }

            static unsigned long getTimeSpan(std::string_view index)
            {
                if (auto timeSpanIt = timeCounter::timeSpan_.find(static_cast<std::string>(index)); timeSpanIt == timeCounter::timeSpan_.end())
                {
                    throw std::runtime_error("dawn: time counter " + static_cast<std::string>(index) + " doesn't exist");
                }
                else
                {
                    return timeSpanIt->second;
                }
            }

            std::string timeIndex_;
            std::chrono::time_point<high_resolution_clock> startTime_;
            std::chrono::time_point<high_resolution_clock> endTime_;
            static std::map<std::string, unsigned int> timeSpan_;
        };

        struct cyclesCounter final
        {
            cyclesCounter(std::string_view index)
            {
                timeIndex_ = index;
                startTime_ = __rdtsc();
            }

            ~cyclesCounter()
            {
                endTime_ = __rdtsc();
                cyclesCounter::timeSpan_[timeIndex_] = endTime_ - startTime_;
            }

            static unsigned long getTimeSpan(std::string_view index)
            {
                if (auto timeSpanIt = cyclesCounter::timeSpan_.find(static_cast<std::string>(index)); timeSpanIt == cyclesCounter::timeSpan_.end())
                {
                    throw std::runtime_error("dawn: time counter " + static_cast<std::string>(index) + " doesn't exist");
                }
                else
                {
                    return timeSpanIt->second;
                }
            }

            std::string timeIndex_;
            uint64_t startTime_;
            uint64_t endTime_;
            static std::map<std::string, uint64_t> timeSpan_;
        };

        inline int getTopBitPosition_2(uint32_t number)
        {
            if (number == 0x1)
            {
                return 0;
            }
            else if (number < 0x3)
            {
                return 1;
            }
            else if (number < 0x5)
            {
                return 2;
            }
            else if (number < 0x9)
            {
                return 3;
            }
            else if (number < 0x11)
            {
                return 4;
            }
            else if (number < 0x21)
            {
                return 4;
            }
            else if (number < 0x41)
            {
                return 4;
            }
            else if (number < 0x81)
            {
                return 4;
            }
            else if (number < 0x101)
            {
                return 4;
            }
            else if (number < 0x201)
            {
                return 4;
            }
            else if (number < 0x401)
            {
                return 4;
            }
            else if (number < 0x801)
            {
                return 4;
            }
            return 5;
        }
    }  // namespace test
}  // namespace dawn
#endif
