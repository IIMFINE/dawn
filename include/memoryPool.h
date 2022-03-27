#ifndef _MEMORY_POOL_H_
#define _MEMORY_POOL_H_
#include "lockFree.h"
#include <atomic>
#include "type.h"
#include <list>

namespace net
{
const int MIN_MEM_BLOCK_TYPE = 7;
const int MAX_MEM_BLOCK_TYPE = 11;
const int MIN_MEM_BLOCK = 1<<MIN_MEM_BLOCK_TYPE;           //128byte, satify to size of cache line buff.
const int MAX_MEM_BLOCK = 1<<MAX_MEM_BLOCK_TYPE;        //2048byte, satify to udp mtu.

const int TOTAL_MEM_EACH_TYPE = (128<<20);    //total memory of each type is 128Mbyte

class  memoryPool
{
public:
    memoryPool();
    ~memoryPool();
    void init();
    memoryNode_t* allocMemBlock(const int requestMemSize);
    bool freeMemBlock(memoryNode_t* realseMemBlock);
private:
    lockFreeStack<memoryNode_t>    memoryStoreQueue[MAX_MEM_BLOCK_TYPE - MIN_MEM_BLOCK_TYPE + 1];
    lockFreeStack<memoryNode_t>    storeEmptyLFQueue;
    std::list<void*>               allCreateMem;
};

memoryNode_t* allocMem(const int requestMemSize);
bool freeMem(memoryNode_t* realseMemBlock);
void memPoolInit();
}

#endif