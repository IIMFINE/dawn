#ifndef _DISCOVERY_H_
#define _DISCOVERY_H_
#include <string>
#include <string_view>

#include "common/multicast.h"

// abbreviation
// sptr: shared pointer
// uptr: unique pointer

namespace dawn
{
    constexpr uint32_t IDENTITY_MESSAGE_MAGIC_NUM = 0xba0ba0fcU;
#pragma pack(1)
    struct identityMessageFormat
    {
        enum class FLAG_TYPE
        {
            // mean nothing, it should not exist in discovery network.
            NONE = 0,
            // WRITE determinate node type is publisher
            WRITE = 1,
            // READ determinate node type is publisher
            READ = 1 << 1
        };
        uint32_t magicNum_ = IDENTITY_MESSAGE_MAGIC_NUM;
        uint16_t topicLen_;
        uint64_t participantId_;
        // flag_ is a bit field
        unsigned char flag_ = 0;
        char topicName_[0];
        identityMessageFormat() = default;
        identityMessageFormat(std::string_view topic, uint64_t participantId, FLAG_TYPE flag = FLAG_TYPE::NONE);
        bool isIdentityMessageValid();
        void fillIdentityMessage(std::string_view topic, uint64_t participantId, FLAG_TYPE flag = FLAG_TYPE::NONE);
        std::string_view extractTopic();

        bool operator==(const identityMessageFormat &instance);
        friend bool operator==(const identityMessageFormat &instance, const FLAG_TYPE &flag);
        friend bool operator==(const FLAG_TYPE &flag, const identityMessageFormat &instance);
    };
#pragma pack()

    struct discovery
    {
        discovery() = default;
        virtual ~discovery() = default;
        virtual std::shared_ptr<identityMessageFormat> discoveryParticipants() = 0;
        virtual int pronouncePresence(std::string_view port) = 0;
        virtual int isTargetParticipant(std::shared_ptr<identityMessageFormat> targetMessage) = 0;
        virtual int checkMessageIntact(std::shared_ptr<identityMessageFormat> message, int message_len);
    };

    struct sharedMemDiscovery : discovery
    {
        sharedMemDiscovery() = default;
        sharedMemDiscovery(std::string_view sharedMemName, std::string_view topic, uint64_t participantId,
                           identityMessageFormat::FLAG_TYPE participantFlag, int sharedMemSize);
        virtual ~sharedMemDiscovery();
        int initialize(std::string_view sharedMemName, std::string_view topic, uint64_t participantId,
                       identityMessageFormat::FLAG_TYPE participantFlag, int sharedMemSize);
        virtual std::shared_ptr<identityMessageFormat> discoveryParticipants() override;
        virtual int pronouncePresence(std::string_view port) override;
        virtual int isTargetParticipant(std::shared_ptr<identityMessageFormat> targetMessage) override;
    };

    struct netDiscovery : discovery
    {
        netDiscovery() = default;
        /// @brief create a discovery participant to discovery other remote participant and pronounce itself identity
        /// @param multicastGroupIp take part in a multicast group
        /// @param receiverLocalIp bind a ip address to receive multicast message, but it can be neglected.
        /// @param unicastIp ip address to bind unicast interface. e.g. "192.168.0.1:2152"
        /// @param topic used to subscribe or publish
        /// @param participantName identity information
        /// @param participantFlag used to define self role is a publisher or subscriber.
        netDiscovery(std::string_view multicastGroupIp, std::string_view receiverLocalIp,
                     std::string_view unicastIp, std::string_view topic, std::string_view participantName, identityMessageFormat::FLAG_TYPE participantFlag);

        /// @brief
        /// @param multicastGroupIp
        /// @param monitorPort
        /// @param unicastIp
        /// @param topic
        /// @param participantName
        /// @param participantFlag
        netDiscovery(std::string_view multicastGroupIp, unsigned short monitorPort,
                     std::string_view unicastIp, std::string_view topic, std::string_view participantName, identityMessageFormat::FLAG_TYPE participantFlag);

        virtual ~netDiscovery() = default;

        /// @brief initialize a discovery participant to discovery other remote participant and pronounce itself identity
        /// @param multicastGroupIp take part in a multicast group
        /// @param receiverLocalIp  bind a ip address to receive multicast message, but it can be neglected.
        /// @param unicastIp ip address to bind unicast interface. e.g. "192.168.0.1:2152"
        /// @param topic used to subscribe or publish
        /// @param participantName identity information
        /// @param participantFlag used to define self role is a publisher or subscriber.
        /// @return PROCESS_SUCCESS, PROCESS_FAIL
        int initialize(std::string_view multicastGroupIp, std::string_view receiverLocalIp,
                       std::string_view unicastIp, std::string_view topic, std::string_view participantName, identityMessageFormat::FLAG_TYPE participantFlag);

        virtual std::shared_ptr<identityMessageFormat> discoveryParticipants() override;
        virtual int pronouncePresence(std::string_view port) override;
        virtual int isTargetParticipant(std::shared_ptr<identityMessageFormat> targetMessage) override;
        using discovery::checkMessageIntact;

    protected:
        std::shared_ptr<multicast> multicast_;
        std::string participantName_;
        std::string topic_;
        std::size_t participantId_;
        identityMessageFormat::FLAG_TYPE participantFlag_;
        std::shared_ptr<identityMessageFormat> intrinsic_identity_sptr_;
        std::vector<udpTransport> unicastGroup_;
        udpTransport intrinsic_udp_;
        std::string multicastGroupIp_;
    };

}

#endif
