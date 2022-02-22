#ifndef _LOCK_FREE_H_
#define _LOCK_FREE_H_
#include <atomic>
#include "type.h"
namespace net
{

template<typename T>
struct  LF_node_t
{
    LF_node_t()
    {
        refcnt.store(0);
        next = nullptr;
        elementVal = nullptr;
    }
    T* elementVal;
    LF_node_t *next;
    std::atomic<int>  refcnt;

};

//dont use smart point which uses too many memory resource 
template<typename T>
class lockFreeStack
{
std::atomic<LF_node_t<T>*>         LF_queueHead;

public:
    lockFreeStack();
    ~lockFreeStack()=default;
    void init(LF_node_t<T>* queueHead);
    LF_node_t<T>* popNode();
    void pushNode(LF_node_t<T> *node);
};

template class lockFreeStack<memoryNode_t>;

}





#endif