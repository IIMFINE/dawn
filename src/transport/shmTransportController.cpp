#include "shmTransportController.h"
#include "shmTransport.h"
#include "shmTransportImpl.hh"

namespace dawn
{
  efficientTpController_shm::efficientTpController_shm(std::unique_ptr<shmTransportImpl> &&shmImpl) :
    shmImpl_(std::forward<std::unique_ptr<shmTransportImpl>>(shmImpl))
  {
  }

  efficientTpController_shm::efficientTpController_shm(std::unique_ptr<shmTransportImpl> &&shmImpl, std::any config) :
    qosCfg_(std::any_cast<qosCfg>(config)),
    shmImpl_(std::forward<std::unique_ptr<shmTransportImpl>>(shmImpl))
  {
  }

  bool efficientTpController_shm::initialize(std::any config)
  {
    qosCfg_ = std::any_cast<qosCfg>(config);
    return PROCESS_SUCCESS;
  }

  bool efficientTpController_shm::write(const void *write_data, const uint32_t data_len)
  {
    return shmImpl_->baseWrite(write_data, data_len);
  }

  bool efficientTpController_shm::read(void *read_data, uint32_t &data_len, abstractTransport::BLOCKING_TYPE block_type)
  {
    auto  subscribeLatestMsg_func = [this, &read_data, &data_len]() {
      shmIndexRingBuffer::ringBufferIndexBlockType block;
      bool result = false;
      if (shmImpl_->subscribeLatestMsgAndLock(block) == PROCESS_SUCCESS)
      {
        if (tasteMsgType(block) == tpController::MSG_FRESHNESS::FRESH)
        {
          updateLatestMsg(block);
          if (shmImpl_->readBuffer(read_data, data_len, block.shmMsgIndex_, block.msgSize_) == PROCESS_SUCCESS)
          {
            result = true;
          }
        }
        shmImpl_->unlockRingBuffer();
      }
      return result;
    };

    //Try to get the latest index block to taste.
    if (subscribeLatestMsg_func() == true)
    {
      return PROCESS_SUCCESS;
    }

    //Block until message arrival
    if (block_type == abstractTransport::BLOCKING_TYPE::BLOCK)
    {
      shmImpl_->channel_ptr_->waitNotify(subscribeLatestMsg_func);
      return PROCESS_SUCCESS;
    }
    else
    {
      auto result = shmImpl_->channel_ptr_->tryWaitNotify(subscribeLatestMsg_func);
      if (result == false)
      {
        return PROCESS_FAIL;
      }
    }
    return PROCESS_FAIL;
  }

  qosCfg::QOS_TYPE efficientTpController_shm::getQosType()
  {
    return qosCfg_.qosType_;
  }

  tpController::MSG_FRESHNESS efficientTpController_shm::tasteMsgType(shmIndexRingBuffer::ringBufferIndexBlockType &msg)
  {
    std::shared_lock  lock(mutex_);

    if (latestMsg_.timeStamp_ < msg.timeStamp_)
    {
      return tpController::MSG_FRESHNESS::FRESH;
    }
    else
    {
      return tpController::MSG_FRESHNESS::STALE;
    }
  }

  bool efficientTpController_shm::updateLatestMsg(shmIndexRingBuffer::ringBufferIndexBlockType  &msg)
  {
    std::unique_lock lock(mutex_);
    if (std::memcmp(&latestMsg_, &msg, sizeof(shmIndexRingBuffer::ringBufferIndexBlockType)) == 0)
    {
      return PROCESS_SUCCESS;
    }
    else if (latestMsg_.timeStamp_ > msg.timeStamp_)
    {
      return PROCESS_FAIL;
    }
    else
    {
      std::memcpy(&latestMsg_, &msg, sizeof(shmIndexRingBuffer::ringBufferIndexBlockType));
      return PROCESS_SUCCESS;
    }
  }

  reliableTpController_shm::reliableTpController_shm(std::unique_ptr<shmTransportImpl> &&shmImpl) :
    shmImpl_(std::forward<std::unique_ptr<shmTransportImpl>>(shmImpl))
  {
  }

  reliableTpController_shm::reliableTpController_shm(std::unique_ptr<shmTransportImpl> &&shmTp, std::any config) :
    qosCfg_(std::any_cast<qosCfg>(config)),
    shmImpl_(std::forward<std::unique_ptr<shmTransportImpl>>(shmTp))
  {
  }

