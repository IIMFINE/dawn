#ifndef _SHM_TRANSPORT_H_
#define _SHM_TRANSPORT_H_
#include "transport.h"
#include <set>
#include <memory>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

namespace dawn
{
  /// @brief shm ring buffer block content: |shm_message_index, message_size, time stamp|...|...|...
  ///        each size of block is 12 byte.
  constexpr const uint8_t SHM_INDEX_BLOCK_SIZE = 4 * 3;
  constexpr const uint8_t SHM_INDEX_RING_BUFFER_HEAD_SIZE = 4 * 3;
  constexpr const uint32_t SHM_BLOCK_NUM = 2560;
  /// @brief BLOCK content: |next, message|...|...
  constexpr const uint32_t SHM_BLOCK_HEAD_SIZE = 4;
  constexpr const uint32_t SHM_BLOCK_CONTENT_SIZE = 1024;
  constexpr const uint32_t SHM_BLOCK_SIZE = SHM_BLOCK_CONTENT_SIZE + SHM_BLOCK_HEAD_SIZE;
  /// @brief total sum of content size is 20MBit
  constexpr const uint32_t SHM_TOTAL_CONTENT_SIZE = SHM_BLOCK_CONTENT_SIZE * SHM_BLOCK_NUM;
  constexpr const uint32_t SHM_TOTAL_SIZE = SHM_BLOCK_NUM * SHM_BLOCK_SIZE;
  constexpr const uint32_t SHM_RING_BUFFER_SIZE = SHM_INDEX_BLOCK_SIZE * SHM_BLOCK_NUM + SHM_INDEX_RING_BUFFER_HEAD_SIZE;
  constexpr const uint32_t SHM_INVALID_INDEX = 0xffffffff;
  constexpr const char*   SHM_MSG_IDENTITY = "dawn_msg";
  constexpr const char*   RING_BUFFER_PREFIX = "ring.";
  constexpr const char*   CHANNEL_PREFIX = "ch.";
  constexpr const char*   MESSAGE_QUEUE_PREFIX = "mq.";
  constexpr const char*   RING_BUFFER_START_PREFIX = "ring.start.";
  constexpr const char*   RING_BUFFER_END_PREFIX = "ring.end.";


#define FIND_SHARE_MEM_BLOCK_ADDR(head, index)  (((char*)head) + (index * SHM_BLOCK_SIZE))
#define FIND_NEXT_MESSAGE_BLOCK_ADDR(head, index)   (((char*)head) + (reinterpret_cast<msgType*>((char*)head) + (index * SHM_BLOCK_SIZE)->next_))

  namespace BI = boost::interprocess;
  struct shmTransport;

  struct shmChannel
  {
    shmChannel() = default;
    shmChannel(std::string_view identity);
    ~shmChannel();
    void initialize(std::string_view identity);
    void notifyAll();
    void waitNotify();
    protected:
    std::string                                 identity_;
    std::shared_ptr<BI::named_condition>        condition_ptr_;
    std::shared_ptr<BI::named_mutex>            mutex_ptr_;
  };

  /// @brief Using share memory to deliver message,
  ///    because of thinking about the scenario for one publisher to multiple subscribers.
  struct shmIndexRingBuffer
  {
    friend struct shmTransport;
    struct ringBufferIndexBlockType
    {
      uint32_t      shmMsgIndex_;
      uint32_t      msgSize_;
      uint32_t      timeStamp_;
    };
    struct __attribute__((packed)) ringBufferType
    {
      uint32_t startIndex_;
      uint32_t endIndex_;
      uint32_t totalIndex_;
      ringBufferIndexBlockType ringBufferBlock_[0];
    };

    shmIndexRingBuffer();
    shmIndexRingBuffer(std::string_view identity);
    ~shmIndexRingBuffer();
    void initialize(std::string_view identity);

    /// @brief Start index move a step and return content pointed by previous start index.
    /// @param prevIndex 
    /// @return PROCESS_SUCCESS success; PROCESS_FAIL failed; ring buffer block
    bool moveStartIndex(ringBufferIndexBlockType &indexBlock);

