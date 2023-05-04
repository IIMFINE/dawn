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
    for (int i = 0; i < MAX_MULTIPLE; ++i)
    {
      int memBlockSize = BASE_BLOCK_SIZE * (i + 1);
      int totalMemBlockNum = EACH_BLOCK_TOTAL_NUM.array[i];
      mallocMemory = (memoryNode_t *)malloc(totalMemBlockNum * (sizeof(memoryNode_t) + memBlockSize));
      mallocLFNode = new LF_node_t<memoryNode_t *>[totalMemBlockNum];

      allCreateMem_.push_back((void *)mallocMemory);
      allCreateMem_.push_back((void *)mallocLFNode);

      memset(mallocMemory, 0x0, (sizeof(memoryNode_t) + memBlockSize));
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
        memoryStoreQueue_[i].init(mallocLFNode);
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

  memoryNode_t* memoryPool::allocMemBlock(const int requestMemSize)
  {
    assert(requestMemSize > 0 && "error: requestMemSize is less than 0");

    int memMask = GET_MEM_QUEUE_MASK(requestMemSize);

    LF_node_t<memoryNode_t *> *memNodeContainer = memoryStoreQueue_[memMask].popNodeWithHazard();

    if (memNodeContainer == nullptr)
    {
      LOG_WARN("alloc fail {} {}", memMask, requestMemSize);
      return nullptr;
    }
    memNodeContainer->next_ = nullptr;
    memoryNode_t *memNode = memNodeContainer->elementVal_;
    memNodeContainer->elementVal_ = nullptr;
    storeEmptyLFQueue_.pushNode(memNodeContainer);
    return memNode;
  }

  bool memoryPool::freeMemBlock(memoryNode_t *releaseMemBlock)
  {
    assert(releaseMemBlock != nullptr && "error: ask to release memory block is nullptr ");

    int memBlockType = releaseMemBlock->memoryBlockType_;
    LF_node_t<memoryNode_t *> *availableContain = nullptr;
    availableContain = storeEmptyLFQueue_.popNodeWithHazard();

    assert(availableContain != nullptr && "error: no enough contain to save memory block and be care about some error happen");

    availableContain->elementVal_ = releaseMemBlock;
    memoryStoreQueue_[memBlockType].pushNode(availableContain);
    return PROCESS_SUCCESS;
  }

  LF_node_t<memoryNode_t *>* memoryPool::allocMemBlockWithContainer(const int requestMemSize)
  {
    assert(requestMemSize > 0 && "error: requestMemSize is less than 0");

    int memMask = GET_MEM_QUEUE_MASK(requestMemSize);

    LF_node_t<memoryNode_t *> *memNodeContainer = memoryStoreQueue_[memMask].popNodeWithHazard();

    if (memNodeContainer == nullptr)
    {
      LOG_WARN("alloc fail {} {}", memMask, requestMemSize);
      return nullptr;
    }
    memNodeContainer->next_ = nullptr;
    return memNodeContainer;
  }

  bool memoryPool::freeMemBlockWithContainer(LF_node_t<memoryNode_t *> *releaseMemBlock)
  {
    assert(releaseMemBlock != nullptr && "error: ask to release memory block is nullptr ");

    int memBlockType = releaseMemBlock->elementVal_->memoryBlockType_;
    memoryStoreQueue_[memBlockType].pushNode(releaseMemBlock);
    return PROCESS_SUCCESS;
  }

  bool memoryPool::freeEmptyContainer(LF_node_t<memoryNode_t *> *releaseContainer)
  {
    assert(releaseContainer != nullptr && "error: ask to release memory container is nullptr ");

    storeEmptyLFQueue_.pushNode(releaseContainer);
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

    for (int i = 0; i < MAX_MULTIPLE; i++)
    {
      auto block_size = BLOCK_SIZE(i);
      int j = 0;
      for (j = 0;j <= highWaterMark_.array[i]; j++)
      {
        auto blockContainer = lowLevelAllocMem(block_size);
        if (blockContainer != nullptr)
        {
          if (memBlockList_[i] == nullptr)
          {
            memBlockList_[i] = blockContainer;
          }
          else
          {
            blockContainer->next_ = memBlockList_[i];
            memBlockList_[i] = blockContainer;
          }
        }
        else
        {
          j--;
          break;
        }
      }

      if (j > highWaterMark_.array[i])
      {
        watermarkIndex_[i] = j - 1;
      }
      else
      {
        watermarkIndex_[i] = j;
      }
    }
    return PROCESS_SUCCESS;
  }

  threadLocalMemoryPool::~threadLocalMemoryPool()
  {
    for (int i = 0;i < MAX_MULTIPLE;i++)
    {
      for (auto memBlock = memBlockList_[i]; memBlock != nullptr;)
      {
        auto nextMemBlock = memBlock->next_;
        lowLevelFreeMem(memBlock);
        memBlock = nextMemBlock;
      }
    }
    for (auto container = remainMemBlockContainerList_; container != nullptr;)
    {
      auto nextContainer = container->next_;
      lowLevelFreeEmptyContainer(container);
      container = nextContainer;
    }
  }

  void* threadLocalMemoryPool::TL_allocMem(const int requestMemSize)
  {
    assert(requestMemSize > 0 && "can not request non positive size memory");
    int memMask = GET_MEM_QUEUE_MASK(requestMemSize);

    ///@note Use branch prediction
    if (memMask < MAX_MULTIPLE)
    {
      decWaterMark(memMask);
      auto memoryPtr = getMemBlockFromQueue(memMask);
      return memoryPtr;
    }
    else
    {
      LOG_WARN("error: request to alloc is too large {}", requestMemSize);
      return nullptr;
    }
    return nullptr;
  }

  void* threadLocalMemoryPool::getMemBlockFromQueue(int blockType)
  {
    --watermarkIndex_[blockType];
    auto memContainer = memBlockList_[blockType];
    assert(memContainer != nullptr && "error: get memory block from queue is nullptr");
    memBlockList_[blockType] = memContainer->next_;
    auto memBlock = memContainer->elementVal_;
    memContainer->elementVal_ = nullptr;
    memContainer->next_ = remainMemBlockContainerList_;
    remainMemBlockContainerList_ = memContainer;
    return memBlock->memoryHead_;
  }

  bool threadLocalMemoryPool::recycleMemBlockToQueue(memoryNode_t* memBlock, int blockType)
  {
    watermarkIndex_[blockType]++;
    if (remainMemBlockContainerList_ != nullptr)
    {
      auto emptyContainer = remainMemBlockContainerList_;
      remainMemBlockContainerList_ = remainMemBlockContainerList_->next_;
      emptyContainer->elementVal_ = memBlock;
      emptyContainer->next_ = memBlockList_[blockType];
      memBlockList_[blockType] = emptyContainer;
      return PROCESS_SUCCESS;
    }
    else
    {
      auto emptyContainer = new LF_node_t<memoryNode_t *>;
      emptyContainer->elementVal_ = memBlock;
      emptyContainer->next_ = memBlockList_[blockType];
      memBlockList_[blockType] = emptyContainer;
      return PROCESS_SUCCESS;
    }
  }

  bool threadLocalMemoryPool::incWaterMark(int blockType)
  {
    auto waterMarkIndex = watermarkIndex_[blockType];

    if (waterMarkIndex < threadLocalMemoryPool::highWaterMark_.array[blockType] + threadLocalMemoryPool::lowWaterMark_.array[blockType])
    {
      return PROCESS_SUCCESS;
    }

    for (int i = waterMarkIndex; i >= threadLocalMemoryPool::highWaterMark_.array[blockType]; i--)
    {
      auto memContainer = memBlockList_[blockType];
      memBlockList_[blockType] = memContainer->next_;
      memContainer->next_ = nullptr;

      if (lowLevelFreeMem(memContainer) == PROCESS_FAIL)
      {
        LOG_WARN("can not decrease memory block in thread local pool, {} type", blockType);
        return PROCESS_FAIL;
      }
      else
      {
        --watermarkIndex_[blockType];
      }
    }

    return PROCESS_SUCCESS;
  }

  bool threadLocalMemoryPool::decWaterMark(int blockType)
  {
    auto waterMarkIndex = watermarkIndex_[blockType];

    if (waterMarkIndex >= threadLocalMemoryPool::lowWaterMark_.array[blockType])
    {
      return PROCESS_SUCCESS;
    }

    if (waterMarkIndex < threadLocalMemoryPool::lowWaterMark_.array[blockType] && waterMarkIndex >= 0)
    {
      //Increase the number of memory blocks in the queue
      for (int i = waterMarkIndex; i < threadLocalMemoryPool::lowWaterMark_.array[blockType]; i++)
      {
        if (auto memContainer = lowLevelAllocMem(BLOCK_SIZE(blockType)); memContainer != nullptr)
        {
          ++watermarkIndex_[blockType];
          memContainer->next_ = memBlockList_[blockType];
          memBlockList_[blockType] = memContainer;
        }
        else
        {
          LOG_WARN("can not supplement thread local memory {} size", BLOCK_SIZE(blockType));
          return PROCESS_FAIL;
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

  threadLocalMemoryPool& getThreadLocalPool()
  {
    static thread_local threadLocalMemoryPool TL_memoryPool;
    return TL_memoryPool;
  }
}
