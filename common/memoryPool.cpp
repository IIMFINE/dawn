#include <iostream>
#include "memoryPool.h"
#include "stdlib.h"
#include "string.h"
#include "baseOperator.h"
#include "setLogger.h"

namespace net
{

memoryPool::memoryPool()
{
    storeEmptyLFQueue.init(nullptr);
}
void memoryPool::init()
{
    memoryNode_t *mallocMemory = nullptr;
    LF_node_t<memoryNode_t>  *mallocLFNode = nullptr;
    std::atomic<LF_node_t<memoryNode_t>*>         LF_queueHead;
    for(int i = MIN_MEM_BLOCK_TYPE; i <= MAX_MEM_BLOCK_TYPE; ++i)
    {
        int memBlockSize = 1 << i;
        int totalMemBlockNum = TOTAL_MEM_EACH_TYPE / memBlockSize;
        mallocMemory = (memoryNode_t*)malloc(TOTAL_MEM_EACH_TYPE);
        mallocLFNode = new LF_node_t<memoryNode_t>[totalMemBlockNum];
        memset(mallocMemory, 0x0, TOTAL_MEM_EACH_TYPE);
        LOG4CPLUS_DEBUG(DAWN_LOG, "init memory pool type "<< i <<" block num "<< totalMemBlockNum);
        if(mallocMemory != nullptr && mallocLFNode != nullptr)
        {
            for(int j = 0; j < totalMemBlockNum; j++)
            {
                char *tempMallocMemory = reinterpret_cast<char*>(mallocMemory);
                // ((memoryNode_t*)(&tempMallocMemory[(i - MIN_MEM_BLOCK_TYPE)*j]))->memoryBlockType = i;
                mallocLFNode[j].elementVal = reinterpret_cast<memoryNode_t*>(&tempMallocMemory[memBlockSize*j]);
                mallocLFNode[j].elementVal->memoryBlockType = i;
                mallocLFNode[j].next = &mallocLFNode[j + 1];
            }
            mallocLFNode[totalMemBlockNum - 1].next = nullptr;
            memoryStoreQueue[i - MIN_MEM_BLOCK_TYPE].init(mallocLFNode);
            LOG4CPLUS_DEBUG(DAWN_LOG, "i alloc "<<i);
        }
    }
}

memoryNode_t* memoryPool::allocMemBlock(const int requestMemSize)
{
    if(requestMemSize <= 0)
    {
        return nullptr;
    }
    int translateMemSize = requestMemSize;
    translateMemSize += sizeof(memoryNode_t);  //because struct memoryNode_t will also occupy some memory
    if(translateMemSize > MAX_MEM_BLOCK)
    {
        LOG4CPLUS_WARN(DAWN_LOG, "error: request to alloc is too large "<< translateMemSize);
        return nullptr;
    }
    int countRequestMemType = 0;
    if(translateMemSize <= MIN_MEM_BLOCK)
    {
        countRequestMemType = MIN_MEM_BLOCK_TYPE;
    }
    else
    {
        countRequestMemType = getTopBitPostion((uint32_t)translateMemSize);
        if(((translateMemSize - 1) & translateMemSize) != 0)
        {
            countRequestMemType += 1;
        }
    }
    // LOG4CPLUS_WARN(DAWN_LOG, "alloc memtype "<< countRequestMemType);
    LF_node_t<memoryNode_t> *memNodeContain = memoryStoreQueue[countRequestMemType - MIN_MEM_BLOCK_TYPE].popNode();
    if(memNodeContain == nullptr)
    {
        LOG4CPLUS_WARN(DAWN_LOG, "alloc fail "<< countRequestMemType <<" "<< translateMemSize);
        return nullptr;
    }
    memNodeContain->next = nullptr;
    memoryNode_t *memNode =  memNodeContain->elementVal;
    memNodeContain->elementVal = nullptr;
    storeEmptyLFQueue.pushNode(memNodeContain);
    return memNode;
}

bool memoryPool::freeMemBlock(memoryNode_t *realseMemBlock)
{
    if(realseMemBlock == nullptr)
    {
        LOG4CPLUS_WARN(DAWN_LOG, "error: ask to realse memory block is nullptr ");
        return PROCESS_FAIL;
    }
    int memBlockType = realseMemBlock->memoryBlockType;
    LF_node_t<memoryNode_t>  *availableContain = nullptr;
    availableContain = storeEmptyLFQueue.popNode();
    if(availableContain == nullptr)
    {
        LOG4CPLUS_WARN(DAWN_LOG, "warn: no enough contain to save memory block and be care about some error happen");
        availableContain = new LF_node_t<memoryNode_t>;
    }
    availableContain->elementVal = realseMemBlock;
    memoryStoreQueue[memBlockType - MIN_MEM_BLOCK_TYPE].pushNode(availableContain);
    return PROCESS_SUCCESS;
}

void memPoolInit()
{
    return singleton<net::memoryPool>::getInstance()->init();
}

memoryNode_t* allocMem(const int requestMemSize)
{
    return singleton<net::memoryPool>::getInstance()->allocMemBlock(requestMemSize);
}

bool freeMem(memoryNode_t* realseMemBlock)
{
    return singleton<net::memoryPool>::getInstance()->freeMemBlock(realseMemBlock);
}

}
