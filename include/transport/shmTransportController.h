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

    /// @brief Deliver config to decide QoS control.
    /// @param ringBuffer_ 
    /// @return 
    virtual bool initialize(std::any config) override;
    virtual bool write(const void *write_data, const uint32_t data_len) override;
    virtual bool read(void *read_data, uint32_t &data_len, abstractTransport::BLOCKING_TYPE block_type) override;
    virtual qosCfg::QOS_TYPE getQosType() override;
    virtual MSG_FRESHNESS tasteMsgType(std::any msg) override;
    virtual bool updateLatestMsg(std::any msg) override;
    private:
    std::shared_mutex                                 mutex_;
    qosCfg                                            qosCfg_;
    shmIndexRingBuffer::ringBufferIndexBlockType      latestMsg_;
    std::unique_ptr<shmTransportImpl>                    shmImpl_;
  };
};

#endif