    /// @brief Move end index meaning have to append new block to ring buffer.
    /// @param indexBlock 
    /// @return PROCESS_SUCCESS: same and start index move, PROCESS_FAIL: different and start index not move.
    bool moveEndIndex(ringBufferIndexBlockType &indexBlock, uint32_t &storePosition);

    /// @brief Watch start index and lock start index mutex until operation finish.
    /// @param indexBlock 
    /// @return 
    bool watchStartIndex(ringBufferIndexBlockType &indexBlock);

    /// @brief Watch end index and lock end index mutex until operation finish.
    /// @param indexBlock 
    /// @return PROCESS_SUCCESS: lock successfully and read buffer. PROCESS_FAIL: lock failed or buffer is empty.
    bool watchEndIndex(ringBufferIndexBlockType &indexBlock);

    /// @brief Unlock end index.
    /// @return 
    bool stopWatchEndIndex();

    /// @brief Pass indexBlock to compare the the block pointing by start index. If they are same, start index will move.
    /// @param indexBlock 
    /// @return PROCESS_SUCCESS: 
    bool compareAndMoveStartIndex(ringBufferIndexBlockType &indexBlock);

    /// @brief Calculate index conformed to ring buffer size.
    /// @param ringBufferIndex 
    /// @return Calculate result.
    uint32_t calculateIndex(uint32_t ringBufferIndex);

    /// @brief 
    /// @param ringBufferIndex 
    /// @return 
    uint32_t calculateLastIndex(uint32_t ringBufferIndex);
    protected:
    std::shared_ptr<BI::named_mutex>                startIndex_mutex_ptr_;
    std::shared_ptr<BI::named_mutex>                endIndex_mutex_ptr_;
    std::shared_ptr<BI::shared_memory_object>       ringBufferShm_ptr_;
    std::shared_ptr<BI::mapped_region>              ringBufferShmRegion_ptr_;
    std::shared_ptr<BI::named_semaphore>            ringBufferInitialFlag_ptr_;
    BI::scoped_lock<BI::named_mutex>                endIndex_lock_;

    ringBufferType                                  *ringBuffer_raw_ptr_;
    std::string                                     shmIdentity_;
    std::string                                     startIndexIdentity_;
    std::string                                     endIndexIdentity_;
  };

  struct __attribute__((packed)) msgType
  {
    uint32_t next_ = SHM_INVALID_INDEX;
    char     content_[0];
  };

  struct shmMsgPool
  {
    shmMsgPool(std::string_view identity = SHM_MSG_IDENTITY);
    ~shmMsgPool();

    std::vector<uint32_t> requireMsgShm(uint32_t data_size);
    uint32_t   requireOneBlock();
    bool recycleMsgShm(uint32_t index);
    void*     getMsgRawBuffer();

    protected:
    uint32_t                                    defaultPriority_;
    std::shared_ptr<BI::message_queue>          availMsg_mq_ptr_;
    std::shared_ptr<BI::named_semaphore>        msgPoolInitialFlag_ptr_;
    std::shared_ptr<BI::shared_memory_object>   msgBufferShm_ptr_;
    std::shared_ptr<BI::mapped_region>          msgBufferShmRegion_ptr_;
    void                                        *msgBuffer_raw_ptr_;
    std::string                                 identity_;
    std::string                                 mqIdentity_;
  };

  struct shmTransport: abstractTransport
  {
    //Compatible with Pimpl-idiom.
    struct shmTransportImpl;
    std::unique_ptr<shmTransportImpl>  impl_;

    shmTransport();
    shmTransport(std::string_view identity);
    ~shmTransport();
    void initialize(std::string_view identity);

    virtual bool write(const void *write_data, const uint32_t data_len) override;

    virtual bool read(void *read_data, uint32_t &data_len) override;

    virtual bool wait() override;
  };

}

#endif
