#ifndef _LOCK_FREE_H_
#define _LOCK_FREE_H_
#include <atomic>
#include <thread>

#include "hazardPointer.h"
#include "setLogger.h"
#include "type.h"
namespace dawn
{

    template <typename T>
    struct LF_Node
    {
        LF_Node() : element_val_(nullptr),
                    next_(nullptr)
        {
        }

        LF_Node(T &&val) : element_val_(std::forward<T>(val)),
                           next_(nullptr)
        {
        }

        T element_val_;
        LF_Node *next_;
    };

    // dont use smart point which uses too many memory resource
    template <typename T>
    class LockFreeStack
    {
        std::atomic<LF_Node<T> *> LF_queue_head_;
        threadLocalHazardPointerQueue<LF_Node<T> *> hazard_pointer_queue_;

    public:
        LockFreeStack();
        ~LockFreeStack() = default;
        void init(LF_Node<T> *queue_head);
        LF_Node<T> *popNodeWithHazard();
        void pushNode(LF_Node<T> *node);
    };

    template <typename T>
    LockFreeStack<T>::LockFreeStack() : LF_queue_head_(static_cast<LF_Node<T> *>(nullptr))
    {
    }

    template <typename T>
    void LockFreeStack<T>::init(LF_Node<T> *queue_head)
    {
        LF_queue_head_.store(queue_head);
    }

    template <typename T>
    void LockFreeStack<T>::pushNode(LF_Node<T> *node)
    {
        assert(node != nullptr && "a null node can be pushed to queue");
        auto oldHead = LF_queue_head_.load(std::memory_order_acquire);
        do
        {
            node->next_ = oldHead;
        } while (!LF_queue_head_.compare_exchange_weak(oldHead, node, std::memory_order_release, std::memory_order_relaxed));
    }

    template <typename T>
    LF_Node<T> *LockFreeStack<T>::popNodeWithHazard()
    {
        LF_Node<T> *oldNode = nullptr;
        LF_Node<T> *headNode = nullptr;

        // Use hazard pointer to avoid the ABA problem
        oldNode = LF_queue_head_.load(std::memory_order_acquire);
        do
        {
            if (oldNode == nullptr)
            {
                LOG_WARN("load the LF queue head is empty");
                break;
            }
            else
            {
                do
                {
                    headNode = oldNode;
                    oldNode = LF_queue_head_.load(std::memory_order_acquire);
                    hazard_pointer_queue_.setHazardPointer(oldNode);
                } while (oldNode != headNode);
                if (oldNode != nullptr && LF_queue_head_.compare_exchange_strong(oldNode, oldNode->next_, std::memory_order_release, std::memory_order_relaxed))
                {
                    // Waiting hazard gone.
                    while (1)
                    {
                        if (hazard_pointer_queue_.findConflictPointer(oldNode) == true)
                        {
                            break;
                        }
                        std::this_thread::yield();
                    }
                    break;
                }
            }
        } while (1);

        if (oldNode == nullptr)
        {
            LOG_WARN("loop the LF queue to end ");
            return nullptr;
        }

        // Eliminate the hazard
        hazard_pointer_queue_.removeHazardPointer();
        oldNode->next_ = nullptr;
        return oldNode;
    }
}

#endif
