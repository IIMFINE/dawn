#include "qosController.h"

namespace dawn
{

  efficientQosController_shm::efficientQosController_shm(std::any config):
    qosCfg_(std::any_cast<qosCfg>(config))
  {
  }

  bool efficientQosController_shm::initialize(std::any config)
  {
    qosCfg_ = std::any_cast<qosCfg>(config);
    return PROCESS_SUCCESS;
  }

  qosController::MSG_FRESHNESS efficientQosController_shm::tasteMsgType(std::any msg)
  {
    auto currentMsg_ptr = std::any_cast<shmIndexRingBuffer::ringBufferIndexBlockType*>(msg);
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
    if (msg_ptr == nullptr)
    {
      return PROCESS_FAIL;
    }
    if (std::memcmp(&latestMsg_, msg_ptr, sizeof(shmIndexRingBuffer::ringBufferIndexBlockType)) == 0)
    {
      return PROCESS_SUCCESS;
    }
    else
    {
      std::memcpy(&latestMsg_, msg_ptr, sizeof(shmIndexRingBuffer::ringBufferIndexBlockType));
      return PROCESS_SUCCESS;
    }
  }

}
