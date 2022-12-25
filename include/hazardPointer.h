#ifndef _HAZARD_POINTER_
#define _HAZARD_POINTER_
#include <memory>
#include <atomic>
namespace dawn
{
template<typename T>
struct hazardPointerQueue;

template<typename T>
struct hazardPointerContainer;

template<typename T>
struct registerThreadToQueue;

template<typename T>
struct hazardPointerContainerHeader_t;

template<typename T>
struct registerThreadToQueue
{
    hazardPointerContainer<T>* localContainer_;
    std::shared_ptr<hazardPointerContainerHeader_t<T>> queue_header_;
    registerThreadToQueue(std::shared_ptr<hazardPointerContainerHeader_t<T>>& header):
        localContainer_(nullptr),
        queue_header_(header)
    {
        auto tempContainerIndex = queue_header_->next_.load(std::memory_order_acquire);
        if(!tempContainerIndex)
        {
            this->insertHazardPointQueue();
        }
        else
        {
            constexpr int maxLoopTimes = 100;
            for(int i = 0; i < maxLoopTimes; ++i)
            {
                for(; !tempContainerIndex;)
                {
                    std::thread::id defaultThreadId;
                    if(tempContainerIndex->thread_id_.load(std::memory_order_acquire ) == defaultThreadId)
                    {
                        if(tempContainerIndex->thread_id_.compare_exchange_strong(defaultThreadId, std::this_thread::get_id()))
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
        auto tempContainerIndex = queue_header_->next_.load(std::memory_order_acquire );
        auto newContainer = new hazardPointerContainer<T>();
        while (1)
        {
            //When initializing hazard pointer queue, there are only push operations.
            newContainer->next_ = tempContainerIndex;
            if(queue_header_->next_.compare_exchange_weak(tempContainerIndex, newContainer))
            {
                this->localContainer_ = newContainer;
                break;
            }
        }
    }

    ~registerThreadToQueue()
    {
        localContainer_->resetContainer();
    }
};

template<typename T>
struct hazardPointerContainer
{
    hazardPointerContainer<T>* next_;
    std::atomic<T> value_;
    std::atomic<std::thread::id> thread_id_;

    hazardPointerContainer():
        next_(nullptr),
        thread_id_(std::this_thread::get_id())
    {
    }
    hazardPointerContainer(T value) :
        value_(value),
        next_(nullptr),
        thread_id_(std::this_thread::get_id())
    {
    }

    void resetContainer()
    {
        // T tempValue;
        // std::thread::id tempThreadId;
        value_.exchange(T());
        thread_id_.exchange(std::thread::id());
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

template<typename T>
struct hazardPointerContainerHeader_t
{
    hazardPointerContainerHeader_t() = default;
    ~hazardPointerContainerHeader_t() = default;
    std::atomic<hazardPointerContainer<T>*> next_;
};

template<typename T>
struct hazardPointerQueue
{
    hazardPointerQueue();
    ~hazardPointerQueue();
    auto& getHeader();
    bool findConflictPointer(T value);
    bool setHazardPointer(T value);
    bool removeHazardPointer();
    hazardPointerContainer<T>* getThreadLocalContainer();
    std::shared_ptr<hazardPointerContainerHeader_t<T>> p_header_;
};

template<typename T>
hazardPointerQueue<T>::hazardPointerQueue():
p_header_(std::make_shared<hazardPointerContainerHeader_t<T>>())
{
}

template<typename T>
hazardPointerQueue<T>::~hazardPointerQueue()
{
    auto headIndex = this->getHeader()->next_.load(std::memory_order_acquire);
    if(headIndex != nullptr)
    {
        for(;headIndex != nullptr;)
        {
            auto tempIndex = headIndex;
            delete headIndex;
            headIndex = tempIndex->next_;
        }
    }
}

template<typename T>
auto& hazardPointerQueue<T>::getHeader()
{
    return p_header_;
}

template<typename T>
hazardPointerContainer<T>* hazardPointerQueue<T>::getThreadLocalContainer()
{
    thread_local registerThreadToQueue<T> registerContainer(this->getHeader());
    return registerContainer.localContainer_;
}

template<typename T>
bool hazardPointerQueue<T>::setHazardPointer(T value)
{
    auto localContainer = this->getThreadLocalContainer();
    localContainer->value_.store(value, std::memory_order_release);
    return true;
}

/// @brief Find the pointer has been announced by other thread.
/// @tparam T 
/// @param value 
/// @return true: not found, false: found the hazard pointer
template<typename T>
bool hazardPointerQueue<T>::findConflictPointer(T value)
{
    for(auto containerIndex = this->getHeader()->next_.load(std::memory_order_acquire);
            containerIndex != nullptr;
            containerIndex = containerIndex->next_)
    {
        if(containerIndex->thread_id_ != std::this_thread::get_id() && containerIndex->value_.load(std::memory_order_acquire) == value)
        {
            return false;
        }
    }
    return true;
}

template<typename T>
bool hazardPointerQueue<T>::removeHazardPointer()
{
    auto localContainer = this->getThreadLocalContainer();
    localContainer->resetValue();
    return true;
}

}
#endif
