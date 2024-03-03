#ifndef __MULTICAST__H_
#define __MULTICAST__H_

#include <string>

#include "net.h"
#include "type.h"

namespace dawn
{
  // constexpr const char *INVALID_IP = "0";
  // constexpr const char *LOCAL_HOST = "127.0.0.1";
  class multicast: public abstractSocket
  {
    public:
    multicast() = delete;
    /// @brief create sockets to send multicast datagram and receive multicast datagram, 
    ///        this class will always create a local fd to send datagram.
    /// @param multicastGroupIpAddr if want don't need receive datagram socket and set INVALID_IP
    /// @param monitorPort use to bind local interface to receive multicast datagram, and it is
    ///                     same as the port of multicast IP.
    /// @param sendIpAddr if want don't need datagram socket bind a ipv4 address and set INVALID_IP.
    multicast(std::string_view multicastGroupIpAddr, unsigned short monitorPort = 0, std::string_view sendIpAddr = INVALID_IP);

    /// @brief create sockets to send multicast datagram and receive multicast datagram,
    ///        this class will always create a local fd to send datagram.
    /// @param multicastGroupIpAddr 
    /// @param monitorIpAddr a specific interface to receive multicast datagram
    /// @param sendIpAddr
    multicast(std::string_view multicastGroupIpAddr, std::string_view monitorIpAddr, std::string_view sendIpAddr = INVALID_IP);
    virtual ~multicast();

    /// @brief bind a local socket which is used to send multicast datagram,
    ///         but it usually doesn't need to bind a ipv4 address.
    /// @param ipAddr e.g. 192.168.0.1:2152
    /// @return
    virtual int initialize(std::string_view ipAddr) override;
    virtual int bindSocket(std::string_view ipAddr) override;

    /// @brief join multicast group with a specific multicast ip and set local interface port
    /// @param multicastGroupIpAddr e.g. 224.0.0.1
    /// @param monitorPort 2152
    /// @return
    int participateMulticastGroup(std::string_view multicastGroupIpAddr, unsigned short monitorPort);

    /// @brief join multicast group with a specific multicast ip and set local interface port
    /// @param multicastGroupIpAddr 
    /// @param monitorIpAddr
    /// @return 
    int participateMulticastGroup(std::string_view multicastGroupIpAddr, std::string_view monitorIpAddr);

    using abstractSocket::send2Socket;
    /// @brief send a datagram to multicast address
    /// @param data
    /// @param dataLen
    /// @param sendIpAddr will be ignored
    /// @return
    virtual int send2Socket(char *data, uint32_t dataLen, std::string_view sendIpAddr) override;

    using abstractSocket::recvFromSpecificSocket;
    virtual int recvFromSocket(char *recvData, size_t dataBuffLen) override;

    virtual int recvFromSocket(char *recvData, size_t dataBuffLen, struct sockaddr_in &clientIpAddr) override;

    int returnSendBindFd();

    protected:
    int sendBindFd_ = -1;
    int receiveBindFd_ = -1;
  };

}

#endif
