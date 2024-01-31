#include "test_helper.h"
#include "common/threadPool.h"
#include "common/memoryPool.h"
#include "common/baseOperator.h"
#include "common/heap.h"

#include "gtest/gtest.h"

TEST(test_heap, test_min_heap)
{
  using namespace dawn;
  MinHeap<int, void*> heap;
  for (int j = 0; j < 3; j++)
  {
    heap.push({ 5, nullptr });
    heap.push({ 2, nullptr });
    heap.push({ 3, nullptr });
    heap.push({ 3, nullptr });
    heap.push({ 5, nullptr });
    heap.push({ 1, nullptr });
    heap.push({ 4, nullptr });
    heap.push({ 6, nullptr });
    heap.push({ 7, nullptr });

    auto size = heap.size();
    for (int i = 0; i < size; i++)
    {
      auto top = heap.top();
      auto [key, value] = *(top.value());
      std::cout << key << " ";
      heap.pop();
    }
    std::cout << std::endl;
    std::cout << "-----------------" << std::endl;
  }

  heap.push({ 5, nullptr });
  heap.push({ 2, nullptr });
  heap.push({ 3, nullptr });
  heap.push({ 3, nullptr });
  heap.push({ 5, nullptr });
  heap.push({ 1, nullptr });
  heap.push({ 4, nullptr });
  heap.push({ 6, nullptr });
  heap.push({ 7, nullptr });

  auto heap1 = heap;
  auto heap2 = std::move(heap1);
  std::vector<int> output;
  std::vector<int> baseline = { 1, 2, 3, 3, 4, 5, 5, 6, 7 };
  for (int i = 0; i < 9; i++)
  {
    auto top = heap2.top();
    auto [key, value] = *(top.value());
    std::cout << key << " ";
    output.push_back(key);
    heap2.pop();
  }
  std::cout << std::endl;
  EXPECT_EQ(output, baseline);
}

TEST(test_heap, test_max_heap)
{
  using namespace dawn;
  MaxHeap<int, void*> heap;
  for (int j = 0; j < 3; j++)
  {
    heap.push({ 5, nullptr });
    heap.push({ 2, nullptr });
    heap.push({ 3, nullptr });
    heap.push({ 3, nullptr });
    heap.push({ 5, nullptr });
    heap.push({ 1, nullptr });
    heap.push({ 4, nullptr });
    heap.push({ 6, nullptr });
    heap.push({ 7, nullptr });

    auto size = heap.size();
    for (int i = 0; i < size; i++)
    {
      auto top = heap.top();
      auto [key, value] = *(top.value());
      std::cout << key << " ";
      heap.pop();
    }
    std::cout << std::endl;
    std::cout << "-----------------" << std::endl;
  }

  heap.push({ 5, nullptr });
  heap.push({ 2, nullptr });
  heap.push({ 3, nullptr });
  heap.push({ 3, nullptr });
  heap.push({ 5, nullptr });
  heap.push({ 1, nullptr });
  heap.push({ 4, nullptr });
  heap.push({ 6, nullptr });
  heap.push({ 7, nullptr });

  auto heap1 = heap;
  auto heap2 = std::move(heap1);
  std::vector<int>  output;
  std::vector<int>  baseline{ 7, 6, 5, 5, 4, 3, 3, 2, 1 };
  for (int i = 0; i < 9; i++)
  {
    auto top = heap2.top();
    auto [key, value] = *(top.value());
    std::cout << key << " ";
    output.push_back(key);
    heap2.pop();
  }
  std::cout << std::endl;
  EXPECT_EQ(output, baseline);
}


