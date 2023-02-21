#include "shmTransport.h"
#include <cassert>
#include "setLogger.h"
#include <ctime>

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
    condition_ptr_ = std::make_shared<named_condition>\
      (open_or_create, identity_.c_str());
    mutex_ptr_ = std::make_shared<named_mutex>\
      (open_or_create, identity_.c_str());
  }

  shmChannel::~shmChannel()
  {
    using namespace BI;
    named_condition::remove(identity_.c_str());
    named_mutex::remove(identity_.c_str());
  }

  void shmChannel::initialize(std::string_view identity)
  {
    using namespace BI;
    identity_ = identity;
    if (identity.empty())
    {
      throw std::runtime_error("shm channel identity is empty");
    }
    condition_ptr_ = std::make_shared<named_condition>\
      (open_or_create, identity_.c_str());
    mutex_ptr_ = std::make_shared<named_mutex>\
      (open_or_create, identity_.c_str());
  }

  void shmChannel::notifyAll()
  {
    condition_ptr_->notify_all();
  }

  void shmChannel::waitNotify()
  {
    using namespace BI;
    scoped_lock<named_mutex> lock(*(mutex_ptr_.get()));
    condition_ptr_->wait(lock);
  }

  shmIndexRingBuffer::shmIndexRingBuffer(std::string_view identity):
    shmIdentity_(identity),
    startIndexIdentity_(RING_BUFFER_START_PREFIX + shmIdentity_),
    endIndexIdentity_(RING_BUFFER_END_PREFIX + shmIdentity_)
  {
    using namespace BI;
    startIndex_mutex_ptr_ = std::make_shared<named_mutex>(open_or_create, startIndexIdentity_.c_str());
    endIndex_mutex_ptr_ = std::make_shared<named_mutex>(open_or_create, endIndexIdentity_.c_str());
    ringBufferShm_ptr_ = std::make_shared<shared_memory_object>(open_or_create, shmIdentity_.c_str(), read_write);
    ringBufferShm_ptr_->truncate(SHM_RING_BUFFER_SIZE);
    ringBufferShmRegion_ptr_ = std::make_shared<mapped_region>(*(ringBufferShm_ptr_.get()), read_write);
    ringBuffer_raw_ptr_ = reinterpret_cast<ringBufferType*>(ringBufferShmRegion_ptr_->get_address());
    ringBufferInitialFlag_ptr_ = std::make_shared<named_semaphore>(open_or_create, shmIdentity_.c_str(), 1);
    ///@note ensure ringBuffer_raw_ptr_ just initialize only one time.
    if (ringBufferInitialFlag_ptr_->try_wait())
    {
      ringBuffer_raw_ptr_->startIndex_ = SHM_INVALID_INDEX;
      ringBuffer_raw_ptr_->endIndex_ = SHM_INVALID_INDEX;
      ringBuffer_raw_ptr_->totalIndex_ = SHM_BLOCK_NUM;
    }
  }

  shmIndexRingBuffer::~shmIndexRingBuffer()
  {
    using namespace BI;
    named_mutex::remove(startIndexIdentity_.c_str());
    named_mutex::remove(endIndexIdentity_.c_str());
    shared_memory_object::remove(shmIdentity_.c_str());
    named_semaphore::remove(shmIdentity_.c_str());
  }

  void shmIndexRingBuffer::initialize(std::string_view identity)
  {
    using namespace BI;
    shmIdentity_ = identity;
    startIndexIdentity_ = RING_BUFFER_START_PREFIX + shmIdentity_;
    endIndexIdentity_ = RING_BUFFER_END_PREFIX + shmIdentity_;
    ringBufferShm_ptr_ = std::make_shared<shared_memory_object>(open_or_create, shmIdentity_.c_str(), read_write);
    ringBufferShm_ptr_->truncate(SHM_RING_BUFFER_SIZE);
    ringBufferShmRegion_ptr_ = std::make_shared<mapped_region>(*(ringBufferShm_ptr_.get()), read_write);
    ringBuffer_raw_ptr_ = reinterpret_cast<ringBufferType*>(ringBufferShmRegion_ptr_->get_address());
    ringBufferInitialFlag_ptr_ = std::make_shared<named_semaphore>(open_or_create, shmIdentity_.c_str(), 1);
    ///@note ensure ringBuffer_raw_ptr_ just initialize only one time.
    if (ringBufferInitialFlag_ptr_->try_wait())
    {
      ringBuffer_raw_ptr_->startIndex_ = SHM_INVALID_INDEX;
      ringBuffer_raw_ptr_->endIndex_ = SHM_INVALID_INDEX;
      ringBuffer_raw_ptr_->totalIndex_ = SHM_BLOCK_NUM;
    }
  }

  bool shmIndexRingBuffer::moveStartIndex(ringBufferIndexBlockType &indexBlock)
  {
    BI::scoped_lock<BI::named_mutex>    startIndexLock(*(startIndex_mutex_ptr_.get()));

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

  bool shmIndexRingBuffer::moveEndIndex(ringBufferIndexBlockType &indexBlock, uint32_t &storePosition)
  {
    BI::scoped_lock<BI::named_mutex>    endIndexLock(*(endIndex_mutex_ptr_.get()));

    if (ringBuffer_raw_ptr_->endIndex_ == SHM_INVALID_INDEX)
    {
      //Lock start index
      BI::scoped_lock<BI::named_mutex>    startIndexLock(*(startIndex_mutex_ptr_.get()));
      ringBuffer_raw_ptr_->endIndex_ = 0;
      ringBuffer_raw_ptr_->startIndex_ = 0;
      std::memcpy(&ringBuffer_raw_ptr_->ringBufferBlock_[ringBuffer_raw_ptr_->endIndex_], &indexBlock, sizeof(ringBufferIndexBlockType));
      ringBuffer_raw_ptr_->endIndex_ = calculateIndex(ringBuffer_raw_ptr_->endIndex_ + 1);
      return PROCESS_SUCCESS;
    }
    else if (ringBuffer_raw_ptr_->startIndex_ == ringBuffer_raw_ptr_->endIndex_)
    {
      LOG_WARN("Ring buffer start index {} will overlap end index {}, ring buffer have not content", \
        (ringBuffer_raw_ptr_->startIndex_ + 1), (ringBuffer_raw_ptr_->endIndex_ + 0));
      return PROCESS_FAIL;
    }
    else if (calculateIndex(ringBuffer_raw_ptr_->endIndex_ + 1) == ringBuffer_raw_ptr_->startIndex_)
    {
      LOG_WARN("Ring buffer's full filled");
      return PROCESS_FAIL;
    }
    else if (calculateIndex(ringBuffer_raw_ptr_->endIndex_ + 1) != ringBuffer_raw_ptr_->startIndex_)
    {
      std::memcpy(&ringBuffer_raw_ptr_->ringBufferBlock_[ringBuffer_raw_ptr_->endIndex_], &indexBlock, sizeof(ringBufferIndexBlockType));
      storePosition = ringBuffer_raw_ptr_->endIndex_;
      ringBuffer_raw_ptr_->endIndex_ = calculateIndex(ringBuffer_raw_ptr_->endIndex_ + 1);
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }

  bool shmIndexRingBuffer::watchStartIndex(ringBufferIndexBlockType &indexBlock)
  {
    ///@todo Return block content pointed by start index and lock start index until completed operation.
    return PROCESS_SUCCESS;
  }

  bool shmIndexRingBuffer::watchEndIndex(ringBufferIndexBlockType &indexBlock)
  {
    BI::scoped_lock<BI::named_mutex>       endLock(*(endIndex_mutex_ptr_), BI::defer_lock);
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
    mqIdentity_(MESSAGE_QUEUE_PREFIX + identity_)
  {
    using namespace BI;
    msgPoolInitialFlag_ptr_ = std::make_shared<named_semaphore>(open_or_create, identity_.c_str(), 1);
    availMsg_mq_ptr_ = std::make_shared<message_queue>(open_or_create, mqIdentity_.c_str(), SHM_BLOCK_NUM, sizeof(uint32_t));
    msgBufferShm_ptr_ = std::make_shared<shared_memory_object>(open_or_create, identity_.c_str(), read_write);
    msgBufferShm_ptr_->truncate(SHM_TOTAL_SIZE);
    msgBufferShmRegion_ptr_ = std::make_shared<mapped_region>(*(msgBufferShm_ptr_.get()), read_write);
    msgBuffer_raw_ptr_ = reinterpret_cast<void*>(msgBufferShmRegion_ptr_->get_address());

    if (msgPoolInitialFlag_ptr_->try_wait())
    {
      for (uint32_t i = 0; i < SHM_BLOCK_NUM; i++)
      {
        availMsg_mq_ptr_->send(&i, sizeof(i), defaultPriority_);
      }
    }
  }

  shmMsgPool::~shmMsgPool()
  {
    using namespace BI;
    message_queue::remove(mqIdentity_.c_str());
    named_semaphore::remove(identity_.c_str());
    shared_memory_object::remove(identity_.c_str());
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
    bool read(void *read_data, uint32_t &data_len);
    bool wait();

    protected:
    bool publishMsg(uint32_t msgIndex, uint32_t msgSize, uint32_t timeStamp);

    bool subscribeMsg(shmIndexRingBuffer::ringBufferIndexBlockType &block);

    bool recycleMsg(uint32_t msgIndex);

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

  bool shmTransport::read(void *read_data, uint32_t &data_len)
  {
    return impl_->read(read_data, data_len);
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

  bool shmTransport::shmTransportImpl::read(void *read_data, uint32_t &data_len)
  {
    //Block until message arrival
    channel_.waitNotify();
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

  bool shmTransport::shmTransportImpl::publishMsg(uint32_t msgIndex, uint32_t msgSize, uint32_t timeStamp)
  {
    shmIndexRingBuffer::ringBufferIndexBlockType ringBufferBlock;
    uint32_t      storePosition;
    ringBufferBlock.shmMsgIndex_ = msgIndex;
    ringBufferBlock.msgSize_ = msgSize;
    ringBufferBlock.timeStamp_ = timeStamp;

    if (ringBuffer_.moveEndIndex(ringBufferBlock, storePosition) == PROCESS_FAIL)
    {
      LOG_ERROR("can not write to ring buffer");
      return PROCESS_FAIL;
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
