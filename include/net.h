#ifndef _NET_H_
#define _NET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "type.h"
#include <map>
#include <sys/epoll.h>
#include "threadPool.h"
#include <memory.h>

namespace dawn
{

enum socketType_t
{
    TCP_TYPE,
    UDP_TYPE
};


class epollMultiIOManager;
class abstractSocket;
class udpSocket;
class tcpSocket;
class tcpAcceptor;
class abstractMultiIOManager;
class abstractEventSubject;
class eventProcessSubject;

class abstractSocket
{
public:
    virtual ~abstractSocket() = default;
    virtual int initialize(const std::string &ipAddr){return false;}
    virtual int bindSocket(const std::string &ipAddr){return false;}
    virtual int sent2Socket(char *data, uint32_t dataLen, const std::string &sendIpAddr){return false;}
    virtual int recvFromSocket(char *recvData, size_t dataBuffLen){return false;}
    virtual int connect2Socket(){return false;}
    virtual int accept2Socket(){return false;}
    virtual int close2Socket(){return false;}
    std::string& getSouceIpAddr();
    int getBindFd();
    int translateStringToIpAddr(const std::string& ipString, struct sockaddr_in *ipAddr_in);
protected:
    int bindFd_;
    std::string sourceIpStr_;
    struct sockaddr_in clientIpAddr_;
};

class udpSocket: public abstractSocket
{
public:
    udpSocket();
    virtual ~udpSocket();
    virtual int initialize(const std::string &ipAddr);
    virtual int bindSocket(const std::string &ipAddr);
    virtual int sent2Socket(char *data, uint32_t dataLen, const std::string &sendIpAddr);
    virtual int recvFromSocket(char *recvData, size_t dataBuffLen);
};

class tcpSocket: public abstractSocket
{
public:
    tcpSocket();
    tcpSocket(int acceptFd, struct sockaddr_in clientIpAddr);
    virtual ~tcpSocket() = default;
    virtual int initialize(const std::string &sourceIpAddr);
    virtual int bindSocket(const std::string &sourceIpAddr);
};

class tcpAcceptor: public abstractSocket
{
public:
    tcpAcceptor() = default;
    tcpAcceptor(int givenBindFd);
    virtual ~tcpAcceptor();
    virtual int accept2Socket();
    virtual int sent2Socket(char *data, uint32_t dataLen, const std::string &sendIpAddr = 0);
    virtual int recvFromSocket(char *recvData, size_t dataBuffLen);
protected:
    int acceptFd_;
    std::string clientIpString_;
    FILE *readFd_;
    FILE *writeFd_;
};


class abstractMultiIOManager
{
public:
    virtual ~abstractMultiIOManager()=default;
    virtual int initialize(int timeout, int maxFdNum) = 0;
    virtual int createMultiIOInstance() = 0;
    virtual int addListenSocket(std::unique_ptr<abstractSocket> uniq_listenSocket) = 0;
    virtual int addFd(int fd, abstractSocket *socket) = 0;
    virtual int removeFd(int fd) = 0;
    virtual int setEventCallback() = 0;
};

class epollMultiIOManager: public abstractMultiIOManager
{
public:
    virtual ~epollMultiIOManager();
    epollMultiIOManager();
    virtual int initialize(int timeout = 0, int maxFdNum = 1);
    virtual int createMultiIOInstance();
    virtual int addFd(int fd,abstractSocket *socket, int eventType = EPOLLIN);
    virtual int addListenSocket(std::unique_ptr<abstractSocket>& uniq_listenSocket);
    virtual int removeFd(int fd);
    virtual int setEventProcessor(eventProcessSubject*);
    auto waitMessage();
    int insertFdSocketMap(int fd, abstractSocket *socket);
    int removeFdSocketMap(int fd);
    int handleEpollEvent(struct epoll_event& event);
protected:
    std::map<int, abstractSocket*> fdSocketMap_;
    std::unique_ptr<abstractSocket> uniq_listenSocket_;
    int epollFd_;
    int timeout_;
    int maxSpyFdNum_;
    eventProcessSubject *eventProcessor_;
    threadPool          netThreadPool_;
};


};

#endif