#ifndef _QOS_CONFIG_H_
#define _QOS_CONFIG_H_
#include <any>

#include "shmTransport.h"

namespace dawn
{
  struct qosCfg
  {
    enum class QOS_TYPE
    {
      RELIABLE,
      EFFICIENT
    };
    qosCfg() = default;
    ~qosCfg() = default;
    QOS_TYPE qosType = QOS_TYPE::EFFICIENT;
  };

  struct qosController
  {
    enum class MSG_FRESHNESS
    {
      FRESH,
      STALE,
      NO_TASTE
    };

    virtual ~qosController() = default;
    virtual bool initialize(std::any config) = 0;
    /// @brief deliver a message pointer.Using std::any to implement polymorphism.
    /// @param msg message pointer
    /// @return 
    virtual MSG_FRESHNESS tasteMsgType(std::any msg) = 0;
    /// @brief Record a message pointer like a ring buffer index block or a entire message, which depends on the scenario.
    ///       No thread safe.
    /// @param msg 
    /// @return PROCESS_SUCCESS if the msg is recorded successfully, otherwise return PROCESS_FAILED.
    virtual bool updateLatestMsg(std::any msg) = 0;
  };

  struct efficientQosController_shm: public qosController
  {
    efficientQosController_shm() = default;
    efficientQosController_shm(std::any config);
    virtual ~efficientQosController_shm() = default;
    /// @brief Deliver config to decide QoS control.
    /// @param ringBuffer_ 
    /// @return 
    virtual bool initialize(std::any config) override;
    virtual MSG_FRESHNESS tasteMsgType(std::any msg) override;
    virtual bool updateLatestMsg(std::any msg) override;
    private:
    qosCfg                                            qosCfg_;
    shmIndexRingBuffer::ringBufferIndexBlockType      latestMsg_;
  };
} //namespace dawn

#endif

