#ifndef _SHM_TRANSPORT_H_
#define _SHM_TRANSPORT_H_
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <memory>
#include <set>

#include "common/setLogger.h"
#include "qosController.h"
#include "transport.h"

namespace dawn
{
    /// @brief shm ring buffer block content: |shm_message_index, message_size, time stamp|...|...|...
    ///        each size of block is 12 byte.
    constexpr const uint8_t kShmIndexBlockSize = 4 * 3;
    constexpr const uint8_t kShmIndexRingBufferHeadSize = 4 * 3;
    constexpr const uint32_t kShmBlockNum = 1024 * 10;
    /// @brief BLOCK content: |next, message|...|...
    constexpr const uint32_t kShmBlockHeadSize = 4;
    constexpr const uint32_t kShmBlockContentSize = 1024;
    constexpr const uint32_t kShmBlockSize = kShmBlockContentSize + kShmBlockHeadSize;
    /// @brief total sum of content size is 10M byte.
    constexpr const uint32_t kShmTotalContentSize = kShmBlockContentSize * kShmBlockNum;
    constexpr const uint32_t kShmTotalSize = kShmBlockNum * kShmBlockSize;
    constexpr const uint32_t kShmRingBufferSize = kShmIndexBlockSize * kShmBlockNum + kShmIndexRingBufferHeadSize;
    constexpr const uint32_t kShmInvalidIndex = 0xffffffff;
    constexpr const char *kShmMsgIdentity = "dawn_msg";
    constexpr const char *kRingBufferPrefix = "ring.";
    constexpr const char *kChannelPrefix = "ch.";
    constexpr const char *kMessageQueuePrefix = "mq.";
    constexpr const char *kMechanismPrefix = "msm.";

#define FIND_SHARE_MEM_BLOCK_ADDR(head, index) (((char *)head) + (index * kShmBlockSize))
#define FIND_NEXT_MESSAGE_BLOCK_ADDR(head, index) (((char *)head) + (reinterpret_cast<msgType *>((char *)head) + (index * kShmBlockSize)->next_))

    struct shmTransport;
    struct qosCfg;

    namespace BI = boost::interprocess;

    template <typename T>
    struct interprocessMechanism
    {
        interprocessMechanism() = default;
        /// @brief Provide a mechanism to share mutex, condition, semaphore, etc.
        ///        *WARN*  But exclude message queue, shared memory.
        /// @param identity The identity for boost interprocess mechanism.
        interprocessMechanism(std::string_view identity);
        ~interprocessMechanism() = default;
        void initialize(std::string_view identity);

        std::string identity_;
        T *mechanism_raw_ptr_;
        std::shared_ptr<BI::shared_memory_object> shmObject_ptr_;
        std::shared_ptr<BI::mapped_region> shmRegion_ptr_;
    };

    template <typename T>
    interprocessMechanism<T>::interprocessMechanism(std::string_view identity) : identity_(identity)
    {
        using namespace BI;
        try
        {
            shmObject_ptr_ = std::make_shared<shared_memory_object>(create_only, identity_.c_str(), read_write);
            shmObject_ptr_->truncate(sizeof(T));
            shmRegion_ptr_ = std::make_shared<mapped_region>(*(shmObject_ptr_.get()), read_write);
            mechanism_raw_ptr_ = reinterpret_cast<T *>(shmRegion_ptr_->get_address());
            new (mechanism_raw_ptr_) T;
        }
        catch (const std::exception &e)
        {
            LOG_WARN("interprocessMechanism exception: {}", e.what());
            shmObject_ptr_ = std::make_shared<shared_memory_object>(open_only, identity_.c_str(), read_write);
            shmRegion_ptr_ = std::make_shared<mapped_region>(*(shmObject_ptr_.get()), read_write);
            mechanism_raw_ptr_ = reinterpret_cast<T *>(shmRegion_ptr_->get_address());
        }
    }

    template <typename T>
    void interprocessMechanism<T>::initialize(std::string_view identity)
    {
        using namespace BI;
        identity_ = identity;
        shmObject_ptr_ = std::make_shared<shared_memory_object>(open_or_create, identity_.c_str(), read_write);
        shmObject_ptr_->truncate(sizeof(T));
        shmRegion_ptr_ = std::make_shared<mapped_region>(*(shmObject_ptr_.get()), read_write);
        mechanism_raw_ptr_ = shmRegion_ptr_->get_address();
        new (mechanism_raw_ptr_) T;
    }

    struct shmChannel
    {
        struct IPC_t
        {
            BI::interprocess_condition condition_;
            BI::interprocess_mutex mutex_;
        };

        shmChannel() = default;
        shmChannel(std::string_view identity);
        ~shmChannel() = default;
        void initialize(std::string_view identity);
        void notifyAll();
        void waitNotify();
        template <typename FUNC_T>
        void waitNotify(FUNC_T &&func);
        bool tryWaitNotify(uint32_t microseconds = 1);
        template <typename FUNC_T>
        bool tryWaitNotify(FUNC_T &&func, uint32_t microsecond = 1);

    protected:
        std::string identity_;
        std::shared_ptr<interprocessMechanism<IPC_t>> ipc_ptr_;
    };

    /// @brief Using share memory to deliver message,
    ///    because of thinking about the scenario for one publisher to multiple subscribers.
    struct shmIndexRingBuffer
    {
        friend struct shmTransport;
        struct ringBufferIndexBlockType
        {
            uint32_t shmMsgIndex_;
            uint32_t msgSize_;
            uint32_t timeStamp_;
        };
        // create a ipc mechanism to protect ring buffer
        struct __attribute__((packed)) ringBufferType
        {
            uint32_t startIndex_;
            uint32_t endIndex_;
            uint32_t totalIndex_;
            ringBufferIndexBlockType ringBufferBlock_[0];
        };

        enum class PROCESS_RESULT
        {
            SUCCESS,
            FAIL,
            BUFFER_FILL,
            BUFFER_EMPTY
        };

        struct IPC_t
        {
            IPC_t() : ringBufferInitializedFlag_(1)
            {
            }
            ~IPC_t() = default;
            BI::interprocess_sharable_mutex startIndex_mutex_;
            BI::interprocess_sharable_mutex endIndex_mutex_;
            BI::interprocess_semaphore ringBufferInitializedFlag_;
        };

        shmIndexRingBuffer() = default;
        shmIndexRingBuffer(std::string_view identity);
        ~shmIndexRingBuffer() = default;
        void initialize(std::string_view identity);

        /// @brief Start index move a step and return content pointed by previous start index.
        /// @param prevIndex
        /// @return PROCESS_SUCCESS success; PROCESS_FAIL failed; ring buffer block
        bool moveStartIndex(ringBufferIndexBlockType &indexBlock);

        /// @brief Move end index meaning have to append new block to ring buffer.
        /// @param indexBlock
        /// @return PROCESS_RESULT
        PROCESS_RESULT moveEndIndex(ringBufferIndexBlockType &indexBlock, uint32_t &storePosition);

        bool getStartBuffer(ringBufferIndexBlockType &indexBlock);

        bool getStartBuffer(ringBufferIndexBlockType &indexBlock, uint32_t &index);

        bool getLatestBuffer(ringBufferIndexBlockType &indexBlock);

        bool getLatestBuffer(ringBufferIndexBlockType &indexBlock, uint32_t &index);

        bool getSpecificIndexBuffer(uint32_t index, ringBufferIndexBlockType &indexBlock);

        /// @brief Watch start index and lock start index mutex until operation finish.
        /// @param indexBlock
        /// @return PROCESS_SUCCESS: lock successfully and read ring buffer in start index.
        bool watchStartBuffer(ringBufferIndexBlockType &indexBlock);

        /// @brief Watch start index and lock start index mutex until operation finish.
        /// @param indexBlock
        /// @return PROCESS_SUCCESS: lock successfully and read ring buffer in start index.
        bool watchStartBuffer(ringBufferIndexBlockType &indexBlock, uint32_t &index);

