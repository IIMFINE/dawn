#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "threadPool.h"
#include "type.h"
#include <sys/time.h>
#include "lockFree.h"
#include <atomic>
#include <thread>
#include <vector>
#include <list>
#include "setLogger.h"

namespace net
{

template<typename T>
lockFreeStack<T>::lockFreeStack():
LF_queueHead(0)
{
}

template<typename T>
void lockFreeStack<T>::init(LF_node_t<T>* queueHead)
{
    LF_queueHead.store(queueHead);
}

template<typename T>
void lockFreeStack<T>::pushNode(LF_node_t<T> *node)
{
    if(node != nullptr)
    {
        node->next = LF_queueHead.load();
        while(!LF_queueHead.compare_exchange_weak(node->next, node, std::memory_order_seq_cst));
    }
    else
    {
        LOG4CPLUS_WARN(DAWN_LOG, "warn: a null node is pushed to queue");
    }
    return;
}

template<typename T>
LF_node_t<T>* lockFreeStack<T>::popNode()
{
    LF_node_t<T> *oldNode = nullptr;

    //avoid the ABA problem
    int tempRecordRefCnt = 0;
    do
    {
        do
        {
            oldNode = LF_queueHead.load();
            if(oldNode != nullptr)
            {
                tempRecordRefCnt = oldNode->refcnt.load();
            }
            else
            {
                LOG4CPLUS_WARN(DAWN_LOG, "warn: load the LF queue head is empty");
                break;
            }
        }while(!LF_queueHead.compare_exchange_strong(oldNode, oldNode->next, std::memory_order_seq_cst));
    } while(oldNode != nullptr && tempRecordRefCnt != oldNode->refcnt.load());
    
    if(oldNode == nullptr)
    {
        LOG4CPLUS_WARN(DAWN_LOG,"warn: traverse the LF queue to end ");
        return nullptr;
    }
    else
    {
        oldNode->refcnt.fetch_add(1);
    }
    return oldNode;
}

}




