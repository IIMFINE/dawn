#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "net.h"
#include <errno.h>
#include <string.h>
#include <memory.h>
#include "setLogger.h"

namespace dawn
{

  abstractSocket::abstractSocket(int bindFd, struct sockaddr_in clientIpAddr):
    clientIpAddr_(clientIpAddr), bindFd_(bindFd)
  {
  }

  int abstractSocket::send2Socket(char *data, uint32_t dataLen, struct sockaddr_in clientIpAddr)
  {
    if (sendto(bindFd_, data, dataLen, 0, reinterpret_cast<struct sockaddr*>(&clientIpAddr), sizeof(clientIpAddr)) == INVALID_VALUE)
    {
      LINUX_ERROR_PRINT();
      LOG_ERROR("cant send data");
      return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
  }

  int abstractSocket::recvFromSocket(char *recvData, size_t dataBuffLen)
  {
    if (recvData != nullptr)
    {
      socklen_t ipAddrLen = sizeof(clientIpAddr_);
      if (auto recvDataLen = recvfrom(bindFd_, recvData, dataBuffLen, 0, (struct sockaddr *)&clientIpAddr_, &ipAddrLen); recvDataLen < 0)
      {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not read {}", bindFd_);
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

  int abstractSocket::recvFromSocket(char *recvData, size_t dataBuffLen, struct sockaddr_in &clientIpAddr)
  {
    if (recvData != nullptr)
    {
      socklen_t ipAddrLen = sizeof(clientIpAddr_);
      if (auto recvDataLen = recvfrom(bindFd_, recvData, dataBuffLen, 0, (struct sockaddr *)&clientIpAddr_, &ipAddrLen); recvDataLen < 0)
      {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not read {}", bindFd_);
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

  int abstractSocket::recvFromSpecificSocket(int fd, char *recvData, size_t dataBuffLen)
  {
    if (recvData != nullptr)
    {
      socklen_t ipAddrLen = sizeof(clientIpAddr_);
      if (auto recvDataLen = recvfrom(fd, recvData, dataBuffLen, 0, (struct sockaddr *)&clientIpAddr_, &ipAddrLen); recvDataLen < 0)
      {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not read {}", fd);
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

  int abstractSocket::recvFromSpecificSocket(int fd, char *recvData, size_t dataBuffLen, struct sockaddr_in &clientIpAddr)
  {
    if (recvData != nullptr)
    {
      socklen_t ipAddrLen = sizeof(clientIpAddr);
      if (auto recvDataLen = recvfrom(fd, recvData, dataBuffLen, 0, (struct sockaddr *)&clientIpAddr, &ipAddrLen); recvDataLen < 0)
      {
        LINUX_ERROR_PRINT();
        LOG_ERROR("can not read {}", fd);
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
  std::string_view abstractSocket::getSourceIpAddr()
  {
    return sourceIpStr_;
  }

  struct sockaddr_in abstractSocket::getClientIpAddr()
  {
    return clientIpAddr_;
  }

  int abstractSocket::getBindFd()
  {
    return bindFd_;
  }

  int abstractSocket::translateStringToIpAddr(const std::string_view &ipString, struct sockaddr_in &ipAddr_in)
  {
    auto portPos = std::string::npos;
    portPos = ipString.find(':');

    if (portPos == std::string::npos)
    {
      LOG_WARN("string dont contain a port {}, now bind a random port", ipString);
      struct in_addr translateIp;
      auto ipAddrNoPort = ipString;

      if (inet_pton(PF_INET, static_cast<std::string>(ipAddrNoPort).c_str(), &translateIp) == 0)
      {
        LOG_ERROR("cant translate this ip {}", ipAddrNoPort);
        return PROCESS_FAIL;
      }

      memset(&ipAddr_in, 0x0, sizeof(ipAddr_in));
      ipAddr_in.sin_family = PF_INET;
      ipAddr_in.sin_addr.s_addr = translateIp.s_addr;
      return PROCESS_SUCCESS;
    }

    struct in_addr translateIp;
    auto ipAddrNoPort = ipString.substr(0, portPos);
    if (inet_pton(PF_INET, static_cast<std::string>(ipAddrNoPort).c_str(), &translateIp) == 0)
    {
      LOG_ERROR("cant translate this ip {}", ipString);
      return PROCESS_FAIL;
    }

    memset(&ipAddr_in, 0x0, sizeof(ipAddr_in));
    ipAddr_in.sin_family = PF_INET;
    ipAddr_in.sin_addr.s_addr = translateIp.s_addr;
    auto ipAddrPort = ipString.substr(portPos + 1, ipString.size());
    ipAddr_in.sin_port = htons(atoi(static_cast<std::string>(ipAddrPort).c_str()));

    return PROCESS_SUCCESS;
  }

  udpTransport::udpTransport(int bindFd, struct sockaddr_in clientIpAddr):
    abstractSocket(bindFd, clientIpAddr)
  {
  }

  udpTransport::udpTransport(udpTransport &&udpSocketIns)
  {
    bindFd_ = udpSocketIns.bindFd_;
    udpSocketIns.bindFd_ = 0;
    clientIpAddr_ = udpSocketIns.clientIpAddr_;
    sourceIpStr_ = std::move(udpSocketIns.sourceIpStr_);
  }

  int udpTransport::initialize(const std::string_view ipAddr)
  {
    if (ipAddr == INVALID_IP)
    {
      LOG_WARN("ip address is INVALID");
      return PROCESS_SUCCESS;
    }
    if (bindSocket(ipAddr) == PROCESS_FAIL)
    {
      LOG_ERROR("error: udp initialize fail");
      return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
  }

  int udpTransport::bindSocket(const std::string_view ipAddr)
  {
    LOG_ERROR("info: now bind the this ip {}", ipAddr);
    // sourceIpStr_ = ipAddr;
    sourceIpStr_ = static_cast<std::string>(ipAddr);
    bindFd_ = socket(PF_INET, SOCK_DGRAM, 0);

    if (bindFd_ == -1)
    {
      LINUX_ERROR_PRINT();
      LOG_ERROR(" cant create udp fd ");
      return PROCESS_FAIL;
    }

    struct sockaddr_in bindIpAddr_in;

    if (translateStringToIpAddr(ipAddr, bindIpAddr_in) == PROCESS_FAIL)
    {
      return PROCESS_FAIL;
    }

    if (bind(bindFd_, (struct sockaddr *)&bindIpAddr_in, sizeof(struct sockaddr_in)) == -1)
    {
      LINUX_ERROR_PRINT();
      LOG_ERROR("error: cant bind the this ip {}", ipAddr);
      return PROCESS_FAIL;
    }

    return PROCESS_SUCCESS;
  }

  int udpTransport::send2Socket(char *data, uint32_t dataLen, std::string_view sendIpAddr)
  {
    struct sockaddr_in sendIpAddr_in;

    if (translateStringToIpAddr(sendIpAddr, sendIpAddr_in) == PROCESS_FAIL)
    {
      return PROCESS_FAIL;
    }

    if (sendto(bindFd_, data, dataLen, 0, (struct sockaddr *)&sendIpAddr_in, sizeof(sendIpAddr_in)) == -1)
    {
      LINUX_ERROR_PRINT();
      LOG_ERROR("cant send data");
      return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
  }

  int udpTransport::recvFromSocket(char *recvData, size_t dataBuffLen)
  {
    ssize_t recvDataLen = 0;
    socklen_t ipAddrLen = sizeof(clientIpAddr_);
    if ((recvDataLen = recvfrom(bindFd_, recvData, dataBuffLen, 0, (struct sockaddr *)&clientIpAddr_, &ipAddrLen)) == -1)
    {
      LINUX_ERROR_PRINT();
      LOG_ERROR("cant recv data");
      return INVALID_VALUE;
    }

    return recvDataLen;
  }

  udpTransport::~udpTransport()
  {
    if (bindFd_ == 0)
    {
      close(bindFd_);
    }
  }

  tcpTransport::tcpTransport()
  {
    bindFd_ = INVALID_VALUE;
  }

  tcpTransport::tcpTransport(int acceptFd, struct sockaddr_in clientIpAddr)
  {
    bindFd_ = acceptFd;
    clientIpAddr_ = clientIpAddr;
  }

  int tcpTransport::initialize(std::string_view sourceIpAddr)
  {
    if (bindSocket(sourceIpAddr) == PROCESS_FAIL)
    {
      LOG_ERROR("udp initialize fail");
      return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
  }

  int tcpTransport::bindSocket(std::string_view sourceIpAddr)
  {
    LOG_INFO("now bind the this ip {}", sourceIpAddr);
    sourceIpStr_ = sourceIpAddr;

    bindFd_ = socket(PF_INET, SOCK_STREAM, 0);

    if (bindFd_ == -1)
    {
      LINUX_ERROR_PRINT();
      LOG_ERROR("cant create udp fd");
      return PROCESS_FAIL;
    }

    struct sockaddr_in clientIpAddr_in;

    if (translateStringToIpAddr(sourceIpAddr, clientIpAddr_in) == PROCESS_FAIL)
    {
      return PROCESS_FAIL;
    }

    clientIpAddr_ = clientIpAddr_in;

    if (bind(bindFd_, (struct sockaddr *)&clientIpAddr_in, sizeof(struct sockaddr_in)) == -1)
    {
      LINUX_ERROR_PRINT();
      LOG_ERROR("cant bind the this ip {}", sourceIpAddr);
      return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
  }

  tcpAcceptor::tcpAcceptor(int givenBindFd): acceptFd_(INVALID_VALUE)
  {
    bindFd_ = givenBindFd;
  }

  tcpAcceptor::~tcpAcceptor()
  {
    if (acceptFd_ != INVALID_VALUE)
    {
      close(acceptFd_);
    }
  }

  int tcpAcceptor::accept2Socket()
  {
    if (bindFd_ != INVALID_VALUE)
    {
      socklen_t addrSize = sizeof(struct sockaddr);
      acceptFd_ = accept(bindFd_, (struct sockaddr *)&clientIpAddr_, &addrSize);

      if (acceptFd_ < 0)
      {
        LINUX_ERROR_PRINT();
        LOG_ERROR("TCP cant accept this connect");
        return PROCESS_FAIL;
      }
      clientIpString_ = inet_ntoa(clientIpAddr_.sin_addr);
      LOG_DEBUG("now accept {}", clientIpString_);
      // IO detach, in order to improve the big data transfer performance
      readFd_ = fdopen(acceptFd_, "r");
      writeFd_ = fdopen(acceptFd_, "a");
    }
    return PROCESS_SUCCESS;
  }

  int tcpAcceptor::send2Socket(char *data, uint32_t dataLen, std::string_view sendIpAddr)
  {
    if (acceptFd_ > 0)
    {
      uint32_t sendLen = 0;
      int fputsReturnValue = 0;
      for (; dataLen < sendLen;)
      {
        if ((fputsReturnValue = fputs(data + sendLen, writeFd_)) != EOF)
        {
          sendLen += (uint32_t)fputsReturnValue;
          fflush(writeFd_);
        }
        else
        {
          LINUX_ERROR_PRINT();
          LOG_WARN("the tcp socket {} is disconnect", acceptFd_);
          break;
        }
      }
      return PROCESS_SUCCESS;
    }
    return PROCESS_SUCCESS;
  }

  int tcpAcceptor::recvFromSocket(char *recvData, size_t dataBuffLen)
  {
    if (acceptFd_ <= 0)
    {
      return PROCESS_FAIL;
    }
    // todo: change to use TLV method, must consider about image and other type file transport, expect of text file.
    if (fgets(recvData, dataBuffLen, readFd_) == nullptr)
    {
      LINUX_ERROR_PRINT();
      LOG_WARN("the tcp socket {} is disconnect", acceptFd_);
      return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
  }

  epollMultiIOManager::epollMultiIOManager(): epollFd_(INVALID_VALUE),
    maxSpyFdNum_(1)
  {
  }

  epollMultiIOManager::~epollMultiIOManager()
  {
    fdSocketMap_.clear();
  }

  int epollMultiIOManager::createMultiIOInstance()
  {
    if ((epollFd_ = epoll_create(maxSpyFdNum_)) == INVALID_VALUE)
    {
      LINUX_ERROR_PRINT();
      LOG_ERROR("cant create epoll fd");
      return PROCESS_FAIL;
    }

    return PROCESS_SUCCESS;
  }

  int epollMultiIOManager::initialize(int timeout, int maxFdNum)
  {
    if (maxFdNum > 0)
    {
      maxSpyFdNum_ = maxFdNum;
    }
    if (createMultiIOInstance() == PROCESS_FAIL)
    {
      LOG_ERROR("cant initizalie epoll instance");
      return PROCESS_FAIL;
    }

    timeout_ = timeout;

    return PROCESS_SUCCESS;
  }

  int epollMultiIOManager::addFd(int addFd, abstractSocket *socket, int eventType)
  {
    struct epoll_event epollEvent;
    epollEvent.events = eventType;
    epollEvent.data.fd = addFd;

    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, addFd, &epollEvent) == INVALID_VALUE)
    {
      LINUX_ERROR_PRINT();
      LOG_ERROR("cant add fd to epoll watch event");
      return PROCESS_FAIL;
    }
    if (insertFdSocketMap(addFd, socket) == INVALID_VALUE)
    {
      LOG_WARN("don't the same socket twice");
    }

    return PROCESS_SUCCESS;
  };

  int epollMultiIOManager::addListenSocket(std::unique_ptr<abstractSocket> &uniq_listenSocket)
  {
    uniq_listenSocket_ = std::move(uniq_listenSocket);
    addFd(uniq_listenSocket_->getBindFd(), uniq_listenSocket_.get());
    return PROCESS_SUCCESS;
  }

  int epollMultiIOManager::insertFdSocketMap(int fd, abstractSocket *socket)
  {
    auto iterator = fdSocketMap_.find(fd);
    if (iterator == fdSocketMap_.end())
    {
      fdSocketMap_.emplace(fd, socket);
      return PROCESS_SUCCESS;
    }

    return PROCESS_FAIL;
  }

  int epollMultiIOManager::removeFdSocketMap(int fd)
  {
    auto iterator = fdSocketMap_.find(fd);
    if (iterator != fdSocketMap_.end())
    {
      fdSocketMap_.erase(fd);
      return PROCESS_SUCCESS;
    }

    LOG_WARN("fd {} don't exist", fd);
    return PROCESS_FAIL;
  }

  int epollMultiIOManager::removeFd(int deleteFd)
  {
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, deleteFd, NULL) == INVALID_VALUE)
    {
      LINUX_ERROR_PRINT();
      LOG_ERROR("cant remove fd {} from epoll set", deleteFd);
      return PROCESS_FAIL;
    }

    removeFdSocketMap(deleteFd);
    return PROCESS_SUCCESS;
  }

  int epollMultiIOManager::setEventProcessor(eventProcessSubject *)
  {
    return PROCESS_SUCCESS;
  }

  auto epollMultiIOManager::waitMessage()
  {
    struct epoll_event *triggerEvent = nullptr;
    int triggerEventNum = epoll_wait(epollFd_, triggerEvent, maxSpyFdNum_, timeout_);
    for (int i = 0; i < triggerEventNum; ++i)
    {
      handleEpollEvent(triggerEvent[i]);
    }
  }

  inline int epollMultiIOManager::handleEpollEvent(struct epoll_event &event)
  {
    int acceptFd = 0;
    if (event.data.fd == uniq_listenSocket_->getBindFd())
    {
      // acceptFd = accept(uniq_listenSocket_->getBindFd(),);
    }
    return 0;
  }

};