        /// @brief Watch end index and lock end index mutex until watch operation asked to stop.
        /// @param indexBlock
        /// @return PROCESS_SUCCESS: lock successfully and read buffer.
        ///         PROCESS_FAIL: buffer is empty or buffer isn't initialize and will not lock.
        bool watchLatestBuffer(ringBufferIndexBlockType &indexBlock);

        /// @brief Watch end index and lock end index mutex until watch operation asked to stop.
        /// @param indexBlock
        /// @return PROCESS_SUCCESS: lock successfully and read buffer.
        ///         PROCESS_FAIL: buffer is empty or buffer isn't initialize and will not lock.
        bool watchLatestBuffer(ringBufferIndexBlockType &indexBlock, uint32_t &index);

        /// @brief Unlock start index mutex.
        /// @return PROCESS_SUCCESS: unlock successfully.
        bool stopWatchStartIndex();

        /// @brief Unlock ring buffer.
        /// @return PROCESS_SUCCESS: unlock successfully.
        bool stopWatchLatestBuffer();

        /// @brief Unlock ring buffer.
        /// @return PROCESS_SUCCESS: unlock successfully.
        bool stopWatchSpecificIndexBuffer();

        /// @brief Get start index for multi-purpose.
        /// @param storeIndex reference to store start index.
        /// @param indexBlock reference to store index block content.
        /// @return PROCESS_SUCCESS: get start index successfully.
        bool getStartIndex(uint32_t &storeIndex, ringBufferIndexBlockType &indexBlock);

        /// @brief Watch specific index and lock specific index mutex until watch operation asked to stop.
        /// @param index Pass specific index to watch.
        /// @param indexBlock Outside buffer to store index block content.
        /// @return PROCESS_SUCCESS: lock successfully and read buffer.
        ///         PROCESS_FAIL: index is invalid or buffer isn't initialize and will not lock.
        bool watchSpecificIndexBuffer(uint32_t index, ringBufferIndexBlockType &indexBlock);

        /// @brief Exclusively lock start index mutex and end index mutex.
        /// @return PROCESS_SUCCESS: lock successfully.
        bool exclusiveLockBuffer();

        /// @brief Unlock start index mutex and end index mutex.
        /// @return PROCESS_SUCCESS: unlock successfully.
        bool exclusiveUnlockBuffer();

        /// @brief sharable lock start index mutex and end index mutex.
        /// @return PROCESS_SUCCESS: lock successfully.
        bool sharedLockBuffer();

        /// @brief sharable unlock start index mutex and end index mutex.
        /// @return PROCESS_SUCCESS: unlock successfully.
        bool sharedUnlockBuffer();

        /// @brief Pass indexBlock to compare the the block pointing by start index. If they are same, start index will move.
        /// @param indexBlock
        /// @return PROCESS_SUCCESS:
        bool compareAndMoveStartIndex(ringBufferIndexBlockType &indexBlock);

        /// @brief Check index is valid or not.
        /// @param index msg index
        /// @return
        bool checkIndexValid(uint32_t index);

        /// @brief Calculate index conformed to ring buffer size.
        /// @param ringBufferIndex
        /// @return Calculate result.
        uint32_t calculateIndex(uint32_t ringBufferIndex);

        /// @brief
        /// @param ringBufferIndex
        /// @return
        uint32_t calculateLastIndex(uint32_t ringBufferIndex);

        std::shared_ptr<interprocessMechanism<IPC_t>> ipc_ptr_;
        std::shared_ptr<BI::shared_memory_object> ringBufferShm_ptr_;
        std::shared_ptr<BI::mapped_region> ringBufferShmRegion_ptr_;
        BI::sharable_lock<BI::interprocess_sharable_mutex> endIndex_shared_lock_;
        BI::sharable_lock<BI::interprocess_sharable_mutex> startIndex_shared_lock_;
        BI::scoped_lock<BI::interprocess_sharable_mutex> endIndex_exclusive_lock_;
        BI::scoped_lock<BI::interprocess_sharable_mutex> startIndex_exclusive_lock_;

