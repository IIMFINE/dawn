#ifndef _LOCK_FREE_H_
#define _LOCK_FREE_H_
#include <atomic>
#include <thread>
#include "type.h"
#include "hazardPointer.h"
#include "setLogger.h"
namespace dawn
{

template<typename T>
struct  LF_node_t
{
    LF_node_t():
    elementVal_(nullptr),
    next_(nullptr)
    {
    }

    LF_node_t(T&& val):
    elementVal_(std::forward<T>(val)),
    next_(nullptr)
    {
    }

    T elementVal_;
    LF_node_t *next_;

};

//dont use smart point which uses too many memory resource 
template<typename T>
class lockFreeStack
{
    std::atomic<LF_node_t<T>*>         LF_queueHead_;
    hazardPointerQueue<LF_node_t<T>*>  hazardPointerQueue_;
public:
    lockFreeStack();
    ~lockFreeStack()=default;
    void init(LF_node_t<T>* queueHead);
    LF_node_t<T>* popNodeWithHazard();
    void pushNode(LF_node_t<T> *node);
};


template<typename T>
lockFreeStack<T>::lockFreeStack():
LF_queueHead_(static_cast<LF_node_t<T>*>(nullptr))
{
}

template<typename T>
void lockFreeStack<T>::init(LF_node_t<T>* queueHead)
{
    LF_queueHead_.store(queueHead);
}

template<typename T>
void lockFreeStack<T>::pushNode(LF_node_t<T> *node)
{
    if(node != nullptr)
    {
        auto oldHead =  LF_queueHead_.load(std::memory_order_acquire);
        do
        {
            node->next_ = oldHead;
        } while (!LF_queueHead_.compare_exchange_weak(oldHead, node, std::memory_order_release, std::memory_order_relaxed));
    }
    else
    {
        LOG_WARN( "a null node can be pushed to queue");
    }
    return;
}

template<typename T>
LF_node_t<T>* lockFreeStack<T>::popNodeWithHazard()
{
    LF_node_t<T> *oldNode = nullptr;
    LF_node_t<T> *headNode = nullptr;

    //Use hazard pointer to avoid the ABA problem
    oldNode = LF_queueHead_.load(std::memory_order_acquire);
    do
    {
        if(oldNode == nullptr)
        {
            LOG_WARN("load the LF queue head is empty");
            break;
        }
        else
        {
            do{
                headNode = oldNode;
                hazardPointerQueue_.setHazardPointer(headNode);
                oldNode = LF_queueHead_.load(std::memory_order_acquire);
            } while (oldNode != headNode);
            if(oldNode != nullptr && LF_queueHead_.compare_exchange_strong(oldNode, oldNode->next_, std::memory_order_release, std::memory_order_relaxed))
            {
                // Waiting hazard gone.
                while(1)
                {
                    if(hazardPointerQueue_.findConflictPointer(oldNode) == true)
                    {
                        break;
                    }
                    std::this_thread::yield();
                }
                break;
            }
        }
    } while (1);

    if(oldNode == nullptr)
    {
        LOG_WARN("loop the LF queue to end ");
        return nullptr;
    }

    //Eliminate the hazard
    hazardPointerQueue_.removeHazardPointer();
    oldNode->next_ = nullptr;
    return oldNode;
}

}

#endif
