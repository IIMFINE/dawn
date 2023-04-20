#include <iostream>
#include "memoryPool.h"
#include "stdlib.h"
#include "string.h"
#include "baseOperator.h"
#include <cassert>

namespace dawn
{

  memoryPool::memoryPool()
  {
    storeEmptyLFQueue_.init(nullptr);
  }

  void memoryPool::init()
  {
    memoryNode_t *mallocMemory = nullptr;
    LF_node_t<memoryNode_t *> *mallocLFNode = nullptr;
    for (int i = MIN_MEM_BLOCK_TYPE; i <= MAX_MEM_BLOCK_TYPE; ++i)
    {
      int memBlockSize = 1 << i;
      int totalMemBlockNum = TOTAL_MEM_SIZE / memBlockSize;
      mallocMemory = (memoryNode_t *)malloc(TOTAL_MEM_SIZE + totalMemBlockNum * (sizeof(memoryNode_t)));
      mallocLFNode = new LF_node_t<memoryNode_t *>[totalMemBlockNum];

      allCreateMem_.push_back((void *)mallocMemory);
      allCreateMem_.push_back((void *)mallocLFNode);

      memset(mallocMemory, 0x0, TOTAL_MEM_SIZE);
      if (mallocMemory != nullptr && mallocLFNode != nullptr)
      {
        char *tempMallocMemory = reinterpret_cast<char *>(mallocMemory);
        for (int j = 0; j < totalMemBlockNum; j++)
        {
          mallocLFNode[j].elementVal_ = reinterpret_cast<memoryNode_t *>\
            (tempMallocMemory + (memBlockSize + sizeof(memoryNode_t)) * j);
          mallocLFNode[j].elementVal_->memoryBlockType_ = i;
          mallocLFNode[j].next_ = &mallocLFNode[j + 1];
        }
        mallocLFNode[totalMemBlockNum - 1].next_ = nullptr;
        memoryStoreQueue_[i - MIN_MEM_BLOCK_TYPE].init(mallocLFNode);
      }
      LOG_INFO("init memory pool type {} block num {}", i, totalMemBlockNum);
    }
  }

  memoryPool::~memoryPool()
  {
    for (auto freeMemIt : allCreateMem_)
    {
      free(freeMemIt);
    }
    allCreateMem_.clear();
  }

  memoryNode_t *memoryPool::allocMemBlock(const int requestMemSize)
  {
    if (requestMemSize <= 0)
    {
      return nullptr;
    }

    if (requestMemSize > MAX_MEM_BLOCK)
    {
      LOG_WARN("error: request to alloc is too large {}", requestMemSize);
      return nullptr;
    }
    int countRequestMemType = 0;
    if (requestMemSize <= MIN_MEM_BLOCK)
    {
      countRequestMemType = MIN_MEM_BLOCK_TYPE;
    }
    else
    {
      countRequestMemType = getTopBitPosition((uint32_t)requestMemSize);
      if (((requestMemSize - 1) & requestMemSize) != 0)
      {
        countRequestMemType += 1;
      }
    }

    LF_node_t<memoryNode_t *> *memNodeContain = memoryStoreQueue_[countRequestMemType - MIN_MEM_BLOCK_TYPE].popNodeWithHazard();

    if (memNodeContain == nullptr)
    {
      LOG_WARN("alloc fail {} {}", countRequestMemType, requestMemSize);
      return nullptr;
    }
    memNodeContain->next_ = nullptr;
    memoryNode_t *memNode = memNodeContain->elementVal_;
    memNodeContain->elementVal_ = nullptr;
    storeEmptyLFQueue_.pushNode(memNodeContain);
    return memNode;
  }

  bool memoryPool::freeMemBlock(memoryNode_t *releaseMemBlock)
  {
    if (releaseMemBlock == nullptr)
    {
      LOG_ERROR("error: ask to release memory block is nullptr ");
      return PROCESS_FAIL;
    }
    int memBlockType = releaseMemBlock->memoryBlockType_;
    LF_node_t<memoryNode_t *> *availableContain = nullptr;
    availableContain = storeEmptyLFQueue_.popNodeWithHazard();

    if (availableContain == nullptr)
    {
      LOG_WARN("warn: no enough contain to save memory block and be care about some error happen");
      availableContain = new LF_node_t<memoryNode_t *>;
    }

    availableContain->elementVal_ = releaseMemBlock;
    memoryStoreQueue_[memBlockType - MIN_MEM_BLOCK_TYPE].pushNode(availableContain);
    return PROCESS_SUCCESS;
  }

