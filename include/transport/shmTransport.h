#ifndef _SHM_TRANSPORT_H_
#define _SHM_TRANSPORT_H_
#include <set>
#include <memory>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include "transport.h"
#include "common/setLogger.h"

namespace dawn
{
  /// @brief shm ring buffer block content: |shm_message_index, message_size, time stamp|...|...|...
  ///        each size of block is 12 byte.
  constexpr const uint8_t SHM_INDEX_BLOCK_SIZE = 4 * 3;
  constexpr const uint8_t SHM_INDEX_RING_BUFFER_HEAD_SIZE = 4 * 3;
  constexpr const uint32_t SHM_BLOCK_NUM = 1024 * 10;
  /// @brief BLOCK content: |next, message|...|...
  constexpr const uint32_t SHM_BLOCK_HEAD_SIZE = 4;
  constexpr const uint32_t SHM_BLOCK_CONTENT_SIZE = 1024;
  constexpr const uint32_t SHM_BLOCK_SIZE = SHM_BLOCK_CONTENT_SIZE + SHM_BLOCK_HEAD_SIZE;
  /// @brief total sum of content size is 10M byte.
  constexpr const uint32_t SHM_TOTAL_CONTENT_SIZE = SHM_BLOCK_CONTENT_SIZE * SHM_BLOCK_NUM;
  constexpr const uint32_t SHM_TOTAL_SIZE = SHM_BLOCK_NUM * SHM_BLOCK_SIZE;
  constexpr const uint32_t SHM_RING_BUFFER_SIZE = SHM_INDEX_BLOCK_SIZE * SHM_BLOCK_NUM + SHM_INDEX_RING_BUFFER_HEAD_SIZE;
  constexpr const uint32_t SHM_INVALID_INDEX = 0xffffffff;
  constexpr const char*   SHM_MSG_IDENTITY = "dawn_msg";
  constexpr const char*   RING_BUFFER_PREFIX = "ring.";
  constexpr const char*   CHANNEL_PREFIX = "ch.";
  constexpr const char*   MESSAGE_QUEUE_PREFIX = "mq.";
  constexpr const char*   MECHANISM_PREFIX = "msm.";

#define FIND_SHARE_MEM_BLOCK_ADDR(head, index)  (((char*)head) + (index * SHM_BLOCK_SIZE))
#define FIND_NEXT_MESSAGE_BLOCK_ADDR(head, index)   (((char*)head) + (reinterpret_cast<msgType*>((char*)head) + (index * SHM_BLOCK_SIZE)->next_))

  struct shmTransport;
  struct qosCfg;

  namespace BI = boost::interprocess;

  template<typename T>
  struct interprocessMechanism
  {
    interprocessMechanism() = default;
    interprocessMechanism(std::string_view   identity);
    ~interprocessMechanism() = default;
    void initialize(std::string_view identity);

    std::string          identity_;
    T*                    mechanism_raw_ptr_;
    std::shared_ptr<BI::shared_memory_object>   shmObject_ptr_;
    std::shared_ptr<BI::mapped_region>         shmRegion_ptr_;
  };

  template<typename T>
  interprocessMechanism<T>::interprocessMechanism(std::string_view identity):
    identity_(identity)
  {
    using namespace BI;
    try
    {
      shmObject_ptr_ = std::make_shared<shared_memory_object>(create_only, identity_.c_str(), read_write);
      shmObject_ptr_->truncate(sizeof(T));
      shmRegion_ptr_ = std::make_shared<mapped_region>(*(shmObject_ptr_.get()), read_write);
      mechanism_raw_ptr_ = reinterpret_cast<T*>(shmRegion_ptr_->get_address());
      new(mechanism_raw_ptr_)  T;
    }
    catch (const std::exception& e)
    {
      LOG_WARN("interprocessMechanism exception: {}", e.what());
      shmObject_ptr_ = std::make_shared<shared_memory_object>(open_only, identity_.c_str(), read_write);
      shmRegion_ptr_ = std::make_shared<mapped_region>(*(shmObject_ptr_.get()), read_write);
      mechanism_raw_ptr_ = reinterpret_cast<T*>(shmRegion_ptr_->get_address());
    }
  }

  template<typename T>
  void interprocessMechanism<T>::initialize(std::string_view identity)
  {
    using namespace BI;
    identity_ = identity;
    shmObject_ptr_ = std::make_shared<shared_memory_object>(open_or_create, identity_.c_str(), read_write);
    shmObject_ptr_->truncate(sizeof(T));
    shmRegion_ptr_ = std::make_shared<mapped_region>(*(shmObject_ptr_.get()), read_write);
    mechanism_raw_ptr_ = shmRegion_ptr_->get_address();
    new(mechanism_raw_ptr_)  T;
  }

  struct shmChannel
  {
    struct IPC_t
    {
      BI::interprocess_condition           condition_;
      BI::interprocess_mutex             mutex_;
    };

