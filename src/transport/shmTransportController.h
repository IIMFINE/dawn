#ifndef __SHM_TRANSPORT_CONTROLLER_H__
#define __SHM_TRANSPORT_CONTROLLER_H__
#include <atomic>

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

    /// @brief Read data efficiently. Support multi-thread read.
    ///        Property: thread safe.
    /// @param read_data pointer to store data.
    /// @param data_len reference to pass read data length.
    /// @param block_type BLOCK or NON_BLOCK read
    /// @return PROCESS_SUCCESS if read data successfully, otherwise return
    /// PROCESS_FAILED.
    virtual bool read(void *read_data, uint32_t &data_len, abstractTransport::BLOCKING_TYPE block_type) override;

    virtual qosCfg::QOS_TYPE getQosType() override;

    /// @brief deliver a message pointer.Using std::any to implement
    /// polymorphism.
    /// @param msg message pointer
    /// @return
    MSG_FRESHNESS tasteMsgType(shmIndexRingBuffer::ringBufferIndexBlockType &msg);

    /// @brief Record a message pointer like a ring buffer index block or a
    /// entire message, which depends on the scenario.
    ///       No thread safe.
    /// @param msg
    /// @return PROCESS_SUCCESS if the msg is recorded successfully, otherwise
    /// return PROCESS_FAILED.
    bool updateLatestMsg(shmIndexRingBuffer::ringBufferIndexBlockType &msg);

private:
    std::shared_mutex mutex_;
    shmIndexRingBuffer::ringBufferIndexBlockType latestMsg_;
    qosCfg qosCfg_;
    std::unique_ptr<shmTransportImpl> shmImpl_;
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
    /// @brief Write data reliably.
    ///        Property: thread safe.
    /// @param write_data Data to be sent.
    /// @param data_len data length.
    /// @return PROCESS_SUCCESS if write data successfully, otherwise return
    /// PROCESS_FAILED.
    virtual bool write(const void *write_data, const uint32_t data_len) override;
    /// @brief Read data reliably.
    ///        Property: thread safe.
    /// @param read_data Data to be store.
    /// @param data_len data length.
    /// @param block_type Blocking or non-blocking read.
    /// @return PROCESS_SUCCESS if read data successfully, otherwise return
    /// PROCESS_FAILED.
    virtual bool read(void *read_data, uint32_t &data_len, abstractTransport::BLOCKING_TYPE block_type) override;
    virtual qosCfg::QOS_TYPE getQosType() override;

    MSG_FRESHNESS tasteMsgAtStartIndex(shmIndexRingBuffer::ringBufferIndexBlockType &startMsg, uint32_t ringBufferIndex);
    MSG_FRESHNESS tasteMsg(shmIndexRingBuffer::ringBufferIndexBlockType &msg, uint32_t ringBufferIndex);
    bool updateLastMsg(shmIndexRingBuffer::ringBufferIndexBlockType &msg);
    bool updateStartMsgAndIndex(shmIndexRingBuffer::ringBufferIndexBlockType &msg, uint32_t ringBufferIndex);

    /// @brief Update stay index.
    /// @param updatedIndex Return the caller the updated stay index.
    /// @param ringBufferIndex If ringBufferIndex is kShmInvalidIndex, then
    /// stayIndex_ will be added by 1.
    ///                        Otherwise, stayIndex_ will be updated by
    ///                        ringBufferIndex.
    /// @return PROCESS_SUCCESS if the stay index is updated successfully,
    /// otherwise return PROCESS_FAILED.
    bool updateStayIndex(uint32_t &updatedIndex, uint32_t ringBufferIndex);

    /// @brief Update stay index.
    /// @param updatedIndex Return the caller the updated stay index.
    /// @param ringBufferIndex If ringBufferIndex is kShmInvalidIndex, then
    /// stayIndex_ will be added by 1.
    ///                        Otherwise, stayIndex_ will be updated by
    ///                        ringBufferIndex.
    /// @return PROCESS_SUCCESS if the stay index is updated successfully,
    /// otherwise return PROCESS_FAILED.
    bool updateStayIndex(uint32_t &updatedIndex);

private:
    std::shared_mutex lastMsgMutex_;
    std::shared_mutex startMsgMutex_;
    shmIndexRingBuffer::ringBufferIndexBlockType lastBlock_;
    shmIndexRingBuffer::ringBufferIndexBlockType startBlock_;

    /// @todo use atomic variable to replace the following two variables.
    std::atomic<uint32_t> stayIndex_ = kShmInvalidIndex;
    uint32_t recordStartIndex_ = kShmInvalidIndex;
    qosCfg qosCfg_;
    std::unique_ptr<shmTransportImpl> shmImpl_;
};
};  // namespace dawn

#endif
