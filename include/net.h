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

namespace net
{

enum socketType_t
{
    TCP_TYPE,
    UDP_TYPE
};


class epollMultiIOManager;
class constractSocket;
class udpSocket;
class tcpSocket;
class tcpAcceptor;
class constractMultiIOManager;
class constractEventSubject;
class eventProcessSubject;

class constractSocket
{
public:
    virtual ~constractSocket() = default;
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
    int bindFd;
    std::string sourceIpString;
    struct sockaddr_in recvIpAddr;
};

class udpSocket: public constractSocket
{
public:
    udpSocket();
    virtual ~udpSocket();
    virtual int initialize(const std::string &ipAddr);
    virtual int bindSocket(const std::string &ipAddr);
    virtual int sent2Socket(char *data, uint32_t dataLen, const std::string &sendIpAddr);
    virtual int recvFromSocket(char *recvData, size_t dataBuffLen);
protected:
    struct sockaddr_in bindIpAddr;
};

class tcpSocket: public constractSocket
{
public:
    tcpSocket();
    virtual ~tcpSocket() = default;
    virtual int initialize(const std::string &ipAddr);
    virtual int bindSocket(const std::string &ipAddr);
protected:
    struct sockaddr_in bindIpAddr;
};

class tcpAcceptor: public constractSocket
{
public:
    tcpAcceptor() = default;
    tcpAcceptor(int givenBindFd);
    virtual ~tcpAcceptor();
    virtual int accept2Socket();
    virtual int sent2Socket(char *data, uint32_t dataLen, const std::string &sendIpAddr = 0);
    virtual int recvFromSocket(char *recvData, size_t dataBuffLen);
protected:
    int acceptFd;
    std::string clientIpString;
    FILE *readFd;
    FILE *writeFd;
};


class constractMultiIOManager
{
public:
    virtual ~constractMultiIOManager()=default;
    virtual int initialize(int timeout, int maxFdNum) = 0;
    virtual int createMultiIOInstance() = 0;
    virtual int addFd(int fd, constractSocket *socket) = 0;
    virtual int removeFd(int fd) = 0;
    virtual callback_t waitMessage() = 0;
    virtual int setEventCallback() = 0;
};

class epollMultiIOManager: public constractMultiIOManager
{
public:
    virtual ~epollMultiIOManager();
    epollMultiIOManager();
    virtual int initialize(int timeout = 0, int maxFdNum = 1);
    virtual int createMultiIOInstance();
    virtual int addFd(int fd,constractSocket *socket, int eventType = EPOLLIN);
    virtual int removeFd(int fd);
    virtual callback_t waitMessage();
    virtual int setEventProcessor(eventProcessSubject*);
    int insertFdSocketMap(int fd, constractSocket *socket);
    int removeFdSocketMap(int fd);
protected:
    std::map<int, constractSocket*> fdSocketMap_;
    int epollFd;
    int timeout;
    int maxSpyFdNum;
    eventProcessSubject *eventProcessor;
    threadPool          netThreadPool;
};


};

#endif