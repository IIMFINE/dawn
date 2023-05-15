#include <cassert>
#include <chrono>

#include "shmTransport.h"
#include "common/setLogger.h"
#include "shmTransportController.h"
#include "shmTransportImpl.hh"

namespace  dawn
{
  shmChannel::shmChannel(std::string_view identity):
    identity_(identity)
  {
    using namespace BI;
    if (identity.empty())
    {
      throw std::runtime_error("dawn: shm channel identity is empty");
    }
    ipc_ptr_ = std::make_shared<interprocessMechanism<IPC_t>>(identity_);
  }

  void shmChannel::initialize(std::string_view identity)
  {
    using namespace BI;
    identity_ = identity;
    if (identity.empty())
    {
      throw std::runtime_error("dawn: shm channel identity is empty");
    }
    ipc_ptr_ = std::make_shared<interprocessMechanism<IPC_t>>(identity_);
  }

  void shmChannel::notifyAll()
  {
    ipc_ptr_->mechanism_raw_ptr_->condition_.notify_all();
  }

  void shmChannel::waitNotify()
  {
    using namespace BI;
    scoped_lock<interprocess_mutex> lock(ipc_ptr_->mechanism_raw_ptr_->mutex_);
    ipc_ptr_->mechanism_raw_ptr_->condition_.wait(lock);
  }

  bool shmChannel::tryWaitNotify(uint32_t microsecond)
  {
    using namespace BI;
    scoped_lock<interprocess_mutex> lock(ipc_ptr_->mechanism_raw_ptr_->mutex_);
    return ipc_ptr_->mechanism_raw_ptr_->condition_.timed_wait(lock, \
      boost::posix_time::microsec_clock::universal_time() + boost::posix_time::microseconds(microsecond));
  }

  shmIndexRingBuffer::shmIndexRingBuffer(std::string_view identity):
    shmIdentity_(identity),
    mechanismIdentity_(MECHANISM_PREFIX + shmIdentity_)
  {
    using namespace BI;
    ipc_ptr_ = std::make_shared<interprocessMechanism<IPC_t>>(mechanismIdentity_);
    ringBufferShm_ptr_ = std::make_shared<shared_memory_object>(open_or_create, shmIdentity_.c_str(), read_write);
    ringBufferShm_ptr_->truncate(SHM_RING_BUFFER_SIZE);
    ringBufferShmRegion_ptr_ = std::make_shared<mapped_region>(*(ringBufferShm_ptr_.get()), read_write);
    ringBuffer_raw_ptr_ = reinterpret_cast<ringBufferType*>(ringBufferShmRegion_ptr_->get_address());

    ///@note ensure ringBuffer_raw_ptr_ just initialize only one time.
    if (ipc_ptr_->mechanism_raw_ptr_->ringBufferInitializedFlag_.try_wait())
    {
      ringBuffer_raw_ptr_->startIndex_ = SHM_INVALID_INDEX;
      ringBuffer_raw_ptr_->endIndex_ = SHM_INVALID_INDEX;
      ringBuffer_raw_ptr_->totalIndex_ = SHM_BLOCK_NUM;
    }
  }

  void shmIndexRingBuffer::initialize(std::string_view identity)
  {
    using namespace BI;
    shmIdentity_ = identity;
    mechanismIdentity_ = MECHANISM_PREFIX + shmIdentity_;
    ipc_ptr_ = std::make_shared<interprocessMechanism<IPC_t>>(mechanismIdentity_);
    ringBufferShm_ptr_ = std::make_shared<shared_memory_object>(open_or_create, shmIdentity_.c_str(), read_write);
    ringBufferShm_ptr_->truncate(SHM_RING_BUFFER_SIZE);
    ringBufferShmRegion_ptr_ = std::make_shared<mapped_region>(*(ringBufferShm_ptr_.get()), read_write);
    ringBuffer_raw_ptr_ = reinterpret_cast<ringBufferType*>(ringBufferShmRegion_ptr_->get_address());

    ///@note ensure ringBuffer_raw_ptr_ just initialize only one time.
    if (ipc_ptr_->mechanism_raw_ptr_->ringBufferInitializedFlag_.try_wait())
    {
      ringBuffer_raw_ptr_->startIndex_ = SHM_INVALID_INDEX;
      ringBuffer_raw_ptr_->endIndex_ = SHM_INVALID_INDEX;
      ringBuffer_raw_ptr_->totalIndex_ = SHM_BLOCK_NUM;
    }
  }