  threadLocalMemoryPool::threadLocalMemoryPool()
  {
    initialize();
  }

  bool threadLocalMemoryPool::initialize()
  {
    /// @note call for safety
    memPoolInit();

    for (int i = 0; i < MAX_MEM_BLOCK_TYPE - MIN_MEM_BLOCK_TYPE + 1; i++)
    {
      auto block_size = BLOCK_SIZE((i + MIN_MEM_BLOCK_TYPE));
      int j = 0;
      for (j = 0;j <= highWaterMark_.array[i]; j++)
      {
        memBlockQueue_[i][j] = lowLevelAllocMem(block_size);
        if (memBlockQueue_[i][j] == nullptr)
        {
          LOG_WARN("thread local memory pool can not finish initialize, alloc mem type {}", i);
          break;
        }
      }

      if (j > 0)
      {
        watermarkIndex_[i] = (--j);
      }
    }
    return PROCESS_SUCCESS;
  }

  threadLocalMemoryPool::~threadLocalMemoryPool()
  {
    for (int i = MIN_MEM_BLOCK_TYPE; i <= MAX_MEM_BLOCK_TYPE;i++)
    {
      auto queueIndex = MEM_TYPE_TO_QUEUE_INDEX(i);
      for (int j = watermarkIndex_[queueIndex];j >= 0;j--)
      {
        lowLevelFreeMem(memBlockQueue_[queueIndex][j]);
      }
    }
  }

  void* threadLocalMemoryPool::TL_allocMem(const int requestMemSize)
  {
    assert(requestMemSize > 0 && "can not request non positive size memory");

    if (requestMemSize > MAX_MEM_BLOCK)
    {
      LOG_WARN("error: request to alloc is too large {}", requestMemSize);
      return nullptr;
    }

    int countRequestMemType = 0;
    if (requestMemSize <= MIN_MEM_BLOCK)
    {
      countRequestMemType = MIN_MEM_BLOCK_TYPE;
    }
    else
    {
      countRequestMemType = getTopBitPosition((uint32_t)requestMemSize);
      ///@note if requestMemSize is bigger than 2 to the power of countRequestMemType, countRequestMemType will add 1
      if (((requestMemSize - 1) & requestMemSize) != 0)
      {
        ++countRequestMemType;
      }
    }
    LOG_DEBUG("alloc memory type {}", countRequestMemType);
    decWaterMark(countRequestMemType);
    auto memoryPtr = getMemBlockFromQueue(countRequestMemType);
    if (memoryPtr == nullptr)
    {
      throw std::runtime_error("dawn: alloc a nullptr memory ptr");
    }
    return memoryPtr;
  }

  void* threadLocalMemoryPool::getMemBlockFromQueue(int blockType)
  {
    auto queueIndex = MEM_TYPE_TO_QUEUE_INDEX(blockType);
    return memBlockQueue_[queueIndex][watermarkIndex_[queueIndex]--];
  }

  bool threadLocalMemoryPool::incWaterMark(int blockType)
  {
    auto queueIndex = MEM_TYPE_TO_QUEUE_INDEX(blockType);
    auto waterMarkIndex = watermarkIndex_[queueIndex];

    if (waterMarkIndex < threadLocalMemoryPool::highWaterMark_.array[queueIndex])
    {
      return PROCESS_SUCCESS;
    }

    if (waterMarkIndex > threadLocalMemoryPool::highWaterMark_.array[queueIndex]
      && waterMarkIndex < (threadLocalMemoryPool::highWaterMark_.array[0] + threadLocalMemoryPool::lowWaterMark_.array[0] - 1))
    {
      for (int i = waterMarkIndex; i >= threadLocalMemoryPool::highWaterMark_.array[queueIndex]; i--)
      {
        auto memBlock = memBlockQueue_[queueIndex][watermarkIndex_[queueIndex]];
        if (lowLevelFreeMem(memBlock) == PROCESS_FAIL)
        {
          LOG_WARN("can not decrease memory block in thread local pool, {} type", blockType);
          return PROCESS_FAIL;
        }
        else
        {
          --watermarkIndex_[queueIndex];
        }
      }
    }
    else if (waterMarkIndex >= (threadLocalMemoryPool::highWaterMark_.array[0] + threadLocalMemoryPool::lowWaterMark_.array[0]))
    {
      throw std::runtime_error("dawn: thread local memory pool is overflow");
    }
    else if (waterMarkIndex == (threadLocalMemoryPool::highWaterMark_.array[0] + threadLocalMemoryPool::lowWaterMark_.array[0] - 1))
    {
      bool waterChangeFlag = false;
      for (int i = waterMarkIndex; i > threadLocalMemoryPool::highWaterMark_.array[queueIndex]; i--)
      {
        auto memBlock = memBlockQueue_[queueIndex][watermarkIndex_[queueIndex]];
        if (lowLevelFreeMem(memBlock) == PROCESS_FAIL)
        {
          LOG_WARN("can not free memory");
          if (waterChangeFlag == false)
          {
            throw std::runtime_error("dawn: thread local memory pool will be overflow");
          }
          return PROCESS_FAIL;
        }
        else
        {
          waterChangeFlag = true;
          --watermarkIndex_[queueIndex];
        }
      }
    }
    return PROCESS_SUCCESS;
  }

