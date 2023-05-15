#ifndef __SHM_TRANSPORT_CONTROLLER_H__
#define __SHM_TRANSPORT_CONTROLLER_H__

#include "qosController.h"
#include "shmTransport.h"

namespace dawn
{
  struct shmTransportImpl;

  struct efficientTpController_shm : public tpController
  {
    efficientTpController_shm() = delete;
    efficientTpController_shm(std::unique_ptr<shmTransportImpl> &&shmTp);
    efficientTpController_shm(std::unique_ptr<shmTransportImpl> &&shmTp, std::any config);
    virtual ~efficientTpController_shm() = default;

    /// @brief Deliver config to decide transport control.
    /// @param ringBuffer_ 
    /// @return 
    virtual bool initialize(std::any config) override;
    virtual bool write(const void *write_data, const uint32_t data_len) override;
    virtual bool read(void *read_data, uint32_t &data_len, abstractTransport::BLOCKING_TYPE block_type) override;
    virtual qosCfg::QOS_TYPE getQosType() override;

    /// @brief deliver a message pointer.Using std::any to implement polymorphism.
    /// @param msg message pointer
    /// @return 
    MSG_FRESHNESS tasteMsgType(shmIndexRingBuffer::ringBufferIndexBlockType &msg);

    /// @brief Record a message pointer like a ring buffer index block or a entire message, which depends on the scenario.
    ///       No thread safe.
    /// @param msg 
    /// @return PROCESS_SUCCESS if the msg is recorded successfully, otherwise return PROCESS_FAILED.
    bool updateLatestMsg(shmIndexRingBuffer::ringBufferIndexBlockType &msg);
    private:
    std::shared_mutex                                 mutex_;
    shmIndexRingBuffer::ringBufferIndexBlockType      latestMsg_;
    qosCfg                                            qosCfg_;
    std::unique_ptr<shmTransportImpl>                 shmImpl_;
  };

  struct reliableTpController_shm : public tpController
  {
    reliableTpController_shm() = delete;
    reliableTpController_shm(std::unique_ptr<shmTransportImpl> &&shmTp);
    reliableTpController_shm(std::unique_ptr<shmTransportImpl> &&shmTp, std::any config);
    virtual ~reliableTpController_shm() = default;

    /// @brief Deliver config to decide transport control.
    /// @param ringBuffer_ 
    /// @return 
    virtual bool initialize(std::any config) override;
    virtual bool write(const void *write_data, const uint32_t data_len) override;
    virtual bool read(void *read_data, uint32_t &data_len, abstractTransport::BLOCKING_TYPE block_type) override;
    virtual qosCfg::QOS_TYPE getQosType() override;

    MSG_FRESHNESS tasteMsgAtStartIndex(shmIndexRingBuffer::ringBufferIndexBlockType &startMsg, uint32_t ringBufferIndex);
    MSG_FRESHNESS tasteMsg(shmIndexRingBuffer::ringBufferIndexBlockType &msg, uint32_t ringBufferIndex);
    bool updateLastMsgAndIndex(shmIndexRingBuffer::ringBufferIndexBlockType &msg, uint32_t ringBufferIndex);
    bool updateStartMsgAndIndex(shmIndexRingBuffer::ringBufferIndexBlockType &msg, uint32_t ringBufferIndex);
    private:
    std::shared_mutex                                 mutex_;
    shmIndexRingBuffer::ringBufferIndexBlockType      lastMsg_;
    shmIndexRingBuffer::ringBufferIndexBlockType      startMsg_;
    uint32_t                                          stayIndex_;
    uint32_t                                          startIndex_;
    qosCfg                                            qosCfg_;
    std::unique_ptr<shmTransportImpl>                 shmImpl_;
  };
};

#endif
