#ifndef _MEMORY_POOL_H_
#define _MEMORY_POOL_H_
#include <atomic>
#include <list>
#include <cassert>
#include "lockFree.h"
#include "type.h"
#include "setLogger.h"
namespace dawn
{
  //BASE block size is 128byte which is satisfied with multiple of cache line buffer.
  constexpr const int BASE_BLOCK_BIT_POSITION = 7;
  constexpr const int BASE_BLOCK_SIZE = 1 << BASE_BLOCK_BIT_POSITION;

  //Max block size is 2048byte which is satisfied with udp mtu.
  constexpr const int MAX_MULTIPLE = 16;
  constexpr const int MAX_MEM_BLOCK_SIZE = BASE_BLOCK_SIZE * MAX_MULTIPLE;
#define BLOCK_SIZE(BLOCK_TYPE) ((BLOCK_TYPE) * BASE_BLOCK_SIZE + BASE_BLOCK_SIZE)

  constexpr const int SEQUENCE_BASE_NUM = 512;
  constexpr const int SEQUENCE_RATIO = 1024;
  constexpr const int MEM_POOL_SUPPORT_THREAD_NUM = 64;
  constexpr const int EACH_THREAD_SEQUENCE_RATIO = SEQUENCE_RATIO / MEM_POOL_SUPPORT_THREAD_NUM;
  static  constexpr const arithmeticSequenceDec<int, MAX_MULTIPLE, 512, SEQUENCE_RATIO> EACH_BLOCK_TOTAL_NUM{};

#define GET_MEM_QUEUE_MASK(size)  ((size + BASE_BLOCK_SIZE - 1) / BASE_BLOCK_SIZE - 1)

#define GET_DATA(node) node->memoryHead_;

#pragma pack(1)
  struct memoryNode_t
  {
    char memoryBlockType_;
    char memoryHead_[0];
  };
#pragma pack()

  template<typename T>
  int getMemBlockType(T* memHead);

  class memoryPool final
  {
    /**
     * @brief usage: init() -> lowLevelAllocMem() -> lowLevelFreeMem()
     *
     */
    public:
    memoryPool();
    ~memoryPool();
    void init();
    memoryNode_t* allocMemBlock(const int requestMemSize);

    bool freeMemBlock(memoryNode_t *releaseMemBlock);

    LF_node_t<memoryNode_t *>* allocMemBlockWithContainer(const int requestMemSize);

    bool freeMemBlockWithContainer(LF_node_t<memoryNode_t *> *releaseMemBlock);

    bool freeEmptyContainer(LF_node_t<memoryNode_t *> *releaseContainer);

    private:
    lockFreeStack<memoryNode_t *> memoryStoreQueue_[MAX_MULTIPLE];
    lockFreeStack<memoryNode_t *> storeEmptyLFQueue_;
    std::list<void *> allCreateMem_;
  };

  class threadLocalMemoryPool final
  {
    public:
    threadLocalMemoryPool();
    ///@todo implement this
    ~threadLocalMemoryPool();
    bool initialize();
    /// @brief TL is the abbreviation for thread local
    /// @param requestMemSize 
    /// @return 
    void* TL_allocMem(const int requestMemSize);

    template<typename T>
    bool TL_freeMem(T* memBlock)
    {
      auto memHead = reinterpret_cast<memoryNode_t*>(reinterpret_cast<char*>(memBlock) - 1);
      incWaterMark(memHead->memoryBlockType_);
      return recycleMemBlockToQueue(memHead, memHead->memoryBlockType_);
    }

    bool incWaterMark(int blockType);

    bool decWaterMark(int blockType);

    void* getMemBlockFromQueue(int blockType);

    bool recycleMemBlockToQueue(memoryNode_t* memBlock, int blockType);