  bool threadLocalMemoryPool::decWaterMark(int blockType)
  {
    auto queueIndex = MEM_TYPE_TO_QUEUE_INDEX(blockType);
    auto waterMarkIndex = watermarkIndex_[queueIndex];

    if (waterMarkIndex > threadLocalMemoryPool::lowWaterMark_.array[queueIndex])
    {
      return PROCESS_SUCCESS;
    }

    if (waterMarkIndex < threadLocalMemoryPool::lowWaterMark_.array[queueIndex] && waterMarkIndex > 0)
    {
      for (int i = waterMarkIndex; i <= threadLocalMemoryPool::lowWaterMark_.array[queueIndex]; i++)
      {
        if (auto memBlock = lowLevelAllocMem(blockType); memBlock != nullptr)
        {
          ++watermarkIndex_[queueIndex];
          memBlockQueue_[queueIndex][watermarkIndex_[queueIndex]] = memBlock;
        }
        else
        {
          LOG_WARN("can not supplement thread local memory {} type", blockType);
          return PROCESS_FAIL;
        }
      }
    }
    else if (waterMarkIndex == 0)
    {
      bool waterChangeFlag = false;
      for (int i = waterMarkIndex; i <= threadLocalMemoryPool::lowWaterMark_.array[queueIndex]; i++)
      {
        if (auto memBlock = lowLevelAllocMem(blockType); memBlock != nullptr)
        {
          ++watermarkIndex_[queueIndex];
          memBlockQueue_[queueIndex][watermarkIndex_[queueIndex]] = memBlock;
          waterChangeFlag = true;
        }
        else
        {
          if (waterChangeFlag == false)
          {
            LOG_WARN("can not supplement thread local memory {} type", blockType);
            throw std::runtime_error("dawn: there will be a breakdown in thread local memory");
          }
          else
          {
            return PROCESS_FAIL;
          }
        }
      }
    }
    else if (waterMarkIndex < 0)
    {
      throw std::runtime_error("dawn: there is a breakdown in thread local memory");
    }
    return PROCESS_SUCCESS;
  }


  void memPoolInit()
  {
    auto initMemPool = []() {
      singleton<dawn::memoryPool>::getInstance()->init();
      return PROCESS_SUCCESS;
    };
    static auto init_bait = initMemPool();
    (void)init_bait;
  }

  void* lowLevelAllocMem(const int requestMemSize)
  {
    auto memoryNode = singleton<dawn::memoryPool>::getInstance()->allocMemBlock(requestMemSize);
    return reinterpret_cast<void*>(memoryNode->memoryHead_);
  }

  bool lowLevelFreeMem(memoryNode_t *releaseMemBlock)
  {
    if (releaseMemBlock == nullptr)
    {
      throw std::runtime_error("dawn: try to release nullptr");
    }
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlock(releaseMemBlock);
  }

  bool lowLevelFreeMem(void *memHead)
  {
    assert(memHead != nullptr && "free a nullptr");
    memoryNode_t* releaseMemNodeHeader = reinterpret_cast<memoryNode_t*>(reinterpret_cast<char*>(memHead) - 1);
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlock(releaseMemNodeHeader);
  }

  threadLocalMemoryPool& getThreadLocalPool()
  {
    static thread_local threadLocalMemoryPool TL_memoryPool;
    return TL_memoryPool;
  }

  void* allocMem(const int requestMemSize)
  {
    auto& memoryPool = getThreadLocalPool();
    return memoryPool.TL_allocMem(requestMemSize);
  }
}
