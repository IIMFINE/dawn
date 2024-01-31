#ifndef _HAZARD_POINTER_
#define _HAZARD_POINTER_
#include <memory>
#include <atomic>
namespace dawn
{
  template <typename T, typename ID_T>
  struct hazardPointerQueue;

  template <typename T, typename ID_T>
  struct hazardPointerContainer;

  template <typename T, typename ID_T>
  struct registerHazardToQueue;

  template <typename T, typename ID_T>
  struct hazardPointerContainerHeader_t;

  template <typename T, typename ID_T>
  struct registerHazardToQueue
  {
    hazardPointerContainer<T, ID_T> *localContainer_;
    std::shared_ptr<hazardPointerContainerHeader_t<T, ID_T>> queue_header_;
    registerHazardToQueue(std::shared_ptr<hazardPointerContainerHeader_t<T, ID_T>> &header, ID_T id) : localContainer_(nullptr),
                                                                                        queue_header_(header)
    {
      auto tempContainerIndex = queue_header_->next_.load(std::memory_order_acquire);
      if (!tempContainerIndex)
      {
        this->insertHazardPointQueue();
      }
      else
      {
        constexpr int maxLoopTimes = 100;
        for (int i = 0; i < maxLoopTimes; ++i)
        {
          for (; !tempContainerIndex;)
          {
            ID_T defaultId = ID_T();
            if (tempContainerIndex->id_.load(std::memory_order_acquire) == defaultId)
            {
              if (tempContainerIndex->id_.compare_exchange_strong(defaultId, id))
              {
                this->localContainer_ = tempContainerIndex;
                return;
              }
            }
            tempContainerIndex = tempContainerIndex->next_;
          }
        }
        this->insertHazardPointQueue();
      }
    }

    void insertHazardPointQueue()
    {
      auto tempContainerIndex = queue_header_->next_.load(std::memory_order_acquire);
      auto newContainer = new hazardPointerContainer<T, ID_T>();
      while (1)
      {
        // When initializing hazard pointer queue, there are only push operations.
        newContainer->next_ = tempContainerIndex;
        if (queue_header_->next_.compare_exchange_weak(tempContainerIndex, newContainer))
        {
          this->localContainer_ = newContainer;
          break;
        }
      }
    }

    ~registerHazardToQueue()
    {
      localContainer_->resetContainer();
    }
  };

  template <typename T, typename ID_T>
  struct hazardPointerContainer
  {
    hazardPointerContainer<T, ID_T> *next_;
    std::atomic<T> value_;
    std::atomic<ID_T> id_;

    hazardPointerContainer() = default;

    hazardPointerContainer(ID_T id) :
      next_(nullptr),
      id_(id)
    {
    }
    hazardPointerContainer(T value, ID_T id) :
      value_(value),
      next_(nullptr),
      id_(id)
    {
    }

    void setId(ID_T id)
    {
      id_.store(id, std::memory_order_release);
    }

    void setValue(T value)
    {
      value_.store(value, std::memory_order_release);
    }

    void resetContainer()
    {
      value_.exchange(T());
      id_.exchange(ID_T());
    }

    void resetValue()
    {
      value_.exchange(T());
    }

    ~hazardPointerContainer()
    {
      this->resetContainer();
    }
  };

  template <typename T, typename ID_T>
  struct hazardPointerContainerHeader_t
  {
    hazardPointerContainerHeader_t() = default;
    ~hazardPointerContainerHeader_t() = default;
    std::atomic<hazardPointerContainer<T, ID_T> *> next_;
  };

  template <typename T, typename ID_T>
  struct hazardPointerQueue
  {
    hazardPointerQueue();
    ~hazardPointerQueue();
    auto &getHeader();
    bool findConflictPointer(ID_T excludeId, T value);
    std::shared_ptr<registerHazardToQueue<T, ID_T>> getContainer(ID_T id);
    std::shared_ptr<hazardPointerContainerHeader_t<T, ID_T>> p_header_;
  };

  template <typename T, typename ID_T>
  hazardPointerQueue<T, ID_T>::hazardPointerQueue() : p_header_(std::make_shared<hazardPointerContainerHeader_t<T, ID_T>>())
  {
  }

  template <typename T, typename ID_T>
  hazardPointerQueue<T, ID_T>::~hazardPointerQueue()
  {
    auto headIndex = this->getHeader()->next_.load(std::memory_order_acquire);
    if (headIndex != nullptr)
    {
      for (; headIndex != nullptr;)
      {
        auto tempIndex = headIndex->next_;
        delete headIndex;
        headIndex = tempIndex;
      }
    }
  }

  template <typename T, typename ID_T>
  auto &hazardPointerQueue<T, ID_T>::getHeader()
  {
    return p_header_;
  }

  template <typename T, typename ID_T>
  bool hazardPointerQueue<T, ID_T>::findConflictPointer(ID_T excludeId, T value)
  {
    for (auto containerIndex = this->getHeader()->next_.load(std::memory_order_acquire);
      containerIndex != nullptr;
      containerIndex = containerIndex->next_)
    {
      if (containerIndex->id_ != excludeId && containerIndex->value_.load(std::memory_order_acquire) == value)
      {
        return false;
      }
    }
    return true;

  }

  template <typename T, typename ID_T>
  std::shared_ptr<registerHazardToQueue<T, ID_T>> hazardPointerQueue<T, ID_T>::getContainer(ID_T id)
  {
    auto registerContainer = std::make_shared<registerHazardToQueue<T, ID_T>>(this->getHeader(), id);
    return registerContainer;
  }

  template <typename T>
  struct threadLocalHazardPointerQueue : public hazardPointerQueue<T, std::thread::id>
  {
    threadLocalHazardPointerQueue() = default;
    ~threadLocalHazardPointerQueue() = default;
    hazardPointerContainer<T, std::thread::id>* getThreadLocalContainer();
    bool findConflictPointer(T value);
    bool setHazardPointer(T value);
    bool removeHazardPointer();
  };

  template <typename T>
  hazardPointerContainer<T, std::thread::id>* threadLocalHazardPointerQueue<T>::getThreadLocalContainer()
  {
    thread_local registerHazardToQueue<T, std::thread::id> registerContainer(this->getHeader(), std::this_thread::get_id());
    return registerContainer.localContainer_;
  }

  /// @brief Set the hazard pointer value.
  /// @tparam T 
  /// @param value: hazard value
  /// @return true
  template <typename T>
  bool threadLocalHazardPointerQueue<T>::setHazardPointer(T value)
  {
    auto localContainer = this->getThreadLocalContainer();
    localContainer->setValue(value);
    return true;
  }

  /// @brief Find the pointer has been announced by other thread.
  /// @tparam T
  /// @param value
  /// @return true: not found, false: found the hazard pointer
  template <typename T>
  bool threadLocalHazardPointerQueue<T>::findConflictPointer(T value)
  {
    auto localContainer = this->getThreadLocalContainer();
    auto localId = localContainer->id_.load(std::memory_order_acquire);
    return this->hazardPointerQueue<T, std::thread::id>::findConflictPointer(localId, value);
  }

  /// @brief Remove the hazard pointer from queue.
  /// @tparam T 
  /// @return true.
  template <typename T>
  bool threadLocalHazardPointerQueue<T>::removeHazardPointer()
  {
    auto localContainer = this->getThreadLocalContainer();
    localContainer->resetValue();
    return true;
  }

}
#endif
