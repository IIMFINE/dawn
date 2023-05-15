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
    shmIndexRingBuffer::ringBufferIndexBlockType block;
    auto  subscribeLatestMsg_func = [&block, this, &read_data, &data_len]() {
      bool result = false;
      if (shmImpl_->subscribeMsgAndLock(block) == PROCESS_SUCCESS)
      {
        if (tasteMsgType(&block) == tpController::MSG_FRESHNESS::FRESH)
        {
          updateLatestMsg(&block);
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

  tpController::MSG_FRESHNESS efficientTpController_shm::tasteMsgType(std::any msg)
  {
    auto currentMsg_ptr = std::any_cast<shmIndexRingBuffer::ringBufferIndexBlockType*>(msg);
    std::shared_lock  lock(mutex_);

    if (currentMsg_ptr == nullptr)
    {
      return tpController::MSG_FRESHNESS::NO_TASTE;
    }
    if (latestMsg_.timeStamp_ < currentMsg_ptr->timeStamp_)
    {
      return tpController::MSG_FRESHNESS::FRESH;
    }
    else
    {
      return tpController::MSG_FRESHNESS::STALE;
    }
  }

  bool efficientTpController_shm::updateLatestMsg(std::any msg)
  {
    auto msg_ptr = std::any_cast<shmIndexRingBuffer::ringBufferIndexBlockType*>(msg);
    assert(msg_ptr != nullptr && "update msg is nullptr");
    std::unique_lock lock(mutex_);
    if (std::memcmp(&latestMsg_, msg_ptr, sizeof(shmIndexRingBuffer::ringBufferIndexBlockType)) == 0)
    {
      return PROCESS_SUCCESS;
    }
    else if (latestMsg_.timeStamp_ > msg_ptr->timeStamp_)
    {
      return PROCESS_FAIL;
    }
    else
    {
      std::memcpy(&latestMsg_, msg_ptr, sizeof(shmIndexRingBuffer::ringBufferIndexBlockType));
      return PROCESS_SUCCESS;
    }
  }
}
