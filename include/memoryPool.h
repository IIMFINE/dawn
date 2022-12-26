#ifndef _MEMORY_POOL_H_
#define _MEMORY_POOL_H_
#include "lockFree.h"
#include <atomic>
#include "type.h"
#include <list>
#include "setLogger.h"
namespace dawn
{
const int MIN_MEM_BLOCK_TYPE = 7;
const int MAX_MEM_BLOCK_TYPE = 11;
const int MIN_MEM_BLOCK = 1<<MIN_MEM_BLOCK_TYPE;           //128byte, satisfy to size of cache line buff.
const int MAX_MEM_BLOCK = 1<<MAX_MEM_BLOCK_TYPE;        //2048byte, satisfy to udp mtu.

const int TOTAL_MEM_SIZE = (128<<20);    //total memory of each type is 128M byte

#define QUEUE_INDEX(BLOCK_TYPE) BLOCK_TYPE < MIN_MEM_BLOCK_TYPE

#pragma pack (1)
struct memoryNode_t
{
    char      memoryBlockType_;
    char      memoryHead_[0];
};
#pragma pack ()

class  memoryPool
{
public:
    memoryPool();
    ~memoryPool();
    void init();
    memoryNode_t* allocMemBlock(const int requestMemSize);
    bool freeMemBlock(memoryNode_t* releaseMemBlock);
private:
    lockFreeStack<memoryNode_t>    memoryStoreQueue_[MAX_MEM_BLOCK_TYPE - MIN_MEM_BLOCK_TYPE + 1];
    lockFreeStack<memoryNode_t>    storeEmptyLFQueue_;
    std::list<void*>               allCreateMem_;
};

memoryNode_t* allocMem(const int requestMemSize);
bool freeMem(memoryNode_t* releaseMemBlock);
void memPoolInit();
}

#endif
