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
  const int MIN_MEM_BLOCK_TYPE = 7;
  const int MAX_MEM_BLOCK_TYPE = 11;
  const int MIN_MEM_BLOCK = 1 << MIN_MEM_BLOCK_TYPE; // 128byte, satisfy to size of cache line buff.
  const int MAX_MEM_BLOCK = 1 << MAX_MEM_BLOCK_TYPE; // 2048byte, satisfy to udp mtu.

  const int TOTAL_MEM_SIZE = (128 << 20); // total size of each type blocks is 128M byte; 

#define BLOCK_SIZE(BLOCK_TYPE) 1 << BLOCK_TYPE

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

  /// @brief allocate memory api for user
  /// @param requestMemSize 
  /// @return 
  void* allocMem(const int requestMemSize);

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
    memoryNode_t *allocMemBlock(const int requestMemSize);
    bool freeMemBlock(memoryNode_t *releaseMemBlock);

  private:
    lockFreeStack<memoryNode_t *> memoryStoreQueue_[MAX_MEM_BLOCK_TYPE - MIN_MEM_BLOCK_TYPE + 1];
    lockFreeStack<memoryNode_t *> storeEmptyLFQueue_;
    std::list<void *> allCreateMem_;
  };

  class threadLocalMemoryPool final
  {

#define MEM_TYPE_TO_QUEUE_INDEX(type)  (type) - MIN_MEM_BLOCK_TYPE
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
      auto blockType = getMemBlockType(memBlock);
      incWaterMark(blockType);
      return recycleMemBlockToQueue(memBlock);
    }

    bool incWaterMark(int blockType);

    bool decWaterMark(int blockType);

    void* getMemBlockFromQueue(int blockType);

    template<typename T>
    bool recycleMemBlockToQueue(T* memBlock)
    {
      auto blockType = getMemBlockType(memBlock);
      auto queueIndex = MEM_TYPE_TO_QUEUE_INDEX(blockType);
      memBlockQueue_[queueIndex][++watermarkIndex_[queueIndex]] = memBlock;
      return PROCESS_SUCCESS;
    }

  private:
    ///@warning May be is incompatible with other compilers.There is gcc 9.4
    static constexpr geometricSequenceDec<int, MAX_MEM_BLOCK_TYPE - MIN_MEM_BLOCK_TYPE + 1, 200, 2> highWaterMark_{};
    ///@warning May be is incompatible with other compilers.There is gcc 9.4
    static constexpr geometricSequenceDec<int, MAX_MEM_BLOCK_TYPE - MIN_MEM_BLOCK_TYPE + 1, 100, 2> lowWaterMark_{};

    /// @brief the number of each type is at least the number added max high water mark and max low water mark
    void* memBlockQueue_[MAX_MEM_BLOCK_TYPE - MIN_MEM_BLOCK_TYPE + 1]\
      [threadLocalMemoryPool::highWaterMark_.array[0] + threadLocalMemoryPool::lowWaterMark_.array[0]];

    int watermarkIndex_[MAX_MEM_BLOCK_TYPE - MIN_MEM_BLOCK_TYPE + 1];
  };

  void* lowLevelAllocMem(const int requestMemSize);

  template<typename T>
  int getMemBlockType(T* memHead)
  {
    assert(memHead != nullptr && "free a nullptr");
    memoryNode_t* releaseMemNodeHeader = reinterpret_cast<memoryNode_t*>(reinterpret_cast<char*>(memHead) - 1);
    return releaseMemNodeHeader->memoryBlockType_;
  }

  template<typename T>
  bool lowLevelFreeMem(T* memHead)
  {
    assert(memHead != nullptr && "free a nullptr");
    memoryNode_t* releaseMemNodeHeader = reinterpret_cast<memoryNode_t*>(reinterpret_cast<char*>(memHead) - 1);
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlock(releaseMemNodeHeader);
  }

  /// @brief free memory to final memory pool
  /// @param releaseMemBlock 
  /// @return 
  bool lowLevelFreeMem(memoryNode_t *releaseMemBlock);

  /// @brief free memory to final memory pool
  /// @param memHead 
  /// @return 
  bool lowLevelFreeMem(void *memHead);

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
    auto& memoryPool = getThreadLocalPool();
    return memoryPool.TL_freeMem(memHead);
  }
}

#endif
