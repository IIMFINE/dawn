#ifndef _HEAP_H_
#define _HEAP_H_
#include <vector>
#include <optional>
#include <memory>
#include <algorithm>
#include <iostream>
#include <shared_mutex>

#include "type.h"

///@todo Improve heap synchronization performance.

namespace dawn
{
#define _GET_PARENT_INDEX(index) ((index - 1) / 2)

  constexpr uint32_t INVALID_HEAP_POSITION = 0xFFFFFFFF;
  template<typename KEY_T, typename CONTENT_T>
  struct minixHeap
  {
    struct heapNode
    {
      struct nodeInfo : public std::enable_shared_from_this<nodeInfo>
      {
        nodeInfo() = default;
        nodeInfo(uint32_t position) :
          heapPosition_(position)
        {
        }
        nodeInfo(const nodeInfo&) = default;

        /// @brief This destruct function will be called once only when its last shared pointer expire.
        ///        So it need to set validInHeap_ to false manually.
        ~nodeInfo() = default;
        uint32_t  heapPosition_ = INVALID_HEAP_POSITION;
        bool      validInHeap_ = true;
      };

      heapNode();
      ~heapNode() = default;
      heapNode(uint32_t heapIndex, bool valid, std::pair<KEY_T, CONTENT_T>&& data_pair);
      heapNode(uint32_t heapIndex, std::pair<KEY_T, CONTENT_T>&& data_pair);
      heapNode(const heapNode &node);

      /// @brief Move constructor. Transfer nodeInfo_ pointer property to new node.
      ///        But not swap nodeInfo_ value.
      /// @param node Move from node.
      heapNode(heapNode &&node);
      heapNode &operator=(const heapNode &node);

      /// @brief Swap nodeInfo_ pointer property and nodeInfo_ value. Move dataPair_ to new node.
      /// @param node: Move from node.
      /// @return heapNode&: Reference of new node.
      heapNode &operator=(heapNode &&node);

      /// @brief Swap all node content to passed node.
      /// @param node passed node
      void swap(heapNode &node);
      bool validInHeap();

      /// @brief Swap nodeInfo_ with passed node.
      /// @param node passed node
      /// @return PROCESS_SUCCESS: true, PROCESS_FAILED: false
      bool swapInfo(heapNode &node);

      /// @brief Use smart pointer to keep latest heap position of node.
      std::shared_ptr<nodeInfo>                       nodeInfo_;
      std::shared_ptr<std::pair<KEY_T, CONTENT_T>>    dataPair_;
    };

    minixHeap() = default;
    virtual ~minixHeap() = default;

    static void swap(heapNode &node_a, heapNode &node_b);

    bool push(std::pair<KEY_T, CONTENT_T> &&dataPair);
    minixHeap::heapNode pushAndGetNode(std::pair<KEY_T, CONTENT_T> &&dataPair);
    std::optional<std::pair<KEY_T, CONTENT_T>> pop();
    std::optional<std::pair<KEY_T, CONTENT_T>> top();
    bool empty();
    int  size();
    bool erase(uint32_t heapIndex);
    bool erase(minixHeap::heapNode &node);

    /// @brief Heapify from parent index.
    /// @param parentIndex Given parent index.
    /// @return Return final position changed from parent index after heapify.
    virtual uint32_t heapify(uint32_t parentIndex) = 0;

    /// @brief Heapify from child index to its parent index.
    /// @param childIndex Given child index.
    /// @return Return final position changed from child index after heapify.
    virtual uint32_t heapifyFromChild(uint32_t childIndex) = 0;

    protected:
    uint32_t heapSize_ = 0;
    std::vector<typename minixHeap<KEY_T, CONTENT_T>::heapNode>   heap_;
    public:
    std::shared_mutex                                             heapMutex_;
  };

  template<typename KEY_T, typename CONTENT_T>
  struct maxHeap : public minixHeap<KEY_T, CONTENT_T>
  {
    maxHeap();
    ~maxHeap() = default;
    maxHeap(const maxHeap &heap);
    maxHeap(maxHeap &&heap);
    maxHeap &operator=(const maxHeap &heap);
    maxHeap &operator=(maxHeap &&heap);
    virtual uint32_t heapify(uint32_t parentIndex) override;
    virtual uint32_t heapifyFromChild(uint32_t childIndex) override;
  };

  template<typename KEY_T, typename CONTENT_T>
  struct minHeap : public minixHeap<KEY_T, CONTENT_T>
  {
    minHeap();
    ~minHeap() = default;
    minHeap(const minHeap &heap);
    minHeap(minHeap &&heap);
    minHeap &operator=(const minHeap &heap);
    minHeap &operator=(minHeap &&heap);
    virtual uint32_t heapify(uint32_t parentIndex) override;
    virtual uint32_t heapifyFromChild(uint32_t childIndex) override;
  };