TEST(test_heap, test_min_heap_node)
{
  using namespace dawn;

  MinHeap<int, void*> heap;
  for (int j = 0; j < 1; j++)
  {
    auto it = heap.pushAndGetNode({ 1, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 0);
    heap.push({ 0, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 1);
    heap.push({ 3, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 1);
    heap.push({ 3, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 1);
    heap.push({ 6, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 1);

    heap.push({ 5, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 1);
    heap.push({ 1, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 1);

    heap.push({ 0, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 3);

    heap.push({ 6, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 3);

    heap.push({ 7, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 3);

    auto size = heap.size();
    for (int i = 0; i < size; i++)
    {
      auto top = heap.top();
      auto [key, value] = *(top.value());
      std::cout << key << " ";
      heap.pop();
    }
    std::cout << std::endl;
    std::cout << "-----------------" << std::endl;

  }
}

TEST(test_heap, test_max_heap_node)
{
  using namespace dawn;

  MaxHeap<int, void*> heap;
  for (int j = 0; j < 1; j++)
  {
    auto it = heap.pushAndGetNode({ 1, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 0);
    heap.push({ 0, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 0);
    heap.push({ 3, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 2);
    heap.push({ 3, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 2);
    heap.push({ 6, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 2);

    heap.push({ 5, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 5);
    heap.push({ 1, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 5);

    heap.push({ 4, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 5);

    heap.push({ 6, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 5);

    heap.push({ 7, nullptr });
    EXPECT_EQ((it.node_info_->heap_position_), 5);

    auto size = heap.size();
    for (int i = 0; i < size; i++)
    {
      auto top = heap.top();
      auto [key, value] = *(top.value());
      std::cout << key << " ";
      heap.pop();
    }
    std::cout << std::endl;
    std::cout << "-----------------" << std::endl;
  }
}

TEST(test_heap, test_heap_node)
{
  using namespace dawn;
  MinixHeap<int, void*>::HeapNode node1{ 0,std::pair<int, void*>{1, nullptr} };
  MinixHeap<int, void*>::HeapNode node2{ 1,std::pair<int, void*>{1, nullptr} };
  MinixHeap<int, void*>::HeapNode node3{ 2,std::pair<int, void*>{1, nullptr} };

  auto previous_node1_nodeinfo_address = reinterpret_cast<std::uintptr_t>(node1.node_info_.get());
  auto previous_node2_nodeinfo_address = reinterpret_cast<std::uintptr_t>(node2.node_info_.get());
  auto previous_node3_nodeinfo_address = reinterpret_cast<std::uintptr_t>(node3.node_info_.get());

  MinixHeap<int, void*>::swap(node1, node2);

  auto node1_nodeinfo_address = reinterpret_cast<std::uintptr_t>(node1.node_info_.get());
  auto node2_nodeinfo_address = reinterpret_cast<std::uintptr_t>(node2.node_info_.get());

  EXPECT_EQ(node1_nodeinfo_address, previous_node2_nodeinfo_address);
  EXPECT_EQ(node2_nodeinfo_address, previous_node1_nodeinfo_address);

  EXPECT_EQ(node2.node_info_->heap_position_, 1);
  EXPECT_EQ(node1.node_info_->heap_position_, 0);

  MinixHeap<int, void*>::swap(node2, node3);

  auto node3_nodeinfo_address = reinterpret_cast<std::uintptr_t>(node3.node_info_.get());

  EXPECT_EQ(node3_nodeinfo_address, previous_node1_nodeinfo_address);

  EXPECT_EQ(node3.node_info_->heap_position_, 2);
  EXPECT_EQ(node2.node_info_->heap_position_, 1);

  MinixHeap<int, void*>::swap(node1, node3);
  EXPECT_EQ(node1.node_info_->heap_position_, 0);
  EXPECT_EQ(node3.node_info_->heap_position_, 2);
}

TEST(test_heap, test_erase_heap_node)
{
  using namespace dawn;

  MaxHeap<int, void*> heap;
  std::vector<int>  result;
  for (int j = 0; j < 1; j++)
  {
    auto it1 = heap.pushAndGetNode({ 10, nullptr });
    auto it2 = heap.pushAndGetNode({ 3, nullptr });
    heap.push({ 3, nullptr });
    heap.push({ 6, nullptr });
    heap.push({ 5, nullptr });
    heap.push({ 1, nullptr });
    heap.push({ 4, nullptr });
    heap.push({ 6, nullptr });
    heap.push({ 7, nullptr });

    heap.erase(it1);
    heap.erase(it2);
    for (int i = 0, size = heap.size(); i < size; i++)
    {
      auto top = heap.top();
      auto [key, value] = *(top.value());
      std::cout << key << " ";
      result.push_back(key);
      heap.pop();
    }

    std::vector<int> baseline{ 7, 6, 6, 5, 4, 3, 1 };
    EXPECT_EQ(result, baseline);
    std::cout << std::endl;
    std::cout << "-----------------" << std::endl;
  }
}

TEST(test_heap, test_heapify_heap_node)
{
  using namespace dawn;
  MaxHeap<int, void*> heap;
  std::vector<int>  result;

  auto it1 = heap.pushAndGetNode({ 10, nullptr });
  auto it2 = heap.pushAndGetNode({ 3, nullptr });
  heap.push({ 3, nullptr });
  heap.push({ 6, nullptr });
  heap.push({ 5, nullptr });
  heap.push({ 1, nullptr });
  heap.push({ 4, nullptr });
  heap.push({ 6, nullptr });
  heap.push({ 7, nullptr });

  it1.modifyKey(0);
  heap.heapify(it1);

  EXPECT_EQ(it1.getNodePosition(), heap.size() - 1);


  std::vector<int> baseline{ 7, 6, 6, 5, 4, 3, 3, 1,0 };
  for (int i = 0, size = heap.size(); i < size; i++)
  {
    auto top = heap.top();
    auto [key, value] = *(top.value());
    std::cout << key << " ";
    result.push_back(key);
    heap.pop();
  }

  EXPECT_EQ(result, baseline);

}

