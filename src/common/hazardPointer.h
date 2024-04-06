#ifndef _HAZARD_POINTER_
#define _HAZARD_POINTER_
#include <atomic>
#include <memory>
namespace dawn
{
    template <typename T, typename ID_T>
    struct HazardPointerQueue;

    template <typename T, typename ID_T>
    struct HazardPointerContainer;

    template <typename T, typename ID_T>
    struct RegisterHazardToQueue;

    template <typename T, typename ID_T>
    struct HazardPointerContainerHeader;

    template <typename T, typename ID_T>
    struct RegisterHazardToQueue
    {
        HazardPointerContainer<T, ID_T> *local_container_;
        std::shared_ptr<HazardPointerContainerHeader<T, ID_T>> queue_header_;
        RegisterHazardToQueue(std::shared_ptr<HazardPointerContainerHeader<T, ID_T>> &header, ID_T id) : local_container_(nullptr),
                                                                                                         queue_header_(header)
        {
            auto temp_container_index = queue_header_->next_.load(std::memory_order_acquire);
            if (!temp_container_index)
            {
                this->insertHazardPointQueue();
            }
            else
            {
                constexpr int kMaxLoopTimes = 100;
                for (int i = 0; i < kMaxLoopTimes; ++i)
                {
                    for (; !temp_container_index;)
                    {
                        ID_T defaultId = ID_T();
                        if (temp_container_index->id_.load(std::memory_order_acquire) == defaultId)
                        {
                            if (temp_container_index->id_.compare_exchange_strong(defaultId, id))
                            {
                                this->local_container_ = temp_container_index;
                                return;
                            }
                        }
                        temp_container_index = temp_container_index->next_;
                    }
                }
                this->insertHazardPointQueue();
            }
        }

        void insertHazardPointQueue()
        {
            auto temp_container_index = queue_header_->next_.load(std::memory_order_acquire);
            auto new_container = new HazardPointerContainer<T, ID_T>();
            while (1)
            {
                // When initializing hazard pointer queue, there are only push operations.
                new_container->next_ = temp_container_index;
                if (queue_header_->next_.compare_exchange_weak(temp_container_index, new_container))
                {
                    this->local_container_ = new_container;
                    break;
                }
            }
        }

        ~RegisterHazardToQueue()
        {
            local_container_->resetContainer();
        }
    };

    template <typename T, typename ID_T>
    struct HazardPointerContainer
    {
        HazardPointerContainer<T, ID_T> *next_;
        std::atomic<T> value_;
        std::atomic<ID_T> id_;

        HazardPointerContainer() = default;

        HazardPointerContainer(ID_T id) : next_(nullptr),
                                          id_(id)
        {
        }
        HazardPointerContainer(T value, ID_T id) : value_(value),
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

        ~HazardPointerContainer()
        {
            this->resetContainer();
        }
    };

    template <typename T, typename ID_T>
    struct HazardPointerContainerHeader
    {
        HazardPointerContainerHeader() = default;
        ~HazardPointerContainerHeader() = default;
        std::atomic<HazardPointerContainer<T, ID_T> *> next_;
    };

    template <typename T, typename ID_T>
    struct HazardPointerQueue
    {
        HazardPointerQueue();
        ~HazardPointerQueue();
        auto &getHeader();
        bool findConflictPointer(ID_T excludeId, T value);
        std::shared_ptr<RegisterHazardToQueue<T, ID_T>> getContainer(ID_T id);
        std::shared_ptr<HazardPointerContainerHeader<T, ID_T>> p_header_;
    };

    template <typename T, typename ID_T>
    HazardPointerQueue<T, ID_T>::HazardPointerQueue() : p_header_(std::make_shared<HazardPointerContainerHeader<T, ID_T>>())
    {
    }

    template <typename T, typename ID_T>
    HazardPointerQueue<T, ID_T>::~HazardPointerQueue()
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
    auto &HazardPointerQueue<T, ID_T>::getHeader()
    {
        return p_header_;
    }

    template <typename T, typename ID_T>
    bool HazardPointerQueue<T, ID_T>::findConflictPointer(ID_T excludeId, T value)
    {
        for (auto container_index = this->getHeader()->next_.load(std::memory_order_acquire);
             container_index != nullptr;
             container_index = container_index->next_)
        {
            if (container_index->id_ != excludeId && container_index->value_.load(std::memory_order_acquire) == value)
            {
                return false;
            }
        }
        return true;
    }

    template <typename T, typename ID_T>
    std::shared_ptr<RegisterHazardToQueue<T, ID_T>> HazardPointerQueue<T, ID_T>::getContainer(ID_T id)
    {
        auto register_container = std::make_shared<RegisterHazardToQueue<T, ID_T>>(this->getHeader(), id);
        return register_container;
    }

    template <typename T>
    struct threadLocalHazardPointerQueue : public HazardPointerQueue<T, std::thread::id>
    {
        threadLocalHazardPointerQueue() = default;
        ~threadLocalHazardPointerQueue() = default;
        HazardPointerContainer<T, std::thread::id> *getThreadLocalContainer();
        bool findConflictPointer(T value);
        bool setHazardPointer(T value);
        bool removeHazardPointer();
    };

    template <typename T>
    HazardPointerContainer<T, std::thread::id> *threadLocalHazardPointerQueue<T>::getThreadLocalContainer()
    {
        thread_local RegisterHazardToQueue<T, std::thread::id> register_container(this->getHeader(), std::this_thread::get_id());
        return register_container.local_container_;
    }

    /// @brief Set the hazard pointer value.
    /// @tparam T
    /// @param value: hazard value
    /// @return true
    template <typename T>
    bool threadLocalHazardPointerQueue<T>::setHazardPointer(T value)
    {
        auto local_container = this->getThreadLocalContainer();
        local_container->setValue(value);
        return true;
    }

    /// @brief Find the pointer has been announced by other thread.
    /// @tparam T
    /// @param value
    /// @return true: not found, false: found the hazard pointer
    template <typename T>
    bool threadLocalHazardPointerQueue<T>::findConflictPointer(T value)
    {
        auto local_container = this->getThreadLocalContainer();
        auto localId = local_container->id_.load(std::memory_order_acquire);
        return this->HazardPointerQueue<T, std::thread::id>::findConflictPointer(localId, value);
    }

    /// @brief Remove the hazard pointer from queue.
    /// @tparam T
    /// @return true.
    template <typename T>
    bool threadLocalHazardPointerQueue<T>::removeHazardPointer()
    {
        auto local_container = this->getThreadLocalContainer();
        local_container->resetValue();
        return true;
    }

}
#endif