  template<typename KEY_T, typename CONTENT_T>
  void minixHeap<KEY_T, CONTENT_T>::swap(minixHeap<KEY_T, CONTENT_T>::heapNode &node_a, minixHeap<KEY_T, CONTENT_T>::heapNode &node_b)
  {
    node_a.swapInfo(node_b);
    std::swap(node_a.dataPair_, node_b.dataPair_);
  }

  template<typename KEY_T, typename CONTENT_T>
  bool minixHeap<KEY_T, CONTENT_T>::push(std::pair<KEY_T, CONTENT_T> &&dataPair)
  {
    this->heapSize_++;
    auto childIndex = this->heapSize_ - 1;
    heap_.emplace_back(typename minixHeap<KEY_T, CONTENT_T>::heapNode(childIndex, true, std::forward<std::pair<KEY_T, CONTENT_T>>(dataPair)));
    return (heapifyFromChild(childIndex) == INVALID_HEAP_POSITION) ? PROCESS_FAIL : PROCESS_SUCCESS;
  }

  template<typename KEY_T, typename CONTENT_T>
  typename minixHeap<KEY_T, CONTENT_T>::heapNode minixHeap<KEY_T, CONTENT_T>::pushAndGetNode(std::pair<KEY_T, CONTENT_T> &&dataPair)
  {
    this->heapSize_++;
    auto childIndex = this->heapSize_ - 1;
    heap_.emplace_back(typename minixHeap<KEY_T, CONTENT_T>::heapNode(childIndex, true, std::forward<std::pair<KEY_T, CONTENT_T>>(dataPair)));
    auto resultIndex = heapifyFromChild(childIndex);
    return heap_[resultIndex];
  }

  template<typename KEY_T, typename CONTENT_T>
  std::optional<std::pair<KEY_T, CONTENT_T>> minixHeap<KEY_T, CONTENT_T>::pop()
  {
    if (this->heapSize_ == 0)
    {
      return std::nullopt;
    }

    heap_[0].nodeInfo_->validInHeap_ = false;
    swap(heap_[0], heap_.back());
    auto ret = std::move(heap_.back());
    heap_.pop_back();
    this->heapSize_--;

    if (this->heapSize_ <= 1)
    {
      return std::make_optional(*(ret.dataPair_));
    }
    heapify(0);
    return std::make_optional(*(ret.dataPair_));
  }

  template<typename KEY_T, typename CONTENT_T>
  std::optional<std::pair<KEY_T, CONTENT_T>> minixHeap<KEY_T, CONTENT_T>::top()
  {
    if (this->heapSize_ == 0)
    {
      return std::nullopt;
    }
    return std::make_optional(*(heap_[0].dataPair_));
  }

  template<typename KEY_T, typename CONTENT_T>
  bool minixHeap<KEY_T, CONTENT_T>::empty()
  {
    return heapSize_ == 0;
  }

  template<typename KEY_T, typename CONTENT_T>
  int minixHeap<KEY_T, CONTENT_T>::size()
  {
    return heapSize_;
  }

  template<typename KEY_T, typename CONTENT_T>
  bool minixHeap<KEY_T, CONTENT_T>::erase(uint32_t heapIndex)
  {
    if (heapSize_ == 0 || heap_[heapIndex].validInHeap() == false)
    {
      return PROCESS_FAIL;
    }

    auto endIndex = this->heapSize_ - 1;
    if (heapIndex > endIndex)
    {
      return PROCESS_FAIL;
    }
    else if (heapIndex == endIndex)
    {
      heap_.pop_back();
      this->heapSize_--;
      return PROCESS_SUCCESS;
    }

    heap_[heapIndex].nodeInfo_->validInHeap_ = false;
    swap(heap_[heapIndex], heap_[endIndex]);
    heap_.pop_back();
    this->heapSize_--;
    heapify(heapIndex);
    return PROCESS_SUCCESS;
  }

  template<typename KEY_T, typename CONTENT_T>
  bool minixHeap<KEY_T, CONTENT_T>::erase(typename minixHeap<KEY_T, CONTENT_T>::heapNode &node)
  {
    if (node.validInHeap() == false)
    {
      return PROCESS_FAIL;
    }
    return erase(node.nodeInfo_->heapPosition_);
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap<KEY_T, CONTENT_T>::heapNode::heapNode() :
    nodeInfo_(std::make_shared<nodeInfo>()),
    dataPair_(std::make_shared<std::pair<KEY_T, CONTENT_T>>())
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap<KEY_T, CONTENT_T>::heapNode::heapNode(uint32_t heapIndex, bool valid, std::pair<KEY_T, CONTENT_T>&& data_pair) :
    nodeInfo_(std::make_shared<nodeInfo>(heapIndex)),
    dataPair_(std::make_shared<std::pair<KEY_T, CONTENT_T>>(std::forward<std::pair<KEY_T, CONTENT_T>>(data_pair)))
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap<KEY_T, CONTENT_T>::heapNode::heapNode(uint32_t heapIndex, std::pair<KEY_T, CONTENT_T>&& data_pair) :
    nodeInfo_(std::make_shared<nodeInfo>(heapIndex)),
    dataPair_(std::make_shared<std::pair<KEY_T, CONTENT_T>>(std::forward<std::pair<KEY_T, CONTENT_T>>(data_pair)))
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap<KEY_T, CONTENT_T>::heapNode::heapNode(const heapNode &node) :
    nodeInfo_(node.nodeInfo_),
    dataPair_(node.dataPair_)
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap<KEY_T, CONTENT_T>::heapNode::heapNode(heapNode &&node) :
    nodeInfo_(std::move(node.nodeInfo_)),
    dataPair_(std::move(node.dataPair_))
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  typename minixHeap<KEY_T, CONTENT_T>::heapNode& minixHeap<KEY_T, CONTENT_T>::heapNode::operator=(const heapNode &node)
  {
    dataPair_ = node.dataPair_;
    nodeInfo_ = node.nodeInfo_;
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  typename minixHeap<KEY_T, CONTENT_T>::heapNode& minixHeap<KEY_T, CONTENT_T>::heapNode::operator=(heapNode &&node)
  {
    nodeInfo_ = std::move(node.nodeInfo_);
    dataPair_ = std::move(node.dataPair_);
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  void minixHeap<KEY_T, CONTENT_T>::heapNode::swap(heapNode &node)
  {
    std::swap(node.dataPair_, dataPair_);
    std::swap(nodeInfo_, node.nodeInfo_);
  }

  template<typename KEY_T, typename CONTENT_T>
  bool minixHeap<KEY_T, CONTENT_T>::heapNode::validInHeap()
  {
    return nodeInfo_->validInHeap_;
  }

  template<typename KEY_T, typename CONTENT_T>
  bool minixHeap<KEY_T, CONTENT_T>::heapNode::swapInfo(heapNode &node)
  {
    if (node.nodeInfo_ == nullptr || nodeInfo_ == nullptr)
    {
      return PROCESS_FAIL;
    }
    std::swap(*nodeInfo_, *node.nodeInfo_);
    return PROCESS_SUCCESS;
  }

  template<typename KEY_T, typename CONTENT_T>
  maxHeap<KEY_T, CONTENT_T >::maxHeap() :
    minixHeap<KEY_T, CONTENT_T>()
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  maxHeap<KEY_T, CONTENT_T>::maxHeap(const maxHeap &heap) :
    minixHeap<KEY_T, CONTENT_T>()
  {
    this->heap_ = heap.heap_;
    this->heapSize_ = heap.heapSize_;
  }

  template<typename KEY_T, typename CONTENT_T>
  maxHeap<KEY_T, CONTENT_T>::maxHeap(maxHeap &&heap) :
    minixHeap<KEY_T, CONTENT_T>()
  {
    this->heap_ = std::move(heap.heap_);
    this->heapSize_ = heap.heapSize_;
    heap.heapSize_ = 0;
  }

  template<typename KEY_T, typename CONTENT_T>
  maxHeap<KEY_T, CONTENT_T>& maxHeap<KEY_T, CONTENT_T>::operator=(const maxHeap &heap)
  {
    this->heap_ = heap.heap_;
    this->heapSize_ = heap.heapSize_;
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  maxHeap<KEY_T, CONTENT_T>& maxHeap<KEY_T, CONTENT_T>::operator=(maxHeap &&heap)
  {
    this->heap_ = std::move(heap.heap_);
    this->heapSize_ = heap.heapSize_;
    heap.heapSize_ = 0;
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  uint32_t maxHeap<KEY_T, CONTENT_T>::heapify(uint32_t parentIndex)
  {
    auto endIndex = this->heapSize_ - 1;
    while (1)
    {
      uint32_t leftChildIndex = 2 * parentIndex + 1;
      uint32_t rightChildIndex = 2 * parentIndex + 2;

      if (rightChildIndex > endIndex)
      {
        rightChildIndex = endIndex;
        if (leftChildIndex > endIndex)
        {
          break;
        }
      }

      if (this->heap_[rightChildIndex].dataPair_->first > this->heap_[leftChildIndex].dataPair_->first)
      {
        if (this->heap_[rightChildIndex].dataPair_->first > this->heap_[parentIndex].dataPair_->first)
        {
          minixHeap<KEY_T, CONTENT_T>::swap(this->heap_[rightChildIndex], this->heap_[parentIndex]);
          parentIndex = rightChildIndex;
        }
        else
        {
          break;
        }
      }
      else
      {
        if (this->heap_[leftChildIndex].dataPair_->first > this->heap_[parentIndex].dataPair_->first)
        {
          minixHeap<KEY_T, CONTENT_T>::swap(this->heap_[leftChildIndex], this->heap_[parentIndex]);
          parentIndex = leftChildIndex;
        }
        else
        {
          break;
        }
      }

      if (parentIndex == endIndex)
      {
        break;
      }
    }
    return PROCESS_SUCCESS;
  }

  template<typename KEY_T, typename CONTENT_T>
  uint32_t maxHeap<KEY_T, CONTENT_T>::heapifyFromChild(uint32_t childIndex)
  {
    while (1)
    {
      if (childIndex == 0)
      {
        break;
      }
      auto parentIndex = _GET_PARENT_INDEX(childIndex);
      if (this->heap_[parentIndex].dataPair_->first < this->heap_[childIndex].dataPair_->first)
      {
        minixHeap<KEY_T, CONTENT_T>::swap(this->heap_[parentIndex], this->heap_[childIndex]);

        childIndex = parentIndex;
      }
      else
      {
        break;
      }
    }
    return childIndex;
  }

  template<typename KEY_T, typename CONTENT_T>
  minHeap<KEY_T, CONTENT_T>::minHeap() :
    minixHeap<KEY_T, CONTENT_T>()
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  minHeap<KEY_T, CONTENT_T>::minHeap(const minHeap &heap) :
    minixHeap<KEY_T, CONTENT_T>()
  {
    this->heap_ = heap.heap_;
    this->heapSize_ = heap.heapSize_;
  }

  template<typename KEY_T, typename CONTENT_T>
  minHeap<KEY_T, CONTENT_T>::minHeap(minHeap &&heap) :
    minixHeap<KEY_T, CONTENT_T>()
  {
    this->heap_ = std::move(heap.heap_);
    this->heapSize_ = heap.heapSize_;
    heap.heapSize_ = 0;
  }

  template<typename KEY_T, typename CONTENT_T>
  minHeap<KEY_T, CONTENT_T>& minHeap<KEY_T, CONTENT_T>::operator=(const minHeap &heap)
  {
    this->heap_ = heap.heap_;
    this->heapSize_ = heap.heapSize_;
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  minHeap<KEY_T, CONTENT_T>& minHeap<KEY_T, CONTENT_T>::operator=(minHeap &&heap)
  {
    this->heap_ = std::move(heap.heap_);
    this->heapSize_ = heap.heapSize_;
    heap.heapSize_ = 0;
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  uint32_t minHeap<KEY_T, CONTENT_T>::heapify(uint32_t parentIndex)
  {
    auto endIndex = this->heapSize_ - 1;
    while (1)
    {
      uint32_t leftChildIndex = 2 * parentIndex + 1;
      uint32_t rightChildIndex = 2 * parentIndex + 2;

      if (rightChildIndex > endIndex)
      {
        rightChildIndex = endIndex;
        if (leftChildIndex > endIndex)
        {
          break;
        }
      }

      if (this->heap_[rightChildIndex].dataPair_->first < this->heap_[leftChildIndex].dataPair_->first)
      {
        if (this->heap_[rightChildIndex].dataPair_->first < this->heap_[parentIndex].dataPair_->first)
        {
          minixHeap<KEY_T, CONTENT_T>::swap(this->heap_[rightChildIndex], this->heap_[parentIndex]);
          parentIndex = rightChildIndex;
        }
        else
        {
          break;
        }
      }
      else
      {
        if (this->heap_[leftChildIndex].dataPair_->first < this->heap_[parentIndex].dataPair_->first)
        {
          minixHeap<KEY_T, CONTENT_T>::swap(this->heap_[leftChildIndex], this->heap_[parentIndex]);
          parentIndex = leftChildIndex;
        }
        else
        {
          break;
        }
      }
      if (parentIndex == endIndex)
      {
        break;
      }
    }

    return PROCESS_SUCCESS;
  }

  template<typename KEY_T, typename CONTENT_T>
  uint32_t minHeap<KEY_T, CONTENT_T>::heapifyFromChild(uint32_t childIndex)
  {
    while (1)
    {
      if (childIndex == 0)
      {
        break;
      }
      auto parentIndex = _GET_PARENT_INDEX(childIndex);
      if (this->heap_[parentIndex].dataPair_->first > this->heap_[childIndex].dataPair_->first)
      {
        minixHeap<KEY_T, CONTENT_T>::swap(this->heap_[parentIndex], this->heap_[childIndex]);
        childIndex = parentIndex;
      }
      else
      {
        break;
      }
    }
    return childIndex;
  }

} // namespace dawn

#endif
