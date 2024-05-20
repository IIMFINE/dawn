#include "multicast.h"

#include <string.h>

namespace dawn
{
multicast::multicast(std::string_view multicastGroupIpAddr, unsigned short monitorPort, std::string_view sendIpAddr)
{
    bindSocket(sendIpAddr);

    if (multicastGroupIpAddr != INVALID_IP)
    {
        participateMulticastGroup(multicastGroupIpAddr, monitorPort);
    }
}

multicast::multicast(std::string_view multicastGroupIpAddr, std::string_view monitorIpAddr, std::string_view sendIpAddr)
{
    bindSocket(sendIpAddr);

    if (multicastGroupIpAddr != INVALID_IP && monitorIpAddr != INVALID_IP)
    {
        participateMulticastGroup(multicastGroupIpAddr, monitorIpAddr);
    }
}

int multicast::participateMulticastGroup(std::string_view multicastGroupIpAddr, unsigned short monitorPort)
{
    LOG_DEBUG("now participate multicast group {} ", multicastGroupIpAddr);
    struct sockaddr_in translateIp;
    if (translateStringToIpAddr(multicastGroupIpAddr, translateIp) == PROCESS_FAIL)
    {
        LOG_ERROR("can not translate participating ip address");
        return PROCESS_FAIL;
    }
    receiveBindFd_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiveBindFd_ < 0)
    {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not create sock");
        return PROCESS_FAIL;
    }
    int reuse = 1;
    if (setsockopt(receiveBindFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not set SO_REUSEADDR on {}", receiveBindFd_);
        return PROCESS_FAIL;
    }
    struct sockaddr_in localSock;
    memset(&localSock, 0x0, sizeof(localSock));
    localSock.sin_family = AF_INET;
    // bind any interfaces, and don't use htonl to transform INADDR_ANY macro
    localSock.sin_addr.s_addr = htonl(INADDR_ANY);
    localSock.sin_port = htons(monitorPort);
    if (bind(receiveBindFd_, (struct sockaddr *)&localSock, sizeof(localSock)))
    {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not bind receive multicast socket");
        return PROCESS_FAIL;
    }

    // join multicast group
    struct ip_mreq multicastGroup;
    multicastGroup.imr_multiaddr.s_addr = translateIp.sin_addr.s_addr;
    multicastGroup.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(receiveBindFd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicastGroup, sizeof(multicastGroup)) < 0)
    {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not to join multicast group, multicast address {}", multicastGroupIpAddr);
        return PROCESS_FAIL;
    }
    return PROCESS_FAIL;
}

int multicast::participateMulticastGroup(std::string_view multicastGroupIpAddr, std::string_view monitorIpAddr)
{
    LOG_DEBUG("now participate multicast group {} with {} interface", multicastGroupIpAddr, monitorIpAddr);
    struct sockaddr_in translateIp;
    struct sockaddr_in translateMonitorIp;

    if (translateStringToIpAddr(multicastGroupIpAddr, translateIp) == PROCESS_FAIL)
    {
        LOG_ERROR("can not translate participating ip address");
        return PROCESS_FAIL;
    }

    if (translateStringToIpAddr(monitorIpAddr, translateMonitorIp) == PROCESS_FAIL)
    {
        LOG_ERROR("can not translate monitor ip address");
        return PROCESS_FAIL;
    }

    receiveBindFd_ = socket(PF_INET, SOCK_DGRAM, 0);
    if (receiveBindFd_ < 0)
    {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not create sock");
        return PROCESS_FAIL;
    }

    int reuse = 1;
    if (setsockopt(receiveBindFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not set SO_REUSEADDR on {}", receiveBindFd_);
        return PROCESS_FAIL;
    }

    struct sockaddr_in localSock;
    memset(&localSock, 0x0, sizeof(localSock));
    localSock.sin_family = PF_INET;
    localSock.sin_port = translateMonitorIp.sin_port;
    // bind any interfaces, and don't use htonl to transform INADDR_ANY macro
    localSock.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(receiveBindFd_, (struct sockaddr *)&localSock, sizeof(localSock)))
    {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not bind receive multicast socket");
        return PROCESS_FAIL;
    }

    // join multicast group
    struct ip_mreq multicastGroup;
    multicastGroup.imr_multiaddr.s_addr = translateIp.sin_addr.s_addr;
    multicastGroup.imr_interface.s_addr = translateMonitorIp.sin_addr.s_addr;
    if (setsockopt(receiveBindFd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicastGroup, sizeof(multicastGroup)) < 0)
    {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not to join multicast group, multicast address {}", multicastGroupIpAddr);
        return PROCESS_FAIL;
    }
    return PROCESS_FAIL;
}
multicast::~multicast()
{
    if (sendBindFd_ >= 0)
    {
        close(sendBindFd_);
    }
    if (receiveBindFd_ >= 0)
    {
        close(receiveBindFd_);
    }
}

