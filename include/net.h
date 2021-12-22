#ifndef _NET_H_
#define _NET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "type.h"
#include <map>
#include <sys/epoll.h>

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
class constractMultiIOManager;
class constractEventSubject;
class eventProcessSubject;

class constractSocket
{
public:
    virtual ~constractSocket()=default;
    virtual bool initialize(const std::string &ipAddr) = 0;
    virtual int bindSocket(const std::string &ipAddr) = 0;
    virtual int sent2Socket(char *data, uint32_t dataLen, const std::string &sendIpAddr) = 0;
    virtual int recvFromSocket(char *recvData, size_t dataBuffLen) = 0;
    std::string& getSouceIpAddr();
    int translateStringToIpAddr(const std::string& ipString, struct sockaddr_in *ipAddr_in);
protected:
    int bindFd;
    std::string sourceIpString;
    struct sockaddr_in bindIpAddr;
    struct sockaddr_in recvIpAddr;
};

class udpSocket: public constractSocket
{
public:
    udpSocket();
    virtual ~udpSocket();
    virtual bool initialize(const std::string &ipAddr);
    virtual int bindSocket(const std::string &ipAddr);
    virtual int sent2Socket(char *data, uint32_t dataLen, const std::string &sendIpAddr);
    virtual int recvFromSocket(char *recvData, size_t dataBuffLen);
};

class constractMultiIOManager
{
public:
    virtual ~constractMultiIOManager()=default;
    virtual int initialize() = 0;
    virtual int createMultiIOInstance() = 0;
    virtual int addFd(int fd, constractSocket *socket) = 0;
    virtual int removeFd(int fd) = 0;
    virtual int waitMessage() = 0;
    virtual int setEventCallback() = 0;
};

class epollMultiIOManager: public constractMultiIOManager
{
public:
    virtual ~epollMultiIOManager();
    epollMultiIOManager();
    virtual int initialize(int timeout);
    virtual int createMultiIOInstance();
    virtual int addFd(int fd,constractSocket *socket, int eventType = EPOLLIN);
    virtual int removeFd(int fd);
    virtual int waitMessage();
    virtual int setEventProcessor(eventProcessSubject*);
    int insertFdSocketMap(int fd, constractSocket *socket);
    int removeFdSocketMap(int fd);
protected:
    std::map<int, constractSocket*> fdSocketMap_;
    int epollFd;
    int timeout;
    eventProcessSubject *eventProcessor;
};


};

#endif