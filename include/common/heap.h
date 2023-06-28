#ifndef _HEAP_H_
#define _HEAP_H_
#include <vector>
#include <optional>
#include <memory>
#include <algorithm>
#include "type.h"
#include <iostream>

namespace dawn
{
#define _GET_PARENT_INDEX(index) ((index - 1) / 2)

  constexpr uint32_t INVALID_HEAP_POSITION = 0xFFFFFFFF;
  struct minixHeap
  {
    template<typename KEY_T, typename CONTENT_T>
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

      void swap(heapNode<KEY_T, CONTENT_T> &node);

      bool validInHeap();

      /// @brief Use smart pointer to keep update of heap position of node
      std::shared_ptr<nodeInfo>                       nodeInfo_;
      std::shared_ptr<std::pair<KEY_T, CONTENT_T>>    dataPair_;
      std::shared_ptr<bool>                           validInHeap_;
    };

    minixHeap() = default;
    ~minixHeap() = default;
    bool empty();
    int size();
    protected:
    uint32_t heapSize_ = 0;
  };

  template<typename KEY_T, typename CONTENT_T>
  struct maxHeap : public minixHeap
  {
    maxHeap();
    ~maxHeap() = default;
    maxHeap(const maxHeap &heap);
    maxHeap(maxHeap &&heap);
    maxHeap &operator=(const maxHeap &heap);
    maxHeap &operator=(maxHeap &&heap);
    bool push(std::pair<KEY_T, CONTENT_T> &&dataPair);
    minixHeap::heapNode<KEY_T, CONTENT_T> pushAndGetNode(std::pair<KEY_T, CONTENT_T> &&dataPair);
    std::optional<std::pair<KEY_T, CONTENT_T>> pop();
    std::optional<std::pair<KEY_T, CONTENT_T>> top();
    bool heapify(uint32_t parentIndex);
    private:
    std::vector<minixHeap::heapNode<key_t, CONTENT_T>> heap_;
  };

  template<typename KEY_T, typename CONTENT_T>
  struct minHeap : public minixHeap
  {
    minHeap();
    ~minHeap() = default;
    minHeap(const minHeap &heap);
    minHeap(minHeap &&heap);
    minHeap &operator=(const minHeap &heap);
    minHeap &operator=(minHeap &&heap);
    bool push(std::pair<KEY_T, CONTENT_T> &&dataPair);
    minixHeap::heapNode<KEY_T, CONTENT_T> pushAndGetNode(std::pair<KEY_T, CONTENT_T> &&dataPair);
    std::optional<std::pair<KEY_T, CONTENT_T>> pop();
    std::optional<std::pair<KEY_T, CONTENT_T>> top();
    bool heapify(uint32_t parentIndex);
    private:
    std::vector<minixHeap::heapNode<key_t, CONTENT_T>> heap_;
  };

  template<typename KEY_T, typename CONTENT_T>
  minixHeap::heapNode<KEY_T, CONTENT_T>::heapNode() :
    nodeInfo_(std::make_shared<nodeInfo>()),
    dataPair_(std::make_shared<std::pair<KEY_T, CONTENT_T>>()),
    validInHeap_(std::make_shared<bool>(true))
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap::heapNode<KEY_T, CONTENT_T>::heapNode(uint32_t heapIndex, bool valid, std::pair<KEY_T, CONTENT_T>&& data_pair) :
    nodeInfo_(std::make_shared<nodeInfo>(heapIndex)),
    dataPair_(std::make_shared<std::pair<KEY_T, CONTENT_T>>(std::forward<std::pair<KEY_T, CONTENT_T>>(data_pair))),
    validInHeap_(std::make_shared<bool>(valid))
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap::heapNode<KEY_T, CONTENT_T>::heapNode(uint32_t heapIndex, std::pair<KEY_T, CONTENT_T>&& data_pair) :
    nodeInfo_(std::make_shared<nodeInfo>(heapIndex)),
    dataPair_(std::make_shared<std::pair<KEY_T, CONTENT_T>>(std::forward<std::pair<KEY_T, CONTENT_T>>(data_pair))),
    validInHeap_(std::make_shared<bool>(true))
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap::heapNode<KEY_T, CONTENT_T>::heapNode(const heapNode &node) :
    nodeInfo_(node.nodeInfo_),
    dataPair_(node.dataPair_)
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap::heapNode<KEY_T, CONTENT_T>::heapNode(heapNode &&node) :
    nodeInfo_(std::make_shared<nodeInfo>()),
    dataPair_(std::make_shared<std::pair<KEY_T, CONTENT_T>>())
  {
    std::swap((dataPair_), (node.dataPair_));
    std::swap(nodeInfo_, node.nodeInfo_);
    *node.nodeInfo_ = *nodeInfo_;
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap::heapNode<KEY_T, CONTENT_T>& minixHeap::heapNode<KEY_T, CONTENT_T>::operator=(const heapNode &node)
  {
    dataPair_ = node.dataPair_;
    nodeInfo_ = node.nodeInfo_;
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap::heapNode<KEY_T, CONTENT_T>& minixHeap::heapNode<KEY_T, CONTENT_T>::operator=(heapNode &&node)
  {
    //Keep nodeInfo_ value of passed node in itself. In case of losing nodeInfo_ value.
    std::swap(nodeInfo_, node.nodeInfo_);
    std::swap(*(nodeInfo_), *(node.nodeInfo_));

    dataPair_ = std::move(node.dataPair_);
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  void minixHeap::heapNode<KEY_T, CONTENT_T>::swap(heapNode<KEY_T, CONTENT_T> &node)
  {
    std::swap(node.dataPair_, dataPair_);
    std::swap(nodeInfo_, node.nodeInfo_);
    std::swap(*nodeInfo_, *node.nodeInfo_);
  }

  template<typename KEY_T, typename CONTENT_T>
  bool minixHeap::heapNode<KEY_T, CONTENT_T>::validInHeap()
  {
    return *validInHeap_;
  }

  template<typename KEY_T, typename CONTENT_T>
  maxHeap<KEY_T, CONTENT_T > ::maxHeap() :
    minixHeap()
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  maxHeap<KEY_T, CONTENT_T>::maxHeap(const maxHeap &heap) :
    minixHeap(),
    heap_(heap.heap_)
  {
    heapSize_ = heap.heapSize_;
  }

  template<typename KEY_T, typename CONTENT_T>
  maxHeap<KEY_T, CONTENT_T>::maxHeap(maxHeap &&heap) :
    minixHeap(),
    heap_(std::move(heap.heap_))
  {
    heapSize_ = heap.heapSize_;
    heap.heapSize_ = 0;
  }

  template<typename KEY_T, typename CONTENT_T>
  maxHeap<KEY_T, CONTENT_T>& maxHeap<KEY_T, CONTENT_T>::operator=(const maxHeap &heap)
  {
    heap_ = heap.heap_;
    heapSize_ = heap.heapSize_;
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  maxHeap<KEY_T, CONTENT_T>& maxHeap<KEY_T, CONTENT_T>::operator=(maxHeap &&heap)
  {
    heap_ = std::move(heap.heap_);
    heapSize_ = heap.heapSize_;
    heap.heapSize_ = 0;
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  bool maxHeap<KEY_T, CONTENT_T >::push(std::pair<KEY_T, CONTENT_T> &&dataPair)
  {
    heapSize_++;
    auto childIndex = heapSize_ - 1;
    heap_.emplace_back(minixHeap::heapNode(childIndex, true, std::forward<std::pair<KEY_T, CONTENT_T>>(dataPair)));
    while (1)
    {
      if (childIndex == 0)
      {
        break;
      }
      auto parentIndex = _GET_PARENT_INDEX(childIndex);
      if (heap_[parentIndex].dataPair_->first < heap_[childIndex].dataPair_->first)
      {
        std::swap(heap_[parentIndex], heap_[childIndex]);
        childIndex = parentIndex;
      }
      else
      {
        break;
      }
    }
    return PROCESS_SUCCESS;
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap::heapNode<KEY_T, CONTENT_T> maxHeap<KEY_T, CONTENT_T>::pushAndGetNode(std::pair<KEY_T, CONTENT_T> &&dataPair)
  {
    heapSize_++;
    auto childIndex = heapSize_ - 1;
    auto parentIndex = childIndex;
    heap_.emplace_back(minixHeap::heapNode(childIndex, true, std::forward<std::pair<KEY_T, CONTENT_T>>(dataPair)));
    while (1)
    {
      if (childIndex == 0)
      {
        break;
      }
      parentIndex = _GET_PARENT_INDEX(childIndex);
      if (heap_[parentIndex].dataPair_->first < heap_[childIndex].dataPair_->first)
      {
        std::swap(heap_[parentIndex], heap_[childIndex]);
        childIndex = parentIndex;
      }
      else
      {
        break;
      }
    }
    return heap_[childIndex];
  }

  template<typename KEY_T, typename CONTENT_T>
  std::optional<std::pair<KEY_T, CONTENT_T>> maxHeap<KEY_T, CONTENT_T>::pop()
  {
    if (heapSize_ == 0)
    {
      return std::nullopt;
    }

    std::swap(heap_[0], heap_.back());
    auto ret = std::move(heap_.back());
    heap_.pop_back();
    heapSize_--;

    if (heapSize_ <= 1)
    {
      return std::make_optional(*(ret.dataPair_));
    }
    heapify(0);
    return std::make_optional(*(ret.dataPair_));
  }

  template<typename KEY_T, typename CONTENT_T>
  std::optional<std::pair<KEY_T, CONTENT_T>> maxHeap<KEY_T, CONTENT_T>::top()
  {
    if (heapSize_ == 0)
    {
      return std::nullopt;
    }
    return std::make_optional(*(heap_[0].dataPair_));
  }

  template<typename KEY_T, typename CONTENT_T>
  bool maxHeap<KEY_T, CONTENT_T>::heapify(uint32_t parentIndex)
  {
    auto endIndex = heapSize_ - 1;
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

      if (heap_[rightChildIndex].dataPair_->first > heap_[leftChildIndex].dataPair_->first)
      {
        if (heap_[rightChildIndex].dataPair_->first > heap_[parentIndex].dataPair_->first)
        {
          std::swap(heap_[rightChildIndex], heap_[parentIndex]);
          parentIndex = rightChildIndex;
        }
        else
        {
          break;
        }
      }
      else
      {
        if (heap_[leftChildIndex].dataPair_->first > heap_[parentIndex].dataPair_->first)
        {
          std::swap(heap_[leftChildIndex], heap_[parentIndex]);
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
  minHeap<KEY_T, CONTENT_T>::minHeap() :
    minixHeap()
  {
  }

  template<typename KEY_T, typename CONTENT_T>
  minHeap<KEY_T, CONTENT_T>::minHeap(const minHeap &heap) :
    minixHeap(),
    heap_(heap.heap_)
  {
    heapSize_ = heap.heapSize_;
  }

  template<typename KEY_T, typename CONTENT_T>
  minHeap<KEY_T, CONTENT_T>::minHeap(minHeap &&heap) :
    minixHeap(),
    heap_(std::move(heap.heap_))
  {
    heapSize_ = heap.heapSize_;
    heap.heapSize_ = 0;
  }

  template<typename KEY_T, typename CONTENT_T>
  minHeap<KEY_T, CONTENT_T>& minHeap<KEY_T, CONTENT_T>::operator=(const minHeap &heap)
  {
    heap_ = heap.heap_;
    heapSize_ = heap.heapSize_;
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  minHeap<KEY_T, CONTENT_T>& minHeap<KEY_T, CONTENT_T>::operator=(minHeap &&heap)
  {
    heap_ = std::move(heap.heap_);
    heapSize_ = heap.heapSize_;
    heap.heapSize_ = 0;
    return *this;
  }

  template<typename KEY_T, typename CONTENT_T>
  bool minHeap<KEY_T, CONTENT_T>::push(std::pair<KEY_T, CONTENT_T> &&dataPair)
  {
    heapSize_++;
    auto childIndex = heapSize_ - 1;
    heap_.emplace_back(minixHeap::heapNode(childIndex, true, std::forward<std::pair<KEY_T, CONTENT_T>>(dataPair)));

    while (1)
    {
      if (childIndex == 0)
      {
        break;
      }
      auto parentIndex = _GET_PARENT_INDEX(childIndex);
      if (heap_[parentIndex].dataPair_->first > heap_[childIndex].dataPair_->first)
      {
        std::swap(heap_[parentIndex], heap_[childIndex]);
        childIndex = parentIndex;
      }
      else
      {
        break;
      }
    }
    return PROCESS_SUCCESS;
  }

  template<typename KEY_T, typename CONTENT_T>
  minixHeap::heapNode<KEY_T, CONTENT_T> minHeap<KEY_T, CONTENT_T>::pushAndGetNode(std::pair<KEY_T, CONTENT_T> &&dataPair)
  {
    heapSize_++;
    auto childIndex = heapSize_ - 1;
    auto parentIndex = childIndex;
    heap_.emplace_back(minixHeap::heapNode(childIndex, true, std::forward<std::pair<KEY_T, CONTENT_T>>(dataPair)));

    while (1)
    {
      if (childIndex == 0)
      {
        break;
      }
      parentIndex = _GET_PARENT_INDEX(childIndex);
      if (heap_[parentIndex].dataPair_->first > heap_[childIndex].dataPair_->first)
      {
        std::swap(heap_[parentIndex], heap_[childIndex]);
        childIndex = parentIndex;
      }
      else
      {
        break;
      }
    }
    return heap_[parentIndex];
  }

  template<typename KEY_T, typename CONTENT_T>
  std::optional<std::pair<KEY_T, CONTENT_T>> minHeap<KEY_T, CONTENT_T>::pop()
  {
    if (heapSize_ == 0)
    {
      return std::nullopt;
    }

    std::swap(heap_[0], heap_.back());
    auto ret = std::move(heap_.back());
    heap_.pop_back();
    heapSize_--;

    if (heapSize_ <= 1)
    {
      return std::make_optional(*(ret.dataPair_));
    }
    heapify(0);
    return std::make_optional(*(ret.dataPair_));
  }

  template<typename KEY_T, typename CONTENT_T>
  std::optional<std::pair<KEY_T, CONTENT_T>> minHeap<KEY_T, CONTENT_T>::top()
  {
    if (heapSize_ == 0)
    {
      return std::nullopt;
    }
    return std::make_optional(*(heap_[0].dataPair_));
  }

  template<typename KEY_T, typename CONTENT_T>
  bool minHeap<KEY_T, CONTENT_T>::heapify(uint32_t parentIndex)
  {
    auto endIndex = heapSize_ - 1;
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

      if (heap_[rightChildIndex].dataPair_->first < heap_[leftChildIndex].dataPair_->first)
      {
        if (heap_[rightChildIndex].dataPair_->first < heap_[parentIndex].dataPair_->first)
        {
          std::swap(heap_[rightChildIndex], heap_[parentIndex]);
          parentIndex = rightChildIndex;
        }
        else
        {
          break;
        }
      }
      else
      {
        if (heap_[leftChildIndex].dataPair_->first < heap_[parentIndex].dataPair_->first)
        {
          std::swap(heap_[leftChildIndex], heap_[parentIndex]);
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

} // namespace dawn

#endif
