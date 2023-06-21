#ifndef _HEAP_H_
#define _HEAP_H_
#include <vector>
#include <optional>
#include "type.h"

namespace dawn
{
#define _GET_PARENT_INDEX(index) ((index - 1) / 2)

  struct minixHeap
  {
    minixHeap() = default;
    virtual ~minixHeap() = default;
    bool empty();
    int size();
    protected:
    int heapSize_ = 0;
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
    bool push(std::pair<KEY_T, CONTENT_T> node);
    std::optional<std::pair<KEY_T, CONTENT_T>> pop();
    std::optional<std::pair<KEY_T, CONTENT_T>> top();

    private:
    std::vector<std::pair<KEY_T, CONTENT_T>> heap_;
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
    bool push(std::pair<KEY_T, CONTENT_T> node);
    std::optional<std::pair<KEY_T, CONTENT_T>> pop();
    std::optional<std::pair<KEY_T, CONTENT_T>> top();

    private:
    std::vector<std::pair<KEY_T, CONTENT_T>> heap_;
  };

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
  bool maxHeap<KEY_T, CONTENT_T >::push(std::pair<KEY_T, CONTENT_T> node)
  {
    heap_.emplace_back(node);
    heapSize_++;
    auto childIndex = heapSize_ - 1;
    while (1)
    {
      if (childIndex == 0)
      {
        break;
      }
      auto parentIndex = _GET_PARENT_INDEX(childIndex);
      if (heap_[parentIndex].first < heap_[childIndex].first)
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
  std::optional<std::pair<KEY_T, CONTENT_T>> maxHeap<KEY_T, CONTENT_T>::pop()
  {
    if (heapSize_ == 0)
    {
      return std::nullopt;
    }
    auto ret = std::move(heap_[0]);
    heap_[0] = std::move(heap_.back());
    heap_.pop_back();
    heapSize_--;
    int parentIndex = 0;
    auto endIndex = heapSize_ - 1;

    if (heapSize_ <= 1)
    {
      return std::make_optional(ret);
    }

    while (1)
    {
      int leftChildIndex = 2 * parentIndex + 1;
      int rightChildIndex = 2 * parentIndex + 2;

      if (leftChildIndex > endIndex)
      {
        leftChildIndex = endIndex;
      }

      if (rightChildIndex > endIndex)
      {
        rightChildIndex = endIndex;
      }

      if (heap_[rightChildIndex].first > heap_[leftChildIndex].first)
      {
        if (heap_[rightChildIndex].first > heap_[parentIndex].first)
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
        if (heap_[leftChildIndex].first > heap_[parentIndex].first)
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
    return std::make_optional(ret);
  }

  template<typename KEY_T, typename CONTENT_T>
  std::optional<std::pair<KEY_T, CONTENT_T>> maxHeap<KEY_T, CONTENT_T>::top()
  {
    if (heapSize_ == 0)
    {
      return std::nullopt;
    }
    return std::make_optional(heap_[0]);
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
  bool minHeap<KEY_T, CONTENT_T>::push(std::pair<KEY_T, CONTENT_T> node)
  {
    heap_.emplace_back(node);
    heapSize_++;
    auto childIndex = heapSize_ - 1;
    while (1)
    {
      if (childIndex == 0)
      {
        break;
      }
      auto parentIndex = _GET_PARENT_INDEX(childIndex);
      if (heap_[parentIndex].first > heap_[childIndex].first)
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
  std::optional<std::pair<KEY_T, CONTENT_T>> minHeap<KEY_T, CONTENT_T>::pop()
  {
    if (heapSize_ == 0)
    {
      return std::nullopt;
    }
    auto ret = std::move(heap_[0]);
    heap_[0] = std::move(heap_.back());
    heap_.pop_back();
    heapSize_--;
    int parentIndex = 0;
    auto endIndex = heapSize_ - 1;

    if (heapSize_ <= 1)
    {
      return std::make_optional(ret);
    }

    while (1)
    {
      int leftChildIndex = 2 * parentIndex + 1;
      int rightChildIndex = 2 * parentIndex + 2;

      if (rightChildIndex > endIndex)
      {
        rightChildIndex = endIndex;
        if (leftChildIndex > endIndex)
        {
          leftChildIndex = endIndex;
        }
      }

      if (heap_[rightChildIndex].first < heap_[leftChildIndex].first)
      {
        if (heap_[rightChildIndex].first < heap_[parentIndex].first)
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
        if (heap_[leftChildIndex].first < heap_[parentIndex].first)
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
    return std::make_optional(ret);
  }

  template<typename KEY_T, typename CONTENT_T>
  std::optional<std::pair<KEY_T, CONTENT_T>> minHeap<KEY_T, CONTENT_T>::top()
  {
    if (heapSize_ == 0)
    {
      return std::nullopt;
    }
    return std::make_optional(heap_[0]);
  }

} // namespace dawn

#endif