    private:
    ///@warning May be this initialization is incompatible with other compilers.There is gcc 9.4
    static constexpr arithmeticSequenceDec<int, MAX_MULTIPLE, 30, EACH_THREAD_SEQUENCE_RATIO> highWaterMark_{};
    ///@warning May be this initialization is incompatible with other compilers.There is gcc 9.4
    static constexpr arithmeticSequenceDec<int, MAX_MULTIPLE, 10, 10> lowWaterMark_{};

    /// @brief the number of each type is at least the number added max high water mark and max low water mark
    LF_node_t<memoryNode_t *>* memBlockList_[MAX_MULTIPLE];

    LF_node_t<memoryNode_t *>* remainMemBlockContainerList_;

    int watermarkIndex_[MAX_MULTIPLE];
  };

  /// @brief Allocate memory with container from final memory pool
  /// @param requestMemSize 
  /// @return memory block with container
  inline LF_node_t<memoryNode_t *>* lowLevelAllocMem(const int requestMemSize)
  {
    return singleton<dawn::memoryPool>::getInstance()->allocMemBlockWithContainer(requestMemSize);
  }

  template<typename T>
  int getMemBlockType(T* memHead)
  {
    assert(memHead != nullptr && "free a nullptr");
    memoryNode_t* releaseMemNodeHeader = reinterpret_cast<memoryNode_t*>(reinterpret_cast<char*>(memHead) - 1);
    return releaseMemNodeHeader->memoryBlockType_;
  }

  inline bool lowLevelFreeEmptyContainer(LF_node_t<memoryNode_t *> *releaseContainer)
  {
    assert(releaseContainer != nullptr && "free a nullptr container");
    return singleton<dawn::memoryPool>::getInstance()->freeEmptyContainer(releaseContainer);
  }

  /// @brief free memory to final memory pool
  /// @tparam T 
  /// @param memHead default is memory block without container.
  /// @return 
  template<typename T>
  inline bool lowLevelFreeMem(T* memHead)
  {
    assert(memHead != nullptr && "free a nullptr");
    memoryNode_t* releaseMemNodeHeader = reinterpret_cast<memoryNode_t*>(reinterpret_cast<char*>(memHead) - 1);
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlock(releaseMemNodeHeader);
  }

  inline bool lowLevelFreeMem(LF_node_t<memoryNode_t *> *memContainer)
  {
    assert((memContainer != nullptr || memContainer->elementVal_ != nullptr) && "free a nullptr container");
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlockWithContainer(memContainer);
  }

  /// @brief free memory to final memory pool
  /// @param releaseMemBlock 
  /// @return 
  inline bool lowLevelFreeMem(memoryNode_t *releaseMemBlock)
  {
    assert(releaseMemBlock != nullptr && "free a nullptr");
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlock(releaseMemBlock);
  }

  /// @brief free memory to final memory pool
  /// @param memHead default memHead is a memory block without container
  /// @return 
  inline bool lowLevelFreeMem(void *memHead)
  {
    assert(memHead != nullptr && "free a nullptr");
    memoryNode_t* releaseMemNodeHeader = reinterpret_cast<memoryNode_t*>(reinterpret_cast<char*>(memHead) - 1);
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlock(releaseMemNodeHeader);
  }

  /// @brief Used to initialize final memory pool.Remember use it before request to allocate memory.
  /// @note It can be called one more time, but the memory pool will be initialized only one time.
  void memPoolInit();

  /// @brief get memory pool belong to per thread
  /// @return 
  threadLocalMemoryPool& getThreadLocalPool();

  /// @brief release memory api for user
  /// @tparam T 
  /// @param memHead 
  /// @return 
  template<typename T>
  bool freeMem(T* memHead)
  {
    assert(memHead != nullptr && "free a nullptr");
    return getThreadLocalPool().TL_freeMem(memHead);
  }

  /// @brief allocate memory api for user
  /// @param requestMemSize 
  /// @return 
  inline void* allocMem(const int requestMemSize)
  {
    return getThreadLocalPool().TL_allocMem(requestMemSize);
  }
}

#endif
