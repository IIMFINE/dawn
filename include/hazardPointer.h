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
struct registerThreadToQueue
{
    hazardPointerContainer<T>* localContainer_;
    registerThreadToQueue():
        localContainer_(nullptr)
    {
        auto tempContainerIndex = hazardPointerQueue<T>::getHeader().next_.load(std::memory_order_acquire);
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
        auto tempContainerIndex = hazardPointerQueue<T>::getHeader().next_.load(std::memory_order_acquire );
        auto newContainer = new hazardPointerContainer<T>();
        while (1)
        {
            //When initializing hazard pointer queue, there are only push operations.
            newContainer->next_ = tempContainerIndex;
            if(hazardPointerQueue<T>::getHeader().next_.compare_exchange_weak(tempContainerIndex, newContainer))
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
        T tempValue;
        value_.exchange(tempValue);
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
    hazardPointerQueue() = default;
    ~hazardPointerQueue();
    static hazardPointerContainerHeader_t<T>& getHeader();
    bool findConflictPointer(T value);
    bool setHazardPointer(T value);
    bool removeHazardPointer();
    hazardPointerContainer<T>* getThreadLocalContainer();
};

template<typename T>
hazardPointerQueue<T>::~hazardPointerQueue()
{
    auto headIndex = hazardPointerQueue::getHeader().next_.load(std::memory_order_acquire);
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
hazardPointerContainerHeader_t<T>& hazardPointerQueue<T>::getHeader()
{
    static hazardPointerContainerHeader_t<T> header;
    return header;
}

template<typename T>
hazardPointerContainer<T>* hazardPointerQueue<T>::getThreadLocalContainer()
{
    thread_local registerThreadToQueue<T> registerContainer;
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
    for(auto containerIndex = hazardPointerQueue::getHeader().next_.load(std::memory_order_acquire); containerIndex != nullptr; containerIndex = containerIndex->next_)
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