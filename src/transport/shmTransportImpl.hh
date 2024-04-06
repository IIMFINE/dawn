#ifndef _SHM_TRANSPORT_IMPL_H_
#define _SHM_TRANSPORT_IMPL_H_

#include "transport.h"

namespace dawn
{
  struct efficientTpController_shm;

  struct shmTransportImpl
  {
    friend struct efficientTpController_shm;
    friend struct reliableTpController_shm;
    shmTransportImpl(std::string_view identity) :
      identity_(identity),
      lockMsgOwns_(0)
    {
      channel_ptr_ = std::make_shared<shmChannel>(kChannelPrefix + identity_);
      shmPool_ptr_ = std::make_shared<shmMsgPool>();
      ringBuffer_ptr_ = std::make_shared<shmIndexRingBuffer>(kRingBufferPrefix + identity_);
      msgShm_raw_ptr_ = shmPool_ptr_->getMsgRawBuffer();
    }

    ~shmTransportImpl() = default;

    /// @brief Write data to data space.
    ///        Property: thread safe
    /// @param write_data
    /// @param data_len
    /// @return 
    bool baseWrite(const void *write_data, const uint32_t data_len)
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
        if (wait2writeLen <= kShmBlockContentSize)
        {
          std::memcpy(msgBlockIns->content_, (char*)write_data + written_len, wait2writeLen);
        }
        else
        {
          std::memcpy(msgBlockIns->content_, (char*)write_data + written_len, kShmBlockContentSize);
          wait2writeLen -= kShmBlockContentSize;
          written_len += kShmBlockContentSize;
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

    /// @brief Read data from data space.
    ///       property: non thread safe
    /// @param read_data
    /// @param data_len
    /// @param block_type
    /// @return PROCESS_SUCCESS: read data successfully.
    bool baseRead(void *read_data, uint32_t &data_len, abstractTransport::BLOCKING_TYPE block_type)
    {
      auto subscribeLatestMsg_func = [this, &read_data, &data_len]() {
        shmIndexRingBuffer::ringBufferIndexBlockType block;
        bool result = false;
        if (subscribeLatestMsgAndSharableLock(block) == PROCESS_SUCCESS)
        {
          if (readBuffer(read_data, data_len, block.shmMsgIndex_, block.msgSize_) == PROCESS_SUCCESS)
          {
            result = true;
          }
          stopWatchRingBuffer();
        }
        return result;
      };

      //Block until message arrival
      if (block_type == abstractTransport::BLOCKING_TYPE::BLOCK)
      {
        channel_ptr_->waitNotify();
        return subscribeLatestMsg_func();
      }
      else
      {
        if (channel_ptr_->tryWaitNotify() == false)
        {
          return PROCESS_FAIL;
        }

        if (subscribeLatestMsg_func() == true)
        {
          return PROCESS_SUCCESS;
        }
        else
        {
          return PROCESS_FAIL;
        }
      }

      return PROCESS_FAIL;
    }

    bool readSpecificIndexMsg(void *read_data, uint32_t &data_len, uint32_t msgIndex, shmIndexRingBuffer::ringBufferIndexBlockType &block)
    {
      bool result = false;
      if (ringBuffer_ptr_->watchSpecificIndexBuffer(msgIndex, block) == PROCESS_SUCCESS)
      {
        if (readBuffer(read_data, data_len, block.shmMsgIndex_, block.msgSize_) == PROCESS_SUCCESS)
        {
          result = true;
        }
        ringBuffer_ptr_->stopWatchSpecificIndexBuffer();
      }
      if (result == true)
      {
        return PROCESS_SUCCESS;
      }
      return PROCESS_FAIL;
    }

    bool wait()
    {
      return PROCESS_SUCCESS;
    }

    protected:
    bool publishMsg(uint32_t msgIndex, uint32_t msgSize)
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

    /// @brief Read the latest index block from ring buffer and will shared lock ring buffer.
    ///        Need to use unlockRingBuffer() to unlock end index of ring buffer.
    /// @param block 
    /// @return PROCESS_SUCCESS if read index block successfully and will lock end index, otherwise return PROCESS_FAIL.
    bool subscribeLatestMsgAndSharableLock(shmIndexRingBuffer::ringBufferIndexBlockType &block)
    {
      if (watchRingBuffer() == PROCESS_SUCCESS)
      {
        if (ringBuffer_ptr_->getLatestBuffer(block) == PROCESS_FAIL)
        {
          stopWatchRingBuffer();
          return PROCESS_FAIL;
        }
        return PROCESS_SUCCESS;
      }
      return PROCESS_FAIL;
    }

    bool checkMsgIndexValid(uint32_t msgIndex)
    {
      return ringBuffer_ptr_->checkIndexValid(msgIndex);
    }

    /// @brief shared lock ring buffer.
    /// @return PROCESS_SUCCESS if lock ring buffer successfully, otherwise return PROCESS_FAIL.
    bool watchRingBuffer()
    {
      std::unique_lock<std::shared_mutex>     lock(lockMsgMutex_);
      if (lockMsgOwns_.load(std::memory_order_acquire) == 0)
      {
        if (ringBuffer_ptr_->sharedLockBuffer() == PROCESS_SUCCESS)
        {
          lockMsgOwns_++;
          return PROCESS_SUCCESS;
        }
        else
        {
          return PROCESS_FAIL;
        }
      }
      else
      {
        lockMsgOwns_++;
        return PROCESS_SUCCESS;
      }
    }

    /// @brief Unlock end index of ring buffer.
    /// @return void
    void stopWatchRingBuffer()
    {
      std::unique_lock<std::shared_mutex>     lock(lockMsgMutex_);
      if (lockMsgOwns_.load(std::memory_order_acquire) == 1)
      {
        if (ringBuffer_ptr_->sharedUnlockBuffer() == PROCESS_SUCCESS)
        {
          lockMsgOwns_--;
        }
      }
      else
      {
        lockMsgOwns_--;
      }
    }

    /// @brief Read data at ring buffer start position.
    /// @param msgIndex position
    /// @param block content
    /// @return PROCESS_SUCCESS if read data successfully, otherwise return PROCESS_FAIL.
    bool requireStartMsg(uint32_t &msgIndex, shmIndexRingBuffer::ringBufferIndexBlockType &block)
    {
      return ringBuffer_ptr_->getStartIndex(msgIndex, block);
    }

    std::vector<uint32_t> retryRequireMsgShm(uint32_t data_size)
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

    bool recycleMsg(uint32_t msgIndex)
    {
      assert(msgIndex < kShmBlockNum && "msg shm index is out of range");
      for (;msgIndex != kShmInvalidIndex;)
      {
        auto msgBlock = FIND_SHARE_MEM_BLOCK_ADDR(msgShm_raw_ptr_, msgIndex);
        auto msgBlockIns = reinterpret_cast<msgType*>(msgBlock);
        auto nextIndex = msgBlockIns->next_;
        shmPool_ptr_->recycleMsgShm(msgIndex);
        msgIndex = nextIndex;
      }
      return PROCESS_SUCCESS;
    }

    bool recycleExpireMsg()
    {
      shmIndexRingBuffer::ringBufferIndexBlockType block;
      if (ringBuffer_ptr_->moveStartIndex(block) == PROCESS_FAIL)
      {
        return PROCESS_FAIL;
      }

      return recycleMsg(block.shmMsgIndex_);
    }

    bool readBuffer(void *read_data, uint32_t &data_len, uint32_t msgHeadIndex, uint32_t msgSize)
    {
      auto msgIndex = msgHeadIndex;
      data_len = 0;
      if (msgIndex == kShmInvalidIndex || msgSize == 0)
      {
        return PROCESS_FAIL;
      }

      for (;msgIndex != kShmInvalidIndex;)
      {
        auto msgBlock = FIND_SHARE_MEM_BLOCK_ADDR(msgShm_raw_ptr_, msgIndex);
        auto msgBlockIns = reinterpret_cast<msgType*>(msgBlock);

        if ((msgSize - data_len) < kShmBlockContentSize)
        {
          std::memcpy(((char*)read_data + data_len), msgBlockIns->content_, (msgSize - data_len));
          data_len += (msgSize - data_len);
        }
        else
        {
          std::memcpy(((char*)read_data + data_len), msgBlockIns->content_, kShmBlockContentSize);
          data_len += kShmBlockContentSize;
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

    protected:
    std::string           identity_;
    void                  *msgShm_raw_ptr_;
    std::atomic<uint32_t>         lockMsgOwns_;
    std::shared_mutex             lockMsgMutex_;
    std::shared_ptr<shmChannel>            channel_ptr_;
    std::shared_ptr<shmMsgPool>            shmPool_ptr_;
    std::shared_ptr<shmIndexRingBuffer>    ringBuffer_ptr_;
  };
}

#endif
