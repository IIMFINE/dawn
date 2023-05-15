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
      if (shmImpl_->subscribeMsgAndLock(block) == PROCESS_SUCCESS)
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

  //add reliableTpController_shm read function
  bool reliableTpController_shm::read(void *read_data, uint32_t &data_len, abstractTransport::BLOCKING_TYPE block_type)
  {
    shmIndexRingBuffer::ringBufferIndexBlockType block;
    auto  subscribeLatestMsg_func = [&block, this, &read_data, &data_len]() {

      if (shmImpl_->requireStartMsg())
      {

      }
    };
  }

  qosCfg::QOS_TYPE reliableTpController_shm::getQosType()
  {
    return qosCfg_.qosType_;
  }

  tpController::MSG_FRESHNESS reliableTpController_shm::tasteMsgAtStartIndex(shmIndexRingBuffer::ringBufferIndexBlockType &startMsg, uint32_t startRingBufferIndex)
  {
    if (startIndex_ != startRingBufferIndex)
    {
      return tpController::MSG_FRESHNESS::FRESH;
    }
    else if (startIndex_ == startRingBufferIndex && startMsg.timeStamp_ != startMsg_.timeStamp_)
    {
      /// @todo Check msg is timeout by checking time stamp.
      return tpController::MSG_FRESHNESS::FRESH;
    }
    else
    {
      return tpController::MSG_FRESHNESS::STALE;
    }
    return tpController::MSG_FRESHNESS::NO_TASTE;
  }


  tpController::MSG_FRESHNESS reliableTpController_shm::tasteMsg(shmIndexRingBuffer::ringBufferIndexBlockType &msg, uint32_t ringBufferIndex)
  {
    if (msg.timeStamp_ > lastMsg_.timeStamp_)
    {
      return tpController::MSG_FRESHNESS::FRESH;
    }
    else if (msg.timeStamp_ == lastMsg_.timeStamp_ && ringBufferIndex == stayIndex_)
    {
      // This scenario happens when time stamp is in next period.
      return tpController::MSG_FRESHNESS::FRESH;
    }
    else if (msg.timeStamp_ < lastMsg_.timeStamp_ && ringBufferIndex == stayIndex_)
    {
      return tpController::MSG_FRESHNESS::FRESH;
    }
    return tpController::MSG_FRESHNESS::NO_TASTE;
  }

  bool reliableTpController_shm::updateLastMsgAndIndex(shmIndexRingBuffer::ringBufferIndexBlockType &msg, uint32_t ringBufferIndex)
  {
    if (msg.timeStamp_ > lastMsg_.timeStamp_ || ringBufferIndex != stayIndex_)
    {
      std::memcpy(&lastMsg_, &msg, sizeof(shmIndexRingBuffer::ringBufferIndexBlockType));
      stayIndex_ = ringBufferIndex;
    }
    return PROCESS_SUCCESS;
  }

  bool reliableTpController_shm::updateStartMsgAndIndex(shmIndexRingBuffer::ringBufferIndexBlockType &msg, uint32_t ringBufferIndex)
  {
    if (msg.timeStamp_ != startMsg_.timeStamp_ || ringBufferIndex != startIndex_)
    {
      std::memcpy(&startMsg_, &msg, sizeof(shmIndexRingBuffer::ringBufferIndexBlockType));
      startIndex_ = ringBufferIndex;
    }
    return PROCESS_SUCCESS;
  }


}