int multicast::initialize(std::string_view ipAddr) { return bindSocket(ipAddr); }

int multicast::bindSocket(std::string_view ipAddr)
{
    sendBindFd_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sendBindFd_ == -1)
    {
        LINUX_ERROR_PRINT();
        LOG_ERROR(" cant create udp fd ");
        return PROCESS_FAIL;
    }

    // set the TTL (time to live) for the send
    constexpr unsigned char ttl = 1;
    // Use SOL_SOCKET to set SO_* flag
    if (setsockopt(sendBindFd_, SOL_SOCKET, SO_REUSEADDR, &ttl, sizeof(ttl)) < 0)
    {
        LOG_ERROR("can not set ttl {}", ttl);
        return PROCESS_FAIL;
    }

    if (ipAddr != INVALID_IP)
    {
        struct sockaddr_in translateIp;
        translateStringToIpAddr(ipAddr, translateIp);

        LOG_DEBUG("info: now try to bind the this ip {}", ipAddr);

        // don't need to bind a local interface
        if (setsockopt(sendBindFd_, IPPROTO_IP, IP_MULTICAST_IF | IP_MULTICAST_TTL, &translateIp.sin_addr, sizeof(translateIp.sin_addr)) < 0)
        {
            LINUX_ERROR_PRINT();
            LOG_ERROR("can not set {} to multicast sock", ipAddr);
            return PROCESS_FAIL;
        }
    }
    return PROCESS_SUCCESS;
}

int multicast::send2Socket(char *data, uint32_t dataLen, std::string_view sendIpAddr)
{
    LOG_DEBUG("send multicast ip {}", sendIpAddr);

    constexpr uint32_t minMulticastIp = 0xE0000000;
    struct sockaddr_in translateIp;
    translateStringToIpAddr(sendIpAddr, translateIp);

    // check four bits in front of ip address using big endian
    if ((translateIp.sin_addr.s_addr & htonl(minMulticastIp)) != htonl(minMulticastIp))
    {
        LOG_ERROR("It is a wrong multicast ip address {}", sendIpAddr);
        return PROCESS_FAIL;
    }

    if (sendto(sendBindFd_, data, dataLen, 0, (struct sockaddr *)&translateIp, sizeof(translateIp)) == -1)
    {
        LINUX_ERROR_PRINT();
        LOG_ERROR("cant send data");
        return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
}

int multicast::recvFromSocket(char *recvData, size_t dataBuffLen)
{
    if (recvData != nullptr)
    {
        socklen_t ipAddrLen = sizeof(clientIpAddr_);
        if (auto recvDataLen = recvfrom(receiveBindFd_, recvData, dataBuffLen, 0, (struct sockaddr *)&clientIpAddr_, &ipAddrLen); recvDataLen < 0)
        {
            LINUX_ERROR_PRINT();
            LOG_ERROR("can not read {}", receiveBindFd_);
            return PROCESS_FAIL;
        }
        else
        {
            return recvDataLen;
        }
    }
    else
    {
        LOG_WARN("transmit a nullptr");
        return PROCESS_FAIL;
    }
}

int multicast::recvFromSocket(char *recvData, size_t dataBuffLen, struct sockaddr_in &clientIpAddr)
{
    if (recvData != nullptr)
    {
        socklen_t ipAddrLen = sizeof(clientIpAddr);
        if (auto recvDataLen = recvfrom(receiveBindFd_, recvData, dataBuffLen, 0, (struct sockaddr *)&clientIpAddr, &ipAddrLen); recvDataLen < 0)
        {
            LINUX_ERROR_PRINT();
            LOG_ERROR("can not read {}", receiveBindFd_);
            return PROCESS_FAIL;
        }
        else
        {
            return recvDataLen;
        }
    }
    else
    {
        LOG_WARN("transmit a nullptr");
        return PROCESS_FAIL;
    }
}

int multicast::returnSendBindFd() { return sendBindFd_; }
}  // namespace dawn
