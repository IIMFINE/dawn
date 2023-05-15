#ifndef _QOS_CONFIG_H_
#define _QOS_CONFIG_H_
#include <any>
#include <shared_mutex>

#include "transport.h"

/// @note "tpController" is a abbreviation of "transport controller".
/// @note "qos" is a abbreviation of "quality of service".
/// @note "cfg" is a abbreviation of "config".
/// @note "msg" is a abbreviation of "message".
/// @note "shm" is a abbreviation of "shared memory".

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

  struct tpController
  {
    enum class MSG_FRESHNESS
    {
      FRESH,
      STALE,
      NO_TASTE
    };

    virtual ~tpController() = default;

    virtual bool initialize(std::any config) = 0;

    virtual bool write(const void *write_data, const uint32_t data_len) = 0;

    virtual bool read(void *read_data, uint32_t &data_len, abstractTransport::BLOCKING_TYPE block_type) = 0;

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
} //namespace dawn

#endif