  bool shmIndexRingBuffer::moveStartIndex(ringBufferIndexBlockType &indexBlock)
  {
    BI::scoped_lock<BI::interprocess_mutex>    startIndexLock(ipc_ptr_->mechanism_raw_ptr_->startIndex_mutex_);

    if (ringBuffer_raw_ptr_->startIndex_ == SHM_INVALID_INDEX)
    {
      LOG_ERROR("Ring buffer is not append any content");
      return PROCESS_FAIL;
    }
    else if (ringBuffer_raw_ptr_->startIndex_ == ringBuffer_raw_ptr_->endIndex_)
    {
      /// @note Prevent to GCC 9.4.0 throw "cannot bind packed field" error.
      LOG_WARN("Ring buffer start index {} overlap end index {}, ring buffer have not content", \
        (ringBuffer_raw_ptr_->startIndex_ + 0), (ringBuffer_raw_ptr_->endIndex_ + 0));
      return PROCESS_FAIL;
    }
    else if (calculateIndex((ringBuffer_raw_ptr_->startIndex_ + 1)) == ringBuffer_raw_ptr_->endIndex_)
    {
      LOG_WARN("Ring buffer start index {} will overlap end index {}", ringBuffer_raw_ptr_->startIndex_ + 1, (ringBuffer_raw_ptr_->endIndex_ + 0));
      std::memcpy(&indexBlock, &ringBuffer_raw_ptr_->ringBufferBlock_[ringBuffer_raw_ptr_->startIndex_], sizeof(ringBufferIndexBlockType));
      ringBuffer_raw_ptr_->startIndex_ = calculateIndex(ringBuffer_raw_ptr_->startIndex_ + 1);
      return PROCESS_SUCCESS;
    }
    else if (calculateIndex((ringBuffer_raw_ptr_->startIndex_ + 1)) != ringBuffer_raw_ptr_->endIndex_)
    {
      std::memcpy(&indexBlock, &ringBuffer_raw_ptr_->ringBufferBlock_[ringBuffer_raw_ptr_->startIndex_], sizeof(ringBufferIndexBlockType));
      ringBuffer_raw_ptr_->startIndex_ = calculateIndex(ringBuffer_raw_ptr_->startIndex_ + 1);
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }

  shmIndexRingBuffer::PROCESS_RESULT shmIndexRingBuffer::moveEndIndex(ringBufferIndexBlockType &indexBlock, uint32_t &storePosition)
  {
    BI::scoped_lock<BI::interprocess_mutex>    endIndexLock(ipc_ptr_->mechanism_raw_ptr_->endIndex_mutex_);

    /// @note Accurate time stamp must be get after locked.
    indexBlock.timeStamp_ = getTimestamp();

    if (ringBuffer_raw_ptr_->endIndex_ == SHM_INVALID_INDEX)
    {
      // This scenario is ring buffer is initial.
      ringBuffer_raw_ptr_->endIndex_ = 0;
      ringBuffer_raw_ptr_->startIndex_ = 0;
      std::memcpy(&ringBuffer_raw_ptr_->ringBufferBlock_[ringBuffer_raw_ptr_->endIndex_], &indexBlock, sizeof(ringBufferIndexBlockType));
      ringBuffer_raw_ptr_->endIndex_ = calculateIndex(ringBuffer_raw_ptr_->endIndex_ + 1);
      return PROCESS_RESULT::SUCCESS;
    }
    else if (ringBuffer_raw_ptr_->startIndex_ == ringBuffer_raw_ptr_->endIndex_)
    {
      // This scenario is ring buffer is empty.
      std::memcpy(&ringBuffer_raw_ptr_->ringBufferBlock_[ringBuffer_raw_ptr_->endIndex_], &indexBlock, sizeof(ringBufferIndexBlockType));
      storePosition = ringBuffer_raw_ptr_->endIndex_;
      ringBuffer_raw_ptr_->endIndex_ = calculateIndex(ringBuffer_raw_ptr_->endIndex_ + 1);
      return PROCESS_RESULT::SUCCESS;
    }
    else if (calculateIndex(ringBuffer_raw_ptr_->endIndex_ + 1) == ringBuffer_raw_ptr_->startIndex_)
    {
      LOG_WARN("Ring buffer's full filled");
      return PROCESS_RESULT::BUFFER_FILL;
    }
    else if (calculateIndex(ringBuffer_raw_ptr_->endIndex_ + 1) != ringBuffer_raw_ptr_->startIndex_)
    {
      std::memcpy(&ringBuffer_raw_ptr_->ringBufferBlock_[ringBuffer_raw_ptr_->endIndex_], &indexBlock, sizeof(ringBufferIndexBlockType));
      storePosition = ringBuffer_raw_ptr_->endIndex_;
      ringBuffer_raw_ptr_->endIndex_ = calculateIndex(ringBuffer_raw_ptr_->endIndex_ + 1);
      return PROCESS_RESULT::SUCCESS;
    }
    return PROCESS_RESULT::FAIL;
  }

  bool shmIndexRingBuffer::watchStartIndex(ringBufferIndexBlockType &indexBlock)
  {
    BI::scoped_lock<BI::interprocess_mutex>       startIndexLock(ipc_ptr_->mechanism_raw_ptr_->startIndex_mutex_);
    if (ringBuffer_raw_ptr_->endIndex_ == SHM_INVALID_INDEX || ringBuffer_raw_ptr_->startIndex_ == SHM_INVALID_INDEX)
    {
      return PROCESS_FAIL;
    }
    else if (ringBuffer_raw_ptr_->endIndex_ == ringBuffer_raw_ptr_->startIndex_)
    {
      LOG_WARN("ring buffer is empty");
      return PROCESS_FAIL;
    }
    else
    {
      std::memcpy(&indexBlock, &ringBuffer_raw_ptr_->ringBufferBlock_[ringBuffer_raw_ptr_->startIndex_], sizeof(ringBufferIndexBlockType));
      startIndex_lock_ = std::move(startIndexLock);
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }

  bool shmIndexRingBuffer::watchLatestBuffer(ringBufferIndexBlockType &indexBlock)
  {
    BI::scoped_lock<BI::interprocess_mutex>       endLock(ipc_ptr_->mechanism_raw_ptr_->endIndex_mutex_);
    ///@note Lock start index prevent other process move start index overlap end index.
    ///     It will cause ring buffer is empty, but still to read content.
    BI::scoped_lock<BI::interprocess_mutex>       startLock(ipc_ptr_->mechanism_raw_ptr_->startIndex_mutex_);

    if (ringBuffer_raw_ptr_->endIndex_ == SHM_INVALID_INDEX)
    {
      return PROCESS_FAIL;
    }
    else if (ringBuffer_raw_ptr_->endIndex_ == ringBuffer_raw_ptr_->startIndex_)
    {
      LOG_WARN("ring buffer is empty");
      return PROCESS_FAIL;
    }
    else
    {
      std::memcpy(&indexBlock, &ringBuffer_raw_ptr_->ringBufferBlock_[calculateLastIndex(ringBuffer_raw_ptr_->endIndex_)], sizeof(ringBufferIndexBlockType));
      endIndex_lock_ = std::move(endLock);
      startIndex_lock_ = std::move(startLock);
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }

  bool shmIndexRingBuffer::stopWatchRingBuffer()
  {
    startIndex_lock_.unlock();
    endIndex_lock_.unlock();
    return PROCESS_SUCCESS;
  }

  bool  shmIndexRingBuffer::getStartIndex(uint32_t &storeIndex, ringBufferIndexBlockType &indexBlock)
  {
    BI::scoped_lock<BI::interprocess_mutex>       startIndexLock(ipc_ptr_->mechanism_raw_ptr_->startIndex_mutex_);
    if (ringBuffer_raw_ptr_->endIndex_ == SHM_INVALID_INDEX || ringBuffer_raw_ptr_->startIndex_ == SHM_INVALID_INDEX)
    {
      return PROCESS_FAIL;
    }
    else if (ringBuffer_raw_ptr_->endIndex_ == ringBuffer_raw_ptr_->startIndex_)
    {
      LOG_WARN("ring buffer is empty");
      return PROCESS_FAIL;
    }
    else
    {
      storeIndex = ringBuffer_raw_ptr_->startIndex_;
      std::memcpy(&indexBlock, &ringBuffer_raw_ptr_->ringBufferBlock_[ringBuffer_raw_ptr_->startIndex_], sizeof(ringBufferIndexBlockType));
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }

  bool shmIndexRingBuffer::watchSpecificIndexBuffer(uint32_t index, ringBufferIndexBlockType &indexBlock)
  {
    BI::scoped_lock<BI::interprocess_mutex>       endLock(ipc_ptr_->mechanism_raw_ptr_->endIndex_mutex_);
    ///@note Lock start index prevent other process move start index overlap end index.
    ///     It will cause ring buffer is empty, but still to read content.
    BI::scoped_lock<BI::interprocess_mutex>       startLock(ipc_ptr_->mechanism_raw_ptr_->startIndex_mutex_);

    if (ringBuffer_raw_ptr_->endIndex_ == SHM_INVALID_INDEX)
    {
      return PROCESS_FAIL;
    }
    else if (ringBuffer_raw_ptr_->endIndex_ == ringBuffer_raw_ptr_->startIndex_)
    {
      LOG_WARN("ring buffer is empty");
      return PROCESS_FAIL;
    }
    else
    {
      std::memcpy(&indexBlock, &ringBuffer_raw_ptr_->ringBufferBlock_[calculateLastIndex(index)], sizeof(ringBufferIndexBlockType));
      endIndex_lock_ = std::move(endLock);
      startIndex_lock_ = std::move(startLock);
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }


  bool shmIndexRingBuffer::compareAndMoveStartIndex(ringBufferIndexBlockType &indexBlock)
  {
    ///@todo Compare index block content and decide to whether move start index.
    return PROCESS_SUCCESS;
  }

  uint32_t shmIndexRingBuffer::calculateIndex(uint32_t ringBufferIndex)
  {
    return ringBufferIndex % (ringBuffer_raw_ptr_->totalIndex_);
  }

  uint32_t shmIndexRingBuffer::calculateLastIndex(uint32_t ringBufferIndex)
  {
    return ringBufferIndex == 0 ? (ringBuffer_raw_ptr_->totalIndex_ - 1) : (ringBufferIndex - 1);
  }

  shmMsgPool::shmMsgPool(std::string_view identity):
    defaultPriority_(0),
    identity_(identity),
    mechanismIdentity_(MECHANISM_PREFIX + identity_),
    mqIdentity_(MESSAGE_QUEUE_PREFIX + identity_)
  {
    using namespace BI;
    ipc_ptr_ = std::make_shared<interprocessMechanism<IPC_t>>(mechanismIdentity_);
    availMsg_mq_ptr_ = std::make_shared<message_queue>(open_or_create, mqIdentity_.c_str(), SHM_BLOCK_NUM, sizeof(uint32_t));
    msgBufferShm_ptr_ = std::make_shared<shared_memory_object>(open_or_create, identity_.c_str(), read_write);
    msgBufferShm_ptr_->truncate(SHM_TOTAL_SIZE);
    msgBufferShmRegion_ptr_ = std::make_shared<mapped_region>(*(msgBufferShm_ptr_.get()), read_write);
    msgBuffer_raw_ptr_ = reinterpret_cast<void*>(msgBufferShmRegion_ptr_->get_address());

    if (ipc_ptr_->mechanism_raw_ptr_->msgPoolInitialFlag_.try_wait())
    {
      for (uint32_t i = 0; i < SHM_BLOCK_NUM; i++)
      {
        availMsg_mq_ptr_->send(&i, sizeof(i), defaultPriority_);
      }
    }
  }

  std::vector<uint32_t> shmMsgPool::requireMsgShm(uint32_t data_size)
  {
    assert(data_size != 0 && " require zero shm size");
    uint32_t needBlockNum = (data_size + SHM_BLOCK_CONTENT_SIZE) / SHM_BLOCK_CONTENT_SIZE;
    if (needBlockNum > SHM_BLOCK_NUM)
    {
      throw std::runtime_error("dawn: allocate too big share memory block");
    }
    std::vector<uint32_t>   indexVec;
    indexVec.reserve(needBlockNum);

    for (uint32_t i = 0; i < needBlockNum; i++)
    {
      auto index = requireOneBlock();
      if (index == SHM_INVALID_INDEX)
      {
        for (auto it : indexVec)
        {
          recycleMsgShm(it);
        }
        //@todo add dawn exception
        throw std::runtime_error("dawn: too low shm resource");
      }
      indexVec.emplace_back(index);
    }

    return indexVec;
  }

  uint32_t shmMsgPool::requireOneBlock()
  {
    using namespace BI;
    uint32_t index = 0;
    message_queue::size_type recvd_size;
    if (availMsg_mq_ptr_->try_receive(&index, sizeof(index), recvd_size, defaultPriority_) == false)
    {
      return SHM_INVALID_INDEX;
    }
    auto mem = FIND_SHARE_MEM_BLOCK_ADDR(msgBuffer_raw_ptr_, index);

    //initialize share memory block
    new(mem) msgType;

    if (recvd_size != sizeof(index))
    {
      return SHM_INVALID_INDEX;
    }
    return index;
  }

  void* shmMsgPool::getMsgRawBuffer()
  {
    return msgBuffer_raw_ptr_;
  }

  bool shmMsgPool::recycleMsgShm(uint32_t index)
  {
    assert(index < (SHM_BLOCK_NUM) && "msg shm index is out of range");

    auto mem = FIND_SHARE_MEM_BLOCK_ADDR(msgBuffer_raw_ptr_, index);

    //Initialize share memory block and clear shm block's content
    auto msgIns = (new(mem) msgType);
    std::memset(&msgIns->content_, 0x0, SHM_BLOCK_CONTENT_SIZE);

    availMsg_mq_ptr_->send(&index, sizeof(index), defaultPriority_);
    return true;
  }

  shmTransport::shmTransport()
  {
  }

  shmTransport::~shmTransport()
  {
  }

  shmTransport::shmTransport(std::string_view identity, std::shared_ptr<qosCfg> qosCfg_ptr)
  {
    auto impl = std::make_unique<shmTransportImpl>(identity);
    if (qosCfg_ptr->qosType_ == qosCfg::QOS_TYPE::EFFICIENT)
    {
      tpController_ptr_ = std::make_unique<efficientTpController_shm>(std::move(impl), *qosCfg_ptr);
    }
  }

  void shmTransport::initialize(std::string_view identity, std::shared_ptr<qosCfg> qosCfg_ptr)
  {
    auto impl = std::make_unique<shmTransportImpl>(identity);
    if (qosCfg_ptr->qosType_ == qosCfg::QOS_TYPE::EFFICIENT)
    {
      tpController_ptr_ = std::make_unique<efficientTpController_shm>(std::move(impl), *qosCfg_ptr);
    }
  }

  bool shmTransport::write(const void *write_data, const uint32_t data_len)
  {
    return tpController_ptr_->write(write_data, data_len);
  }

  bool shmTransport::read(void *read_data, uint32_t &data_len, BLOCKING_TYPE block_type)
  {
    return tpController_ptr_->read(read_data, data_len, block_type);
  }

  bool shmTransport::wait()
  {
    return PROCESS_SUCCESS;
  }

  uint32_t  getTimestamp()
  {
    using namespace std::chrono;
    return duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
  }
}
