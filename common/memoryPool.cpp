#include <iostream>
#include "memoryPool.h"
#include "stdlib.h"
#include "string.h"
#include "baseOperator.h"

namespace dawn
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
        int totalMemBlockNum = TOTAL_MEM_SIZE / memBlockSize;
        mallocMemory = (memoryNode_t*)malloc(TOTAL_MEM_SIZE);
        mallocLFNode = new LF_node_t<memoryNode_t>[totalMemBlockNum];

        allCreateMem.push_back((void*)mallocMemory);
        allCreateMem.push_back((void*)mallocLFNode);

        memset(mallocMemory, 0x0, TOTAL_MEM_SIZE);
        if(mallocMemory != nullptr && mallocLFNode != nullptr)
        {
            char *tempMallocMemory = reinterpret_cast<char*>(mallocMemory);
            for(int j = 0; j < totalMemBlockNum; j++)
            {
                mallocLFNode[j].elementVal_ = reinterpret_cast<memoryNode_t*>(tempMallocMemory + memBlockSize*j);
                mallocLFNode[j].elementVal_->memoryBlockType_ = i;
                mallocLFNode[j].next_ = &mallocLFNode[j + 1];
            }
            mallocLFNode[totalMemBlockNum - 1].next_ = nullptr;
            memoryStoreQueue[i - MIN_MEM_BLOCK_TYPE].init(mallocLFNode);
        }
        LOG_INFO("init memory pool type {} block num {}", i, totalMemBlockNum);
    }
}

memoryPool::~memoryPool()
{
    for(auto freeMem : allCreateMem)
    {
        free(freeMem);
    }
    allCreateMem.clear();
}

memoryNode_t* memoryPool::allocMemBlock(const int requestMemSize)
{
    if(requestMemSize <= 0)
    {
        return nullptr;
    }
    int translateMemSize = requestMemSize;
    translateMemSize += sizeof(memoryNode_t);  //Because struct memoryNode_t will also occupy some memory
    if(translateMemSize > MAX_MEM_BLOCK)
    {
        LOG_WARN("error: request to alloc is too large {}", translateMemSize);
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

    LOG_DEBUG("alloc memory type {}", countRequestMemType);
    LF_node_t<memoryNode_t> *memNodeContain = memoryStoreQueue[countRequestMemType - MIN_MEM_BLOCK_TYPE].popNode();

    if(memNodeContain == nullptr)
    {
        LOG_WARN("alloc fail {} {}", countRequestMemType, translateMemSize);
        return nullptr;
    }
    memNodeContain->next_ = nullptr;
    memoryNode_t *memNode =  memNodeContain->elementVal_;
    memNodeContain->elementVal_ = nullptr;
    storeEmptyLFQueue.pushNode(memNodeContain);
    return memNode;
}

bool memoryPool::freeMemBlock(memoryNode_t *releaseMemBlock)
{
    if(releaseMemBlock == nullptr)
    {
        LOG_ERROR("error: ask to release memory block is nullptr ");
        return PROCESS_FAIL;
    }
    int memBlockType = releaseMemBlock->memoryBlockType_;
    LF_node_t<memoryNode_t>  *availableContain = nullptr;
    availableContain = storeEmptyLFQueue.popNode();

    if(availableContain == nullptr)
    {
        LOG_WARN("warn: no enough contain to save memory block and be care about some error happen");
        availableContain = new LF_node_t<memoryNode_t>;
    }

    availableContain->elementVal_ = releaseMemBlock;
    memoryStoreQueue[memBlockType - MIN_MEM_BLOCK_TYPE].pushNode(availableContain);
    return PROCESS_SUCCESS;
}

void memPoolInit()
{
    return singleton<dawn::memoryPool>::getInstance()->init();
}

memoryNode_t* allocMem(const int requestMemSize)
{
    return singleton<dawn::memoryPool>::getInstance()->allocMemBlock(requestMemSize);
}

bool freeMem(memoryNode_t* releaseMemBlock)
{
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlock(releaseMemBlock);
}

}
