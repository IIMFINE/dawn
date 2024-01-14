#include "test_helper.h"
#include "common/heap.h"
#include "common/timer.h"

#include "gtest/gtest.h"
#include <iostream>

struct overloadStruct
{
  overloadStruct() = default;
  ~overloadStruct() = default;
  bool operator<(const overloadStruct &other) const
  {
    std::cout << "operator<" << std::endl;
    if (v1_ < other.v1_)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  bool operator>(const overloadStruct &other) const
  {
    std::cout << "operator>" << std::endl;
    if (v1_ > other.v1_)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  uint32_t   v1_;
  uint32_t   v2_;
};

TEST(test_dawn, test_operator_overload_in_pointer)
{
  overloadStruct* a = new overloadStruct{ 1,2 };
  overloadStruct* b = new overloadStruct{ 2,3 };
  if ((*a) < (*b))
  {
    std::cout << "a < b" << std::endl;
  }
  else
  {
    std::cout << "a > b" << std::endl;
  }

}


TEST(test_dawn, test_heapify_with_operator_overload)
{
  using namespace dawn;
  std::vector<int> baseline = { 41,23,21,11,4,3,2,1,1 };
  overloadStruct a = { 1,2 };
  overloadStruct aa = { 1,2 };
  overloadStruct b = { 2,3 };
  overloadStruct c = { 3,4 };
  overloadStruct d = { 4,5 };
  overloadStruct a1 = { 11,2 };
  overloadStruct b1 = { 23,3 };
  overloadStruct c1 = { 21,4 };
  overloadStruct d1 = { 41,5 };

  MaxHeap<overloadStruct, void*> max_heap;
  auto it11 = max_heap.pushAndGetNode({ a1, nullptr });
  auto it31 = max_heap.pushAndGetNode({ c1,nullptr });
  auto it21 = max_heap.pushAndGetNode({ b1,nullptr });
  auto it41 = max_heap.pushAndGetNode({ d1,nullptr });
  auto it1 = max_heap.pushAndGetNode({ a, nullptr });
  auto ita1 = max_heap.pushAndGetNode({ aa, nullptr });
  auto it3 = max_heap.pushAndGetNode({ c,nullptr });
  auto it2 = max_heap.pushAndGetNode({ b,nullptr });
  auto it4 = max_heap.pushAndGetNode({ d,nullptr });
  std::vector<int> result;

  for (int i = 0, size = max_heap.size(); i < size; ++i)
  {
    result.push_back(max_heap.top().value()->first.v1_);
    max_heap.pop();
  }
  EXPECT_EQ(baseline, result);
  for (auto &it : result)
  {
    std::cout << it << std::endl;
  }
  std::cout << std::endl;
  std::cout << "-----------------------------" << std::endl;
}

TEST(test_dawn, dawn_timer_construct)
{
  using namespace dawn;
  EventTimer timer;

  int test_num = 100;
  //run 60 seconds
  int duration = 10;

  int base_add_result = test_num * duration;
  std::atomic<int> test_sum = 0;

  for (int i = 1; i < test_num; ++i)
  {
    timer.addEvent(
      [i, &test_sum]() {
        test_sum++;
        // std::cout << "hello world " << i << "  " << std::chrono::steady_clock::now().time_since_epoch().count() << std::endl;
      },
      std::chrono::seconds(1)
    );
  }

  // for (int i = 1; i < test_num; ++i)
  // {
  //   timer.addEvent(
  //     [i]() {
  //       std::cout << "hello world milliseconds " << i << "  " << std::chrono::steady_clock::now().time_since_epoch().count() << std::endl;
  //     },
  //     std::chrono::milliseconds(i)
  //   );
  // }

  std::this_thread::sleep_for(std::chrono::seconds(duration));

  EXPECT_EQ(test_sum.load(), base_add_result);
}