  bool reliableTpController_shm::initialize(std::any config)
  {
    qosCfg_ = std::any_cast<qosCfg>(config);
    return PROCESS_SUCCESS;
  }

  bool reliableTpController_shm::write(const void *write_data, const uint32_t data_len)
  {
    return shmImpl_->baseWrite(write_data, data_len);
  }

  bool reliableTpController_shm::read(void *read_data, uint32_t &data_len, abstractTransport::BLOCKING_TYPE block_type)
  {
    auto  subscribeLatestMsg_func = [this, &read_data, &data_len]() {
      shmIndexRingBuffer::ringBufferIndexBlockType startIndexBlock;
      uint32_t   startIndex;
      bool result = false;
      if (shmImpl_->requireStartMsg(startIndex, startIndexBlock) == PROCESS_SUCCESS)
      {
        switch (tasteMsgAtStartIndex(startIndexBlock, startIndex))
        {
          case tpController::MSG_FRESHNESS::NEW_ROUND:
            {
              if (shmImpl_->lockRingBuffer() == PROCESS_SUCCESS)
              {
                uint32_t     startIndex;
                if (shmImpl_->ringBuffer_ptr_->watchStartBuffer(startIndexBlock, startIndex) == PROCESS_SUCCESS)
                {
                  if (shmImpl_->readBuffer(read_data, data_len, startIndexBlock.shmMsgIndex_, startIndexBlock.msgSize_) == PROCESS_SUCCESS)
                  {
                    updateStartMsgAndIndex(startIndexBlock, startIndex);
                    updateLastMsg(startIndexBlock);
                    uint32_t msgIndex;
                    updateStayIndex(msgIndex, startIndex);
                    result = true;
                  }
                }
                shmImpl_->unlockRingBuffer();
              }
            }
            break;

          case tpController::MSG_FRESHNESS::FRESH:
            {
              updateStartMsgAndIndex(startIndexBlock, startIndex);
              if (shmImpl_->lockRingBuffer() == PROCESS_SUCCESS)
              {
                uint32_t    msgIndex;
                shmIndexRingBuffer::ringBufferIndexBlockType  block;
                if (updateStayIndex(msgIndex) == PROCESS_SUCCESS)
                {
                  if (shmImpl_->ringBuffer_ptr_->watchSpecificIndexBuffer(msgIndex, block) == PROCESS_SUCCESS)
                  {
                    if (shmImpl_->readBuffer(read_data, data_len, block.shmMsgIndex_, block.msgSize_) == PROCESS_SUCCESS)
                    {
                      updateLastMsg(block);
                      result = true;
                    }
                  }
                }
                shmImpl_->unlockRingBuffer();
              }
            }
            break;

          case tpController::MSG_FRESHNESS::STALE:
            {
              if (shmImpl_->lockRingBuffer() == PROCESS_SUCCESS)
              {
                uint32_t    msgIndex;
                shmIndexRingBuffer::ringBufferIndexBlockType  block;
                if (updateStayIndex(msgIndex) == PROCESS_SUCCESS)
                {
                  if (shmImpl_->ringBuffer_ptr_->watchSpecificIndexBuffer(msgIndex, block) == PROCESS_SUCCESS)
                  {
                    if (shmImpl_->readBuffer(read_data, data_len, block.shmMsgIndex_, block.msgSize_) == PROCESS_SUCCESS)
                    {
                      updateLastMsg(block);
                      result = true;
                    }
                  }
                }
                shmImpl_->unlockRingBuffer();
              }
            }
            break;
          default:
            result = false;
            break;
        }
      }

      return result;
    };

    if (subscribeLatestMsg_func() == true)
    {
      return PROCESS_SUCCESS;
    }

    if (block_type == abstractTransport::BLOCKING_TYPE::BLOCK)
    {
      shmImpl_->channel_ptr_->waitNotify(subscribeLatestMsg_func);
      return PROCESS_SUCCESS;
    }
    else
    {
      auto result = shmImpl_->channel_ptr_->tryWaitNotify(subscribeLatestMsg_func);
      if (result == false)
      {
        return PROCESS_FAIL;
      }
    }
    return PROCESS_FAIL;
  }

  qosCfg::QOS_TYPE reliableTpController_shm::getQosType()
  {
    return qosCfg_.qosType_;
  }

