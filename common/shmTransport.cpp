#include <cassert>
#include <ctime>
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "shmTransport.h"
#include "setLogger.h"

namespace  dawn
{
  shmChannel::shmChannel(std::string_view identity):
    identity_(identity)
  {
    using namespace BI;
    if (identity.empty())
    {
      throw std::runtime_error("shm channel identity is empty");
    }
    ipc_ptr_ = std::make_shared<interprocessMechanism<IPC_t>>(identity_);
  }

  void shmChannel::initialize(std::string_view identity)
  {
    using namespace BI;
    identity_ = identity;
    if (identity.empty())
    {
      throw std::runtime_error("shm channel identity is empty");
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
      LOG_WARN("Ring buffer start index {} will overlap end index {}, ring buffer have not content", \
        ringBuffer_raw_ptr_->startIndex_ + 1, (ringBuffer_raw_ptr_->endIndex_ + 0));
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

    if (ringBuffer_raw_ptr_->endIndex_ == SHM_INVALID_INDEX)
    {
      // This scenario is ring buffer is initial.
      //Lock start index
      BI::scoped_lock<BI::interprocess_mutex>    startIndexLock(ipc_ptr_->mechanism_raw_ptr_->startIndex_mutex_);
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

  bool shmIndexRingBuffer::watchEndIndex(ringBufferIndexBlockType &indexBlock)
  {
    BI::scoped_lock<BI::interprocess_mutex>       endLock(ipc_ptr_->mechanism_raw_ptr_->endIndex_mutex_, BI::defer_lock);
    endLock.lock();
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
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }

  bool shmIndexRingBuffer::stopWatchEndIndex()
  {
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
      throw std::runtime_error("allocate too big share memory block");
    }
    std::vector<uint32_t>   indexVec;
    indexVec.reserve(needBlockNum);

    for (uint32_t i = 0; i < needBlockNum; i++)
    {
      auto index = requireOneBlock();
      if (index == SHM_INVALID_INDEX)
      {
        throw std::runtime_error("too low shm resource");
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
    availMsg_mq_ptr_->receive(&index, sizeof(index), recvd_size, defaultPriority_);
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
    shmTransportImpl(std::string_view identity);
    ~shmTransportImpl() = default;
    bool write(const void *write_data, const uint32_t data_len);
    bool read(void *read_data, uint32_t &data_len, BLOCK_TYPE block_type);
    bool wait();
    bool tryRead(void *read_data, uint32_t &data_len);

    protected:
    bool publishMsg(uint32_t msgIndex, uint32_t msgSize, uint32_t timeStamp);

    bool subscribeMsg(shmIndexRingBuffer::ringBufferIndexBlockType &block);

    bool recycleMsg(uint32_t msgIndex);

    bool recycleExpireMsg();

    bool read(void *read_data, uint32_t &data_len, uint32_t msgHeadIndex, uint32_t msgSize);

    protected:
    std::string           identity_;
    void                  *msgShm_raw_ptr_;
    shmChannel            channel_;
    shmMsgPool            shmPool_;
    shmIndexRingBuffer    ringBuffer_;
    std::pair<uint32_t, shmIndexRingBuffer::ringBufferIndexBlockType>     publishedMsgPair_;
  };

  shmTransport::shmTransport()
  {
  }

  shmTransport::~shmTransport()
  {
  }

  shmTransport::shmTransport(std::string_view identity)
  {
    impl_ = std::make_unique<shmTransportImpl>(identity);
  }

  void shmTransport::initialize(std::string_view identity)
  {
    impl_ = std::make_unique<shmTransportImpl>(identity);
  }


  bool shmTransport::write(const void *write_data, const uint32_t data_len)
  {
    return impl_->write(write_data, data_len);
  }

  bool shmTransport::read(void *read_data, uint32_t &data_len, BLOCK_TYPE block_type)
  {
    return impl_->read(read_data, data_len, block_type);
  }

  bool shmTransport::wait()
  {
    return PROCESS_SUCCESS;
  }

  shmTransport::shmTransportImpl::shmTransportImpl(std::string_view identity):
    identity_(identity),
    channel_(CHANNEL_PREFIX + identity_),
    shmPool_(),
    ringBuffer_(RING_BUFFER_PREFIX + identity_)
  {
    msgShm_raw_ptr_ = shmPool_.getMsgRawBuffer();
  }

  bool shmTransport::shmTransportImpl::write(const void *write_data, const uint32_t data_len)
  {
    auto msg_vec = shmPool_.requireMsgShm(data_len);
    assert(msg_vec.size() != 0 && "can not allocate shm");

    msgType   *prev_msg = nullptr;
    uint32_t written_len = 0;
    auto wait2writeLen = data_len;
    for (auto msgIndex : msg_vec)
    {
      auto msgBlock = FIND_SHARE_MEM_BLOCK_ADDR(msgShm_raw_ptr_, msgIndex);
      auto msgBlockIns = new(msgBlock) msgType;
      if (wait2writeLen <= SHM_BLOCK_CONTENT_SIZE)
      {
        std::memcpy(msgBlockIns->content_, (char*)write_data + written_len, data_len);
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
      }
      else
      {
        prev_msg = msgBlockIns;
      }
    }

    auto msgHeadIndex = msg_vec[0];

    if (publishMsg(msgHeadIndex, data_len, std::time(0)) == PROCESS_FAIL)
    {
      return PROCESS_FAIL;
    }

    channel_.notifyAll();

    return PROCESS_SUCCESS;
  }

  bool shmTransport::shmTransportImpl::read(void *read_data, uint32_t &data_len, BLOCK_TYPE block_type)
  {
    //Block until message arrival
    if (block_type == BLOCK_TYPE::BLOCK)
    {
      channel_.waitNotify();

    }
    else
    {
      auto result = channel_.tryWaitNotify();
      if (result == false)
      {
        return PROCESS_FAIL;
      }
    }

    shmIndexRingBuffer::ringBufferIndexBlockType block;
    if (subscribeMsg(block) == PROCESS_SUCCESS)
    {
      auto result = read(read_data, data_len, block.shmMsgIndex_, block.msgSize_);
      ringBuffer_.stopWatchEndIndex();
      return result;
    }
    else
    {
      ringBuffer_.stopWatchEndIndex();
      return PROCESS_FAIL;
    }
  }

  bool shmTransport::shmTransportImpl::tryRead(void *read_data, uint32_t &data_len)
  {
    return PROCESS_SUCCESS;
  }


  bool shmTransport::shmTransportImpl::publishMsg(uint32_t msgIndex, uint32_t msgSize, uint32_t timeStamp)
  {
    shmIndexRingBuffer::ringBufferIndexBlockType ringBufferBlock;
    uint32_t      storePosition;
    ringBufferBlock.shmMsgIndex_ = msgIndex;
    ringBufferBlock.msgSize_ = msgSize;
    ringBufferBlock.timeStamp_ = timeStamp;
    auto result = ringBuffer_.moveEndIndex(ringBufferBlock, storePosition);

    if (result == shmIndexRingBuffer::PROCESS_RESULT::FAIL)
    {
      LOG_ERROR("can not write to ring buffer");
      return PROCESS_FAIL;
    }
    else if (result == shmIndexRingBuffer::PROCESS_RESULT::BUFFER_FILL)
    {
      recycleExpireMsg();
      result = ringBuffer_.moveEndIndex(ringBufferBlock, storePosition);
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
      // publishedMsgPair_.first = storePosition;
      // publishedMsgPair_.second = ringBufferBlock;
      return PROCESS_SUCCESS;
    }
    return PROCESS_SUCCESS;
  }

  bool shmTransport::shmTransportImpl::subscribeMsg(shmIndexRingBuffer::ringBufferIndexBlockType &block)
  {
    if (ringBuffer_.watchEndIndex(block) == PROCESS_FAIL)
    {
      return PROCESS_FAIL;
    }
    else
    {
      return PROCESS_SUCCESS;
    }
  }

  bool shmTransport::shmTransportImpl::recycleMsg(uint32_t msgIndex)
  {
    assert(msgIndex < SHM_BLOCK_NUM && "msg shm index is out of range");
    for (;msgIndex != SHM_INVALID_INDEX;)
    {
      auto msgBlock = FIND_SHARE_MEM_BLOCK_ADDR(msgShm_raw_ptr_, msgIndex);
      auto msgBlockIns = reinterpret_cast<msgType*>(msgBlock);
      auto nextIndex = msgBlockIns->next_;
      shmPool_.recycleMsgShm(msgIndex);
      msgIndex = nextIndex;
    }
    return PROCESS_SUCCESS;
  }

  bool shmTransport::shmTransportImpl::recycleExpireMsg()
  {
    shmIndexRingBuffer::ringBufferIndexBlockType block;
    ringBuffer_.moveStartIndex(block);
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

    if (data_len != 0)
    {
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }

}
