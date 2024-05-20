#ifndef _MEMORY_POOL_H_
#define _MEMORY_POOL_H_
#include <atomic>
#include <cassert>
#include <list>

#include "lockFree.h"
#include "setLogger.h"
#include "type.h"

namespace dawn
{
// BASE block size is 128byte which is satisfied with multiple of cache line
// buffer.
constexpr const int kBaseBlockBitPosition = 7;
constexpr const int kBaseBlockSize = 1 << kBaseBlockBitPosition;

// Max block size is 2048byte which is satisfied with udp mtu.
constexpr const int kMaxMultiple = 16;
constexpr const int kMaxMemBlockSize = kBaseBlockSize * kMaxMultiple;
#define BLOCK_SIZE(BLOCK_TYPE) ((BLOCK_TYPE)*kBaseBlockSize + kBaseBlockSize)

constexpr const int kSequenceBaseNum = 512;
constexpr const int kSequenceRatio = 1024;
constexpr const int kMemPoolSupportThreadNum = 64;
constexpr const int kEachThreadSequenceRatio = kSequenceRatio / kMemPoolSupportThreadNum;
static constexpr const arithmeticSequenceDec<int, kMaxMultiple, 512, kSequenceRatio> kEachBlockTotalNum{};

#define GET_MEM_QUEUE_MASK(size) ((size + kBaseBlockSize - 1) / kBaseBlockSize - 1)

#define GET_DATA(node) node->memoryHead_;

#pragma pack(1)
struct memoryNode_t
{
    char memoryBlockType_;
    char memoryHead_[0];
};
#pragma pack()

template <typename T>
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

    bool freeMemBlock(memoryNode_t* releaseMemBlock);

    LF_Node<memoryNode_t*>* allocMemBlockWithContainer(const int requestMemSize);

    bool freeMemBlockWithContainer(LF_Node<memoryNode_t*>* releaseMemBlock);

    bool freeEmptyContainer(LF_Node<memoryNode_t*>* releaseContainer);

private:
    LockFreeStack<memoryNode_t*> memoryStoreQueue_[kMaxMultiple];
    LockFreeStack<memoryNode_t*> storeEmptyLFQueue_;
    std::list<void*> allCreateMem_;
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

    template <typename T>
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
    ///@warning May be this initialization is incompatible with other
    /// compilers.There is gcc 9.4
    static constexpr arithmeticSequenceDec<int, kMaxMultiple, 30, kEachThreadSequenceRatio> highWaterMark_{};
    ///@warning May be this initialization is incompatible with other
    /// compilers.There is gcc 9.4
    static constexpr arithmeticSequenceDec<int, kMaxMultiple, 10, 10> lowWaterMark_{};

    /// @brief the number of each type is at least the number added max high
    /// water mark and max low water mark
    LF_Node<memoryNode_t*>* memBlockList_[kMaxMultiple];

    LF_Node<memoryNode_t*>* remainMemBlockContainerList_;

    int watermarkIndex_[kMaxMultiple];
};

/// @brief Allocate memory with container from final memory pool
/// @param requestMemSize
/// @return memory block with container
inline LF_Node<memoryNode_t*>* lowLevelAllocMem(const int requestMemSize)
{
    return singleton<dawn::memoryPool>::getInstance()->allocMemBlockWithContainer(requestMemSize);
}

template <typename T>
int getMemBlockType(T* memHead)
{
    assert(memHead != nullptr && "free a nullptr");
    memoryNode_t* releaseMemNodeHeader = reinterpret_cast<memoryNode_t*>(reinterpret_cast<char*>(memHead) - 1);
    return releaseMemNodeHeader->memoryBlockType_;
}

inline bool lowLevelFreeEmptyContainer(LF_Node<memoryNode_t*>* releaseContainer)
{
    assert(releaseContainer != nullptr && "free a nullptr container");
    return singleton<dawn::memoryPool>::getInstance()->freeEmptyContainer(releaseContainer);
}

/// @brief free memory to final memory pool
/// @tparam T
/// @param memHead default is memory block without container.
/// @return
template <typename T>
inline bool lowLevelFreeMem(T* memHead)
{
    assert(memHead != nullptr && "free a nullptr");
    memoryNode_t* releaseMemNodeHeader = reinterpret_cast<memoryNode_t*>(reinterpret_cast<char*>(memHead) - 1);
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlock(releaseMemNodeHeader);
}

inline bool lowLevelFreeMem(LF_Node<memoryNode_t*>* memContainer)
{
    assert((memContainer != nullptr || memContainer->element_val_ != nullptr) && "free a nullptr container");
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlockWithContainer(memContainer);
}

/// @brief free memory to final memory pool
/// @param releaseMemBlock
/// @return
inline bool lowLevelFreeMem(memoryNode_t* releaseMemBlock)
{
    assert(releaseMemBlock != nullptr && "free a nullptr");
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlock(releaseMemBlock);
}

/// @brief free memory to final memory pool
/// @param memHead default memHead is a memory block without container
/// @return
inline bool lowLevelFreeMem(void* memHead)
{
    assert(memHead != nullptr && "free a nullptr");
    memoryNode_t* releaseMemNodeHeader = reinterpret_cast<memoryNode_t*>(reinterpret_cast<char*>(memHead) - 1);
    return singleton<dawn::memoryPool>::getInstance()->freeMemBlock(releaseMemNodeHeader);
}

/// @brief Used to initialize final memory pool.Remember use it before request
/// to allocate memory.
/// @note It can be called one more time, but the memory pool will be
/// initialized only one time.
void memPoolInit();

/// @brief get memory pool belong to per thread
/// @return
threadLocalMemoryPool& getThreadLocalPool();

/// @brief release memory api for user
/// @tparam T
/// @param memHead
/// @return
template <typename T>
bool freeMem(T* memHead)
{
    assert(memHead != nullptr && "free a nullptr");
    return getThreadLocalPool().TL_freeMem(memHead);
}

/// @brief allocate memory api for user
/// @param requestMemSize
/// @return
inline void* allocMem(const int requestMemSize) { return getThreadLocalPool().TL_allocMem(requestMemSize); }
}  // namespace dawn

#endif