  tpController::MSG_FRESHNESS reliableTpController_shm::tasteMsgAtStartIndex(shmIndexRingBuffer::ringBufferIndexBlockType &startMsg, uint32_t startRingBufferIndex)
  {
    std::shared_lock        lock(startMsgMutex_);
    if (recordStartIndex_ != SHM_INVALID_INDEX)
    {
      if (recordStartIndex_ != startRingBufferIndex)
      {
        return tpController::MSG_FRESHNESS::FRESH;
      }
      else if (recordStartIndex_ == startRingBufferIndex && startMsg.timeStamp_ != startBlock_.timeStamp_)
      {
        /// @todo Check msg is timeout by checking time stamp.
        return tpController::MSG_FRESHNESS::NEW_ROUND;
      }
      else
      {
        return tpController::MSG_FRESHNESS::STALE;
      }
    }
    else
    {
      return tpController::MSG_FRESHNESS::NEW_ROUND;
    }
    return tpController::MSG_FRESHNESS::NO_TASTE;
  }


  tpController::MSG_FRESHNESS reliableTpController_shm::tasteMsg(shmIndexRingBuffer::ringBufferIndexBlockType &msg, uint32_t ringBufferIndex)
  {
    std::shared_lock        lock(lastMsgMutex_);
    if (stayIndex_ != SHM_INVALID_INDEX)
    {
      if (msg.timeStamp_ > lastBlock_.timeStamp_)
      {
        return tpController::MSG_FRESHNESS::FRESH;
      }
      else if (msg.timeStamp_ == lastBlock_.timeStamp_ && ringBufferIndex == stayIndex_)
      {
        // This scenario happens when time stamp is in next period.
        return tpController::MSG_FRESHNESS::FRESH;
      }
      else if (msg.timeStamp_ < lastBlock_.timeStamp_ && ringBufferIndex == stayIndex_)
      {
        return tpController::MSG_FRESHNESS::FRESH;
      }
    }
    else
    {
      return tpController::MSG_FRESHNESS::FRESH;
    }
    return tpController::MSG_FRESHNESS::NO_TASTE;
  }

  bool reliableTpController_shm::updateLastMsg(shmIndexRingBuffer::ringBufferIndexBlockType &block)
  {
    std::unique_lock       shared_lock(lastMsgMutex_);
    std::memcpy(&lastBlock_, &block, sizeof(shmIndexRingBuffer::ringBufferIndexBlockType));
    return PROCESS_SUCCESS;
  }

  bool reliableTpController_shm::updateStartMsgAndIndex(shmIndexRingBuffer::ringBufferIndexBlockType &block, uint32_t ringBufferIndex)
  {
    std::unique_lock       shared_lock(startMsgMutex_);
    if (ringBufferIndex != recordStartIndex_ && block.timeStamp_ != startBlock_.timeStamp_)
    {
      std::memcpy(&startBlock_, &block, sizeof(shmIndexRingBuffer::ringBufferIndexBlockType));
      recordStartIndex_ = ringBufferIndex;
      return PROCESS_SUCCESS;
    }
    else
    {
      return PROCESS_FAIL;
    }
  }

  bool reliableTpController_shm::updateStayIndex(uint32_t &updatedIndex)
  {
    auto oldIndex = stayIndex_.load(std::memory_order_acquire);

    if (shmImpl_->checkMsgIndexValid(oldIndex) == false)
    {
      return PROCESS_FAIL;
    }

    while (stayIndex_.compare_exchange_strong(oldIndex, oldIndex + 1, std::memory_order_acq_rel) == false)
    {
      if (shmImpl_->checkMsgIndexValid(oldIndex + 1) == true)
      {
        continue;
      }
      return PROCESS_FAIL;
    }
    if (shmImpl_->checkMsgIndexValid(oldIndex + 1) == true)
    {
      updatedIndex = oldIndex + 1;
      return PROCESS_SUCCESS;
    }
    else
    {
      return PROCESS_FAIL;
    }
  }

  bool reliableTpController_shm::updateStayIndex(uint32_t &updatedIndex, uint32_t ringBufferIndex)
  {
    auto oldIndex = stayIndex_.load(std::memory_order_acquire);

    //If other thread is updating stayIndex_, then wait for it to finish.
    while (stayIndex_.compare_exchange_weak(oldIndex, ringBufferIndex, std::memory_order_acq_rel) != true)
    {
    }
    updatedIndex = ringBufferIndex;
    return PROCESS_SUCCESS;
  }

}