    shmChannel() = default;
    shmChannel(std::string_view identity);
    ~shmChannel() = default;
    void initialize(std::string_view identity);
    void notifyAll();
    void waitNotify();
    bool tryWaitNotify(uint32_t microseconds = 1);
    protected:
    std::string                                 identity_;
    std::shared_ptr<interprocessMechanism<IPC_t>> ipc_ptr_;
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
    //create a ipc mechanism to protect ring buffer
    struct __attribute__((packed)) ringBufferType
    {
      uint32_t startIndex_;
      uint32_t endIndex_;
      uint32_t totalIndex_;
      ringBufferIndexBlockType ringBufferBlock_[0];
    };

    enum class PROCESS_RESULT
    {
      SUCCESS,
      FAIL,
      BUFFER_FILL,
      BUFFER_EMPTY
    };

    struct IPC_t
    {
      IPC_t():
        ringBufferInitializedFlag_(1)
      {
      }
      ~IPC_t() = default;
      BI::interprocess_mutex             startIndex_mutex_;
      BI::interprocess_mutex             endIndex_mutex_;
      BI::interprocess_semaphore         ringBufferInitializedFlag_;
    };

    shmIndexRingBuffer();
    shmIndexRingBuffer(std::string_view identity);
    ~shmIndexRingBuffer() = default;
    void initialize(std::string_view identity);

    /// @brief Start index move a step and return content pointed by previous start index.
    /// @param prevIndex 
    /// @return PROCESS_SUCCESS success; PROCESS_FAIL failed; ring buffer block
    bool moveStartIndex(ringBufferIndexBlockType &indexBlock);

    /// @brief Move end index meaning have to append new block to ring buffer.
    /// @param indexBlock 
    /// @return PROCESS_RESULT
    PROCESS_RESULT moveEndIndex(ringBufferIndexBlockType &indexBlock, uint32_t &storePosition);

    /// @brief Watch start index and lock start index mutex until operation finish.
    /// @param indexBlock 
    /// @return 
    bool watchStartIndex(ringBufferIndexBlockType &indexBlock);

    /// @brief Watch end index and lock end index mutex until watch operation asked to stop.
    /// @param indexBlock 
    /// @return PROCESS_SUCCESS: lock successfully and read buffer. 
    ///         PROCESS_FAIL: buffer is empty or buffer isn't initialize and will not lock.
    bool watchRingBuffer(ringBufferIndexBlockType &indexBlock);

    /// @brief Unlock ring buffer.
    /// @return 
    bool stopWatchRingBuffer();

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
    std::shared_ptr<BI::named_semaphore>            ringBufferInitialFlag_ptr_;
    std::shared_ptr<interprocessMechanism<IPC_t>>   ipc_ptr_;
    std::shared_ptr<BI::shared_memory_object>       ringBufferShm_ptr_;
    std::shared_ptr<BI::mapped_region>              ringBufferShmRegion_ptr_;
    BI::scoped_lock<BI::interprocess_mutex>         endIndex_lock_;
    BI::scoped_lock<BI::interprocess_mutex>         startIndex_lock_;

    ringBufferType                                  *ringBuffer_raw_ptr_;
    std::string                                     shmIdentity_;
    std::string                                     mechanismIdentity_;
  };

  struct __attribute__((packed)) msgType
  {
    uint32_t next_ = SHM_INVALID_INDEX;
    char     content_[0];
  };

  struct shmMsgPool
  {
    struct IPC_t
    {
      IPC_t():
        msgPoolInitialFlag_(1)
      {
      }
      ~IPC_t() = default;
      BI::interprocess_semaphore     msgPoolInitialFlag_;
    };

    shmMsgPool(std::string_view identity = SHM_MSG_IDENTITY);
    ~shmMsgPool() = default;

    std::vector<uint32_t> requireMsgShm(uint32_t data_size);
    uint32_t   requireOneBlock();
    bool recycleMsgShm(uint32_t index);
    void*     getMsgRawBuffer();

    protected:
    uint32_t                                    defaultPriority_;
    std::shared_ptr<interprocessMechanism<IPC_t>>  ipc_ptr_;
    std::shared_ptr<BI::message_queue>          availMsg_mq_ptr_;
    std::shared_ptr<BI::shared_memory_object>   msgBufferShm_ptr_;
    std::shared_ptr<BI::mapped_region>          msgBufferShmRegion_ptr_;
    void                                        *msgBuffer_raw_ptr_;
    std::string                                 identity_;
    std::string                                 mechanismIdentity_;
    std::string                                 mqIdentity_;
  };

  struct shmTransport: abstractTransport
  {
    //Compatible with Pimpl-idiom.
    struct shmTransportImpl;
    std::unique_ptr<shmTransportImpl>  impl_;

    shmTransport();
    shmTransport(std::string_view identity, std::shared_ptr<qosCfg> qosCfg_ptr = std::make_shared<qosCfg>());
    ~shmTransport();
    void initialize(std::string_view identity, std::shared_ptr<qosCfg> qosCfg_ptr = std::make_shared<qosCfg>());

    virtual bool write(const void *write_data, const uint32_t data_len) override;

    virtual bool read(void *read_data, uint32_t &data_len, BLOCK_TYPE block_type) override;

    virtual bool wait() override;
  };

}

#endif
