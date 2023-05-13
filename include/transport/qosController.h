#ifndef _QOS_CONFIG_H_
#define _QOS_CONFIG_H_
#include <any>

#include "shmTransport.h"
#include <shared_mutex>

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
    virtual ~qosCfg() = default;
    QOS_TYPE qosType_ = QOS_TYPE::EFFICIENT;
  };

  struct reliableQosCfg : public qosCfg
  {
    reliableQosCfg();
    virtual ~reliableQosCfg() = default;
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
    /// @brief Get QoS type from config.
    /// @return 
    virtual qosCfg::QOS_TYPE getQosType() = 0;
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
    virtual qosCfg::QOS_TYPE getQosType() override;
    virtual MSG_FRESHNESS tasteMsgType(std::any msg) override;
    virtual bool updateLatestMsg(std::any msg) override;
    private:
    std::shared_mutex                                  mutex_;
    qosCfg                                            qosCfg_;
    shmIndexRingBuffer::ringBufferIndexBlockType      latestMsg_;
  };

  // struct reliableQosController_shm : public qosController
  // {
  //   reliableQosController_shm() = default;
  //   reliableQosController_shm(std::any config);
  //   virtual ~reliableQosController_shm() = default;
  //   /// @brief Deliver config to decide QoS control.
  //   /// @param ringBuffer_ 
  //   /// @return 
  //   virtual bool initialize(std::any config) override;
  // virtual qosCfg::QOS_TYPE getQosType() override;
  //   virtual MSG_FRESHNESS tasteMsgType(std::any msg) override;
  //   virtual bool updateLatestMsg(std::any msg) override;
  //   private:
  //   std::shared_mutex                                  mutex_;
  //   qosCfg                                            qosCfg_;
  //   shmIndexRingBuffer::ringBufferIndexBlockType      latestMsg_;
  // };

} //namespace dawn

#endif

