#include <cassert>
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "shmTransport.h"
#include "common/setLogger.h"
#include "qosController.h"

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
    ///@todo Return block content pointed by start index and lock start index until completed operation.
    return PROCESS_SUCCESS;
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

  struct shmTransport::shmTransportImpl
  {
    shmTransportImpl(std::string_view identity, std::shared_ptr<qosCfg> qosCfg_ptr);
    ~shmTransportImpl() = default;

    /// @brief Write data to data space.
    ///       property: thread safe
    /// @param write_data
    /// @param data_len
    /// @return 
    bool write(const void *write_data, const uint32_t data_len);

    /// @brief Read data from data space.
    ///       property: non thread safe
    /// @param read_data
    /// @param data_len
    /// @param block_type
    /// @return
    bool read(void *read_data, uint32_t &data_len, BLOCKING_TYPE block_type);
    bool wait();

    protected:
    bool publishMsg(uint32_t msgIndex, uint32_t msgSize);

    /// @brief Read the latest index block from ring buffer and will lock ring buffer.
    ///        Need to use unlockRingBuffer() to unlock end index of ring buffer.
    /// @param block 
    /// @return PROCESS_SUCCESS if read index block successfully and will lock end index, otherwise return PROCESS_FAIL.
    bool subscribeMsgAndLock(shmIndexRingBuffer::ringBufferIndexBlockType &block);

    /// @brief Unlock end index of ring buffer.
    /// @return
    void unlockRingBuffer();

    std::vector<uint32_t> retryRequireMsgShm(uint32_t data_size);

    bool recycleMsg(uint32_t msgIndex);

    bool recycleExpireMsg();

    bool read(void *read_data, uint32_t &data_len, uint32_t msgHeadIndex, uint32_t msgSize);

    protected:
    std::string           identity_;
    void                  *msgShm_raw_ptr_;
    volatile bool         lockMsgFlag_;
    std::shared_ptr<shmChannel>            channel_ptr_;
    std::shared_ptr<shmMsgPool>            shmPool_ptr_;
    std::shared_ptr<shmIndexRingBuffer>    ringBuffer_ptr_;
    std::shared_ptr<qosController>         qosController_ptr_;
  };

  shmTransport::shmTransport()
  {
  }

  shmTransport::~shmTransport()
  {
  }

  shmTransport::shmTransport(std::string_view identity, std::shared_ptr<qosCfg> qosCfg_ptr)
  {
    impl_ = std::make_unique<shmTransportImpl>(identity, qosCfg_ptr);
  }

  void shmTransport::initialize(std::string_view identity, std::shared_ptr<qosCfg> qosCfg_ptr)
  {
    impl_ = std::make_unique<shmTransportImpl>(identity, qosCfg_ptr);
  }


  bool shmTransport::write(const void *write_data, const uint32_t data_len)
  {
    return impl_->write(write_data, data_len);
  }

  bool shmTransport::read(void *read_data, uint32_t &data_len, BLOCKING_TYPE block_type)
  {
    return impl_->read(read_data, data_len, block_type);
  }

  bool shmTransport::wait()
  {
    return PROCESS_SUCCESS;
  }

  shmTransport::shmTransportImpl::shmTransportImpl(std::string_view identity, std::shared_ptr<qosCfg> qosCfg_ptr):
    identity_(identity)
  {
    channel_ptr_ = std::make_shared<shmChannel>(CHANNEL_PREFIX + identity_);
    shmPool_ptr_ = std::make_shared<shmMsgPool>();
    ringBuffer_ptr_ = std::make_shared<shmIndexRingBuffer>(RING_BUFFER_PREFIX + identity_);
    msgShm_raw_ptr_ = shmPool_ptr_->getMsgRawBuffer();
    qosController_ptr_ = std::shared_ptr<qosController>(dynamic_cast<qosController*>(new efficientQosController_shm(*(qosCfg_ptr.get()))));
  }

  bool shmTransport::shmTransportImpl::write(const void *write_data, const uint32_t data_len)
  {
    auto msg_vec = retryRequireMsgShm(data_len);

    if (msg_vec.size() == 0)
    {
      LOG_ERROR("can not allocate enough shm");
      return PROCESS_FAIL;
    }

    msgType   *prev_msg = nullptr;
    uint32_t written_len = 0;
    auto wait2writeLen = data_len;

    /// @note Write data to shm
    for (auto msgIndex : msg_vec)
    {
      auto msgBlock = FIND_SHARE_MEM_BLOCK_ADDR(msgShm_raw_ptr_, msgIndex);
      auto msgBlockIns = new(msgBlock) msgType;
      if (wait2writeLen <= SHM_BLOCK_CONTENT_SIZE)
      {
        std::memcpy(msgBlockIns->content_, (char*)write_data + written_len, wait2writeLen);
      }
      else
      {
        std::memcpy(msgBlockIns->content_, (char*)write_data + written_len, SHM_BLOCK_CONTENT_SIZE);
        wait2writeLen -= SHM_BLOCK_CONTENT_SIZE;
        written_len += SHM_BLOCK_CONTENT_SIZE;
      }

      if (prev_msg != nullptr)
      {
        prev_msg->next_ = msgIndex;
        prev_msg = msgBlockIns;
      }
      else
      {
        prev_msg = msgBlockIns;
      }
    }

    /// @note Publish msg to ring buffer
    if (publishMsg(msg_vec[0], data_len) == PROCESS_FAIL)
    {
      return PROCESS_FAIL;
    }

    channel_ptr_->notifyAll();

    return PROCESS_SUCCESS;
  }

  bool shmTransport::shmTransportImpl::read(void *read_data, uint32_t &data_len, BLOCKING_TYPE block_type)
  {
    shmIndexRingBuffer::ringBufferIndexBlockType block;
    std::function<bool(void)> subscribeLatestMsg_func;

    switch (qosController_ptr_->getQosType())
    {
      case qosCfg::QOS_TYPE::EFFICIENT:
        subscribeLatestMsg_func = [&block, this, &read_data, &data_len]() {
          bool result = false;
          if (subscribeMsgAndLock(block) == PROCESS_SUCCESS)
          {
            if (qosController_ptr_->tasteMsgType(&block) == qosController::MSG_FRESHNESS::FRESH)
            {
              qosController_ptr_->updateLatestMsg(&block);
              if (read(read_data, data_len, block.shmMsgIndex_, block.msgSize_) == PROCESS_SUCCESS)
              {
                result = true;
              }
            }
            unlockRingBuffer();
          }
          return result;
        };
        break;
      case qosCfg::QOS_TYPE::RELIABLE:
        subscribeLatestMsg_func = [&block, this, &read_data, &data_len]() {
            
          return false;
        };
        break;
      default:
        break;
    }
    //Try to get the latest index block to taste.
    if (subscribeLatestMsg_func() == true)
    {
      return PROCESS_SUCCESS;
    }

    //Block until message arrival
    if (block_type == BLOCKING_TYPE::BLOCK)
    {
      channel_ptr_->waitNotify(subscribeLatestMsg_func);
      return PROCESS_SUCCESS;
    }
    else
    {
      auto result = channel_ptr_->tryWaitNotify();
      if (result == false)
      {
        return PROCESS_FAIL;
      }
    }

    return PROCESS_FAIL;
  }

  bool shmTransport::shmTransportImpl::publishMsg(uint32_t msgIndex, uint32_t msgSize)
  {
    shmIndexRingBuffer::ringBufferIndexBlockType ringBufferBlock;
    uint32_t      storePosition;
    ringBufferBlock.shmMsgIndex_ = msgIndex;
    ringBufferBlock.msgSize_ = msgSize;
    auto result = ringBuffer_ptr_->moveEndIndex(ringBufferBlock, storePosition);

    if (result == shmIndexRingBuffer::PROCESS_RESULT::FAIL)
    {
      LOG_ERROR("can not write to ring buffer");
      return PROCESS_FAIL;
    }
    else if (result == shmIndexRingBuffer::PROCESS_RESULT::BUFFER_FILL)
    {
      //Best effort to publish the message.
      recycleExpireMsg();
      result = ringBuffer_ptr_->moveEndIndex(ringBufferBlock, storePosition);
      if (result == shmIndexRingBuffer::PROCESS_RESULT::FAIL)
      {
        LOG_ERROR("can not write to ring buffer");
        return PROCESS_FAIL;
      }
      return PROCESS_SUCCESS;
    }
    else
    {
      LOG_DEBUG("shm publish successfully");
      ///@todo record the latest message
      return PROCESS_SUCCESS;
    }
    return PROCESS_SUCCESS;
  }

  bool shmTransport::shmTransportImpl::subscribeMsgAndLock(shmIndexRingBuffer::ringBufferIndexBlockType &block)
  {
    if (ringBuffer_ptr_->watchLatestBuffer(block) == PROCESS_FAIL)
    {
      return PROCESS_FAIL;
    }
    else
    {
      lockMsgFlag_ = true;
      return PROCESS_SUCCESS;
    }
  }

  void shmTransport::shmTransportImpl::unlockRingBuffer()
  {
    if (lockMsgFlag_ == true)
    {
      ringBuffer_ptr_->stopWatchRingBuffer();
      lockMsgFlag_ = false;
    }
  }

  std::vector<uint32_t> shmTransport::shmTransportImpl::retryRequireMsgShm(uint32_t data_size)
  {
    int retryTime = 3;
    for (int i = 0; i < retryTime; i++)
    {
      try
      {
        auto msg_vec = shmPool_ptr_->requireMsgShm(data_size);
        return msg_vec;
      }
      catch (const std::exception& e)
      {
        LOG_ERROR("message shm pool is too low");
        recycleExpireMsg();
      }
    }
    return std::vector<uint32_t>{};
  }

  bool shmTransport::shmTransportImpl::recycleMsg(uint32_t msgIndex)
  {
    assert(msgIndex < SHM_BLOCK_NUM && "msg shm index is out of range");
    for (;msgIndex != SHM_INVALID_INDEX;)
    {
      auto msgBlock = FIND_SHARE_MEM_BLOCK_ADDR(msgShm_raw_ptr_, msgIndex);
      auto msgBlockIns = reinterpret_cast<msgType*>(msgBlock);
      auto nextIndex = msgBlockIns->next_;
      shmPool_ptr_->recycleMsgShm(msgIndex);
      msgIndex = nextIndex;
    }
    return PROCESS_SUCCESS;
  }

  bool shmTransport::shmTransportImpl::recycleExpireMsg()
  {
    shmIndexRingBuffer::ringBufferIndexBlockType block;
    if (ringBuffer_ptr_->moveStartIndex(block) == PROCESS_FAIL)
    {
      return PROCESS_FAIL;
    }

    return recycleMsg(block.shmMsgIndex_);
  }

  bool shmTransport::shmTransportImpl::read(void *read_data, uint32_t &data_len, uint32_t msgHeadIndex, uint32_t msgSize)
  {
    auto msgIndex = msgHeadIndex;
    data_len = 0;
    if (msgIndex == SHM_INVALID_INDEX || msgSize == 0)
    {
      return PROCESS_FAIL;
    }

    for (;msgIndex != SHM_INVALID_INDEX;)
    {
      auto msgBlock = FIND_SHARE_MEM_BLOCK_ADDR(msgShm_raw_ptr_, msgIndex);
      auto msgBlockIns = reinterpret_cast<msgType*>(msgBlock);

      if ((msgSize - data_len) < SHM_BLOCK_CONTENT_SIZE)
      {
        std::memcpy(((char*)read_data + data_len), msgBlockIns->content_, (msgSize - data_len));
        data_len += (msgSize - data_len);
      }
      else
      {
        std::memcpy(((char*)read_data + data_len), msgBlockIns->content_, SHM_BLOCK_CONTENT_SIZE);
        data_len += SHM_BLOCK_CONTENT_SIZE;
      }
      msgIndex = msgBlockIns->next_;
    }

    if (data_len == msgSize)
    {
      return PROCESS_SUCCESS;
    }
    LOG_WARN("message size is truncated {}, actual size {}", data_len, msgSize);
    return PROCESS_FAIL;
  }

  uint32_t  getTimestamp()
  {
    using namespace std::chrono;
    return duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
  }
}
