#ifndef _SHM_TRANSPORT_IMPL_H_
#define _SHM_TRANSPORT_IMPL_H_

#include "transport.h"

namespace dawn
{
  struct efficientTpController_shm;

  struct shmTransportImpl
  {
    friend struct efficientTpController_shm;
    shmTransportImpl(std::string_view identity) :
      identity_(identity)
    {
      channel_ptr_ = std::make_shared<shmChannel>(CHANNEL_PREFIX + identity_);
      shmPool_ptr_ = std::make_shared<shmMsgPool>();
      ringBuffer_ptr_ = std::make_shared<shmIndexRingBuffer>(RING_BUFFER_PREFIX + identity_);
      msgShm_raw_ptr_ = shmPool_ptr_->getMsgRawBuffer();
    }

    ~shmTransportImpl() = default;

    /// @brief Write data to data space.
    ///       property: thread safe
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
        if (subscribeMsgAndLock(block) == PROCESS_SUCCESS)
        {
          if (readBuffer(read_data, data_len, block.shmMsgIndex_, block.msgSize_) == PROCESS_SUCCESS)
          {
            result = true;
          }
          unlockRingBuffer();
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
        auto result = channel_ptr_->tryWaitNotify();
        if (result == false)
        {
          return PROCESS_FAIL;
        }
        return subscribeLatestMsg_func();
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

    /// @brief Read the latest index block from ring buffer and will lock ring buffer.
    ///        Need to use unlockRingBuffer() to unlock end index of ring buffer.
    /// @param block 
    /// @return PROCESS_SUCCESS if read index block successfully and will lock end index, otherwise return PROCESS_FAIL.
    bool subscribeMsgAndLock(shmIndexRingBuffer::ringBufferIndexBlockType &block)
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

    /// @brief Unlock end index of ring buffer.
    /// @return
    void unlockRingBuffer()
    {
      if (lockMsgFlag_ == true)
      {
        ringBuffer_ptr_->stopWatchRingBuffer();
        lockMsgFlag_ = false;
      }
    }

    /// @brief 
    /// @param block 
    /// @return 
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

    protected:
    std::string           identity_;
    void                  *msgShm_raw_ptr_;
    volatile bool         lockMsgFlag_;
    std::shared_ptr<shmChannel>            channel_ptr_;
    std::shared_ptr<shmMsgPool>            shmPool_ptr_;
    std::shared_ptr<shmIndexRingBuffer>    ringBuffer_ptr_;
  };
}

#endif
