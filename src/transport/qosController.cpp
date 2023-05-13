#include "qosController.h"

namespace dawn
{

  reliableQosCfg::reliableQosCfg() :
    qosCfg()
  {
    qosType_ = qosCfg::QOS_TYPE::RELIABLE;
  }

  efficientQosController_shm::efficientQosController_shm(std::any config):
    qosCfg_(std::any_cast<qosCfg>(config))
  {
  }

  bool efficientQosController_shm::initialize(std::any config)
  {
    qosCfg_ = std::any_cast<qosCfg>(config);
    return PROCESS_SUCCESS;
  }

  qosCfg::QOS_TYPE efficientQosController_shm::getQosType()
  {
    return qosCfg_.qosType_;
  }

  qosController::MSG_FRESHNESS efficientQosController_shm::tasteMsgType(std::any msg)
  {
    auto currentMsg_ptr = std::any_cast<shmIndexRingBuffer::ringBufferIndexBlockType*>(msg);
    std::shared_lock  lock(mutex_);

    if (currentMsg_ptr == nullptr)
    {
      return qosController::MSG_FRESHNESS::NO_TASTE;
    }
    if (latestMsg_.timeStamp_ < currentMsg_ptr->timeStamp_)
    {
      return qosController::MSG_FRESHNESS::FRESH;
    }
    else
    {
      return qosController::MSG_FRESHNESS::STALE;
    }
  }

  bool efficientQosController_shm::updateLatestMsg(std::any msg)
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

  // reliableQosController_shm::reliableQosController_shm(std::any config)
  // {
  //   qosCfg_ = std::any_cast<qosCfg>(config);
  // }

}