        ringBufferType *ringBuffer_raw_ptr_;
        std::string shmIdentity_;
        std::string mechanismIdentity_;
    };

    struct __attribute__((packed)) msgType
    {
        uint32_t next_ = kShmInvalidIndex;
        char content_[0];
    };

    struct shmMsgPool
    {
        struct IPC_t
        {
            IPC_t() : msgPoolInitialFlag_(1)
            {
            }
            ~IPC_t() = default;
            BI::interprocess_semaphore msgPoolInitialFlag_;
        };

        shmMsgPool(std::string_view identity = kShmMsgIdentity);
        ~shmMsgPool() = default;

        std::vector<uint32_t> requireMsgShm(uint32_t data_size);
        uint32_t requireOneBlock();
        bool recycleMsgShm(uint32_t index);
        void *getMsgRawBuffer();

    protected:
        uint32_t defaultPriority_;
        std::shared_ptr<interprocessMechanism<IPC_t>> ipc_ptr_;
        std::shared_ptr<BI::message_queue> availMsg_mq_ptr_;
        std::shared_ptr<BI::shared_memory_object> msgBufferShm_ptr_;
        std::shared_ptr<BI::mapped_region> msgBufferShmRegion_ptr_;
        void *msgBuffer_raw_ptr_;
        std::string identity_;
        std::string mechanismIdentity_;
        std::string mqIdentity_;
    };

    struct shmTransport : abstractTransport
    {
        shmTransport();
        shmTransport(std::string_view identity, std::shared_ptr<qosCfg> qosCfg_ptr = std::make_shared<qosCfg>());
        ~shmTransport();
        shmTransport(const shmTransport &) = delete;
        shmTransport &operator=(const shmTransport &) = delete;
        void initialize(std::string_view identity, std::shared_ptr<qosCfg> qosCfg_ptr = std::make_shared<qosCfg>());

        /// @brief Write data to shared memory.
        /// @param write_data data
        /// @param data_len data length
        /// @return PROCESS_SUCCESS: write successfully. Otherwise, write fail.
        virtual bool write(const void *write_data, const uint32_t data_len) override;

        /// @brief Read data from shared memory. It is a blocking read function.
        /// @param read_data read data buffer
        /// @param data_len read data length
        /// @return PROCESS_SUCCESS: read successfully. Otherwise, read fail.
        virtual bool read(void *read_data, uint32_t &data_len) override;

        /// @brief Read data from shared memory. It can choose blocking or non-blocking.
        /// @param read_data read data buffer
        /// @param data_len read data length
        /// @param block_type blocking or non-blocking
        /// @return PROCESS_SUCCESS: read successfully. Otherwise, read fail.
        virtual bool read(void *read_data, uint32_t &data_len, BLOCKING_TYPE block_type) override;

        virtual bool wait() override;

        std::unique_ptr<tpController> tpController_ptr_;
    };

    template <typename FUNC_T>
    void shmChannel::waitNotify(FUNC_T &&func)
    {
        using namespace BI;
        scoped_lock<interprocess_mutex> lock(ipc_ptr_->mechanism_raw_ptr_->mutex_);
        ipc_ptr_->mechanism_raw_ptr_->condition_.wait(lock, std::forward<FUNC_T>(func));
    }

    template <typename FUNC_T>
    bool shmChannel::tryWaitNotify(FUNC_T &&func, uint32_t microsecond)
    {
        using namespace BI;
        scoped_lock<interprocess_mutex> lock(ipc_ptr_->mechanism_raw_ptr_->mutex_);
        return ipc_ptr_->mechanism_raw_ptr_->condition_.timed_wait(lock,
                                                                   boost::posix_time::microsec_clock::universal_time() + boost::posix_time::microseconds(microsecond), func);
    }

    /// @brief Get a microsecond timestamp.
    ///         !!!WARN!!! This timestamp will be truncated to 32 bits.
    /// @return microsecond timestamp.
    uint32_t getTimestamp();

}

#endif
