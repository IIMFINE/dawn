#include "discovery/discovery.h"
#include "common/memoryPool.h"
#include <functional>
#include <memory>

namespace dawn
{

  int discovery::checkMessageIntact(std::shared_ptr<identityMessageFormat> message, int message_len)
  {
    if (message != nullptr)
    {
      return PROCESS_FAIL;
    }
    if (message->topicLen_ + (int)sizeof(identityMessageFormat) == message_len)
    {
      return PROCESS_SUCCESS;
    }
    return PROCESS_FAIL;
  }

  netDiscovery::netDiscovery(std::string_view multicastGroupIp, std::string_view receiverLocalIp,
    std::string_view unicastIp, std::string_view topic, std::string_view participantName, identityMessageFormat::FLAG_TYPE participantFlag):
    multicast_(std::make_shared<multicast>(multicastGroupIp, receiverLocalIp)), participantName_(participantName),
    topic_(topic),
    participantId_(std::hash<std::string_view>{}(participantName)),
    participantFlag_(participantFlag),
    multicastGroupIp_(multicastGroupIp)
  {
    auto self_identity_mem = allocMem(sizeof(identityMessageFormat) + topic_.length());

    intrinsic_identity_sptr_ = std::shared_ptr<identityMessageFormat>(new(self_identity_mem) identityMessageFormat(topic_, participantId_, participantFlag),
      [](identityMessageFormat* mem) {

        freeMem(mem);
      });

    if (intrinsic_udp_.initialize(unicastIp) == PROCESS_FAIL)
    {
      throw std::runtime_error("dawn: can not initialize intrinsic_udp_");
    }
  }

  netDiscovery::netDiscovery(std::string_view multicastGroupIp, unsigned short monitorPort,
    std::string_view unicastIp, std::string_view topic, std::string_view participantName, identityMessageFormat::FLAG_TYPE participantFlag):
    multicast_(std::make_shared<multicast>(multicastGroupIp, monitorPort)), participantName_(participantName),
    topic_(topic),
    participantId_(std::hash<std::string_view>{}(participantName)),
    participantFlag_(participantFlag),
    multicastGroupIp_(multicastGroupIp)
  {
    auto self_identity_mem = allocMem(sizeof(identityMessageFormat) + topic_.length());

    intrinsic_identity_sptr_ = std::shared_ptr<identityMessageFormat>(new(self_identity_mem) identityMessageFormat(topic_, participantId_, participantFlag),
      [](identityMessageFormat* mem) {

        freeMem(mem);
      });

    if (intrinsic_udp_.initialize(unicastIp) == PROCESS_FAIL)
    {
      throw std::runtime_error("dawn: can not initialize intrinsic_udp_");
    }
  }

  int netDiscovery::initialize(std::string_view multicastGroupIp, std::string_view receiverLocalIp,
    std::string_view unicastIp, std::string_view topic, std::string_view participantName, identityMessageFormat::FLAG_TYPE participantFlag)
  {
    participantName_ = participantName;
    participantId_ = std::hash<std::string_view>{}(participantName);
    multicast_ = std::make_shared<multicast>(multicastGroupIp, receiverLocalIp);
    participantFlag_ = participantFlag;
    multicastGroupIp_ = multicastGroupIp;
    if (intrinsic_udp_.initialize(unicastIp))
    {
      return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
  }

  std::shared_ptr<identityMessageFormat> netDiscovery::discoveryParticipants()
  {
    std::shared_ptr<identityMessageFormat> \
      message_sptr_(reinterpret_cast<identityMessageFormat*>(allocMem(UDP_MTU)), freeMem<identityMessageFormat>);
    struct sockaddr_in clientIpaddr;
    if (auto len = multicast_->recvFromSocket(reinterpret_cast<char*>(message_sptr_.get()), UDP_MTU, clientIpaddr); len == PROCESS_FAIL)
    {
      LOG_ERROR("can not read multicast message");
      return nullptr;
    }
    else
    {
      //check message is intact
      if (checkMessageIntact(message_sptr_, len) == PROCESS_SUCCESS)
      {
        return message_sptr_;
      }
      else
      {
        return nullptr;
      }
    }
  }

  int netDiscovery::pronouncePresence(std::string_view port)
  {
    //todo add port
    multicastGroupIp_.append(":");
    multicastGroupIp_.append(port);
    if (multicast_->send2Socket(reinterpret_cast<char*>(intrinsic_identity_sptr_.get()), sizeof(identityMessageFormat) + topic_.length(), \
      multicastGroupIp_) == PROCESS_FAIL)
    {
      return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
  }

  int netDiscovery::isTargetParticipant(std::shared_ptr<identityMessageFormat> targetMessage)
  {
    if (targetMessage != nullptr)
    {
      if (targetMessage->participantId_ != participantId_)
      {
        if (targetMessage->extractTopic() == topic_)
        {
          if (static_cast<identityMessageFormat::FLAG_TYPE>(targetMessage->flag_) != participantFlag_)
          {
            return PROCESS_SUCCESS;
          }
        }
      }
    }
    return PROCESS_FAIL;
  }

  identityMessageFormat::identityMessageFormat(std::string_view topic, uint64_t participantId, FLAG_TYPE flag):
    topicLen_(topic.length()), participantId_(participantId), flag_(static_cast<unsigned char>(flag))
  {
    memcpy(topicName_, static_cast<std::string>(topic).c_str(), topicLen_);
  }

  bool identityMessageFormat::isIdentityMessageValid()
  {
    if (magicNum_ == IDENTITY_MESSAGE_MAGIC_NUM)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  void identityMessageFormat::fillIdentityMessage(std::string_view topic, uint64_t participantId, FLAG_TYPE flag)
  {
    topicLen_ = topic.length();
    memcpy(topicName_, static_cast<std::string>(topic).c_str(), topicLen_);
    participantId_ = participantId;
    flag_ = static_cast<unsigned char>(flag);
  }

  std::string_view identityMessageFormat::extractTopic()
  {
    return topicName_;
  }

  bool identityMessageFormat::operator==(const identityMessageFormat &instance)
  {
    if (topicLen_ == instance.topicLen_)
    {
      if (memcmp(instance.topicName_, topicName_, topicLen_) == 0)
      {
        return true;
      }
    }
    return false;
  }

  bool operator==(const identityMessageFormat &instance, const identityMessageFormat::FLAG_TYPE &flag)
  {
    if (static_cast<char>(flag) == instance.flag_)
    {
      return true;
    }
    return false;
  }

  bool operator==(const identityMessageFormat::FLAG_TYPE &flag, const identityMessageFormat &instance)
  {
    if (static_cast<char>(flag) == instance.flag_)
    {
      return true;
    }
    return false;
  }

}

