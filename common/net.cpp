#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
 #include <unistd.h>
#include "net.h"
#include <errno.h>
#include <string.h>
#include <memory.h>
#include "setLogger.h"

namespace net
{
std::string& constractSocket::getSouceIpAddr()
{
    return sourceIpString;
}

int constractSocket::getBindFd()
{
    return bindFd;
}

int constractSocket::translateStringToIpAddr(const std::string& ipString, struct sockaddr_in *ipAddr_in)
{
    uint32_t portPos = 0;

    if( (portPos = ipString.find(':')) == ipString.npos)
    {
        std::cout<<"error: cant traslate this ip, string dont containt a port "<< ipString <<std::endl;
        return PROCESS_FAIL;
    }

    struct in_addr trasnlateIp;
    unsigned short translatePort;
    std::string ipAddrNoPort = ipString.substr(0, portPos);
    std::string ipAddrPort = ipString.substr(portPos + 1, ipString.size());

    if(inet_pton(AF_INET, ipAddrNoPort.c_str(), &trasnlateIp) == 0)
    {
        std::cout<<"error: cant trasnlate this ip "<< ipString <<std::endl;
        return PROCESS_FAIL;
    }

    translatePort = htons(atoi(ipAddrPort.c_str()));

    memset(ipAddr_in, 0x0, sizeof(*ipAddr_in));
    ipAddr_in->sin_family = AF_INET;
    ipAddr_in->sin_addr.s_addr = trasnlateIp.s_addr;
    ipAddr_in->sin_port = translatePort;

    return PROCESS_SUCCESS;
}

udpSocket::udpSocket()
{
    bindFd = INVALID_VALUE;
    sourceIpString.clear();
}

int udpSocket::initialize(const std::string &ipAddr)
{
    if(bindSocket(ipAddr) == PROCESS_FAIL)
    {
        std::cout<<"error: udp initialize fail"<<std::endl;
        return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
}

/*
*input: e.g: 192.168.1.1:2152
*/
int udpSocket::bindSocket(const std::string &ipAddr)
{
    std::cout<< "info: now bind the this ip " << ipAddr <<std::endl;
    sourceIpString = ipAddr;

    bindFd = socket(PF_INET, SOCK_DGRAM, 0);

    if(bindFd == -1)
    {
        LINUX_ERROR_PRINT();
        std::cout<<"error: cant create udp fd "<<std::endl;
        return PROCESS_FAIL;
    }

    struct sockaddr_in bindIpAddr_in;

    if(translateStringToIpAddr(ipAddr, &bindIpAddr_in) == PROCESS_FAIL)
    {
        return PROCESS_FAIL;
    }

    bindIpAddr = bindIpAddr_in;

    if(bind(bindFd, (struct sockaddr*)&bindIpAddr_in, sizeof(struct sockaddr_in)) == -1)
    {
        LINUX_ERROR_PRINT();
        std::cout<< "error: cant bind the this ip " << ipAddr <<std::endl;
        return PROCESS_FAIL;
    }
    
    return PROCESS_SUCCESS;
}

int udpSocket::sent2Socket(char *data, uint32_t dataLen, const std::string &sendIpAddr)
{
    struct sockaddr_in sendIpAddr_in;

    if(translateStringToIpAddr(sendIpAddr, &sendIpAddr_in) == PROCESS_FAIL)
    {
        return PROCESS_FAIL;
    }

    if(sendto(bindFd, data, dataLen, 0, (struct sockaddr*)&sendIpAddr_in, sizeof(sendIpAddr_in)) == -1)
    {
        LINUX_ERROR_PRINT();
        std::cout<<"error: cant send data "<<std::endl;
    }
    return PROCESS_SUCCESS;
}

int udpSocket::recvFromSocket(char *recvData, size_t dataBuffLen)
{
    ssize_t recvDataLen = 0;
    socklen_t ipAddrLen = sizeof(recvIpAddr);
    if((recvDataLen = recvfrom(bindFd, recvData, dataBuffLen, 0, (struct sockaddr*)&recvIpAddr, &ipAddrLen)) == -1)
    {
        LINUX_ERROR_PRINT();
        std::cout <<"error: cant recv data"<<std::endl;
        return INVALID_VALUE;
    }

    return recvDataLen;
}

udpSocket::~udpSocket()
{
    close(bindFd);
    sourceIpString.clear();
}

tcpSocket::tcpSocket()
{
    bindFd = INVALID_VALUE;
    sourceIpString.clear();
}

int tcpSocket::initialize(const std::string &ipAddr)
{
    if(bindSocket(ipAddr) == PROCESS_FAIL)
    {
        std::cout<<"error: udp initialize fail"<<std::endl;
        return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
}

/*
*input: e.g: 192.168.1.1:2152
*/
int tcpSocket::bindSocket(const std::string &ipAddr)
{
    std::cout<< "info: now bind the this ip " << ipAddr <<std::endl;
    sourceIpString = ipAddr;

    bindFd = socket(PF_INET, SOCK_STREAM, 0);

    if(bindFd == -1)
    {
        LINUX_ERROR_PRINT();
        std::cout<<"error: cant create udp fd "<<std::endl;
        return PROCESS_FAIL;
    }

    struct sockaddr_in bindIpAddr_in;

    if(translateStringToIpAddr(ipAddr, &bindIpAddr_in) == PROCESS_FAIL)
    {
        return PROCESS_FAIL;
    }

    bindIpAddr = bindIpAddr_in;

    if(bind(bindFd, (struct sockaddr*)&bindIpAddr_in, sizeof(struct sockaddr_in)) == -1)
    {
        LINUX_ERROR_PRINT();
        std::cout<< "error: cant bind the this ip " << ipAddr <<std::endl;
        return PROCESS_FAIL;
    }
    return PROCESS_SUCCESS;
}

tcpAcceptor::tcpAcceptor(int givenBindFd):
acceptFd(INVALID_VALUE)
{
    bindFd = givenBindFd;
}


tcpAcceptor::~tcpAcceptor()
{
    if(acceptFd != INVALID_VALUE)
    {
        close(acceptFd);
    }
}

int tcpAcceptor::accept2Socket()
{
    if(bindFd != INVALID_VALUE)
    {
        socklen_t addrSize =  sizeof(struct sockaddr);
        acceptFd = accept(bindFd, (struct sockaddr*)&recvIpAddr, &addrSize);

        if(acceptFd < 0)
        {
            LINUX_ERROR_PRINT();
            std::cout<<"error: TCP cant accept this connect"<<std::endl;
            return PROCESS_FAIL;
        }
        clientIpString = inet_ntoa(recvIpAddr.sin_addr);
        std::cout<<"debug: now accept "<<clientIpString<<std::endl;
        //IO detach, in order to improve the big data transfer performance
        readFd = fdopen(acceptFd, "r");
        writeFd = fdopen(acceptFd, "a");
    }
    return PROCESS_SUCCESS;
}

int tcpAcceptor::sent2Socket(char *data, uint32_t dataLen, const std::string &sendIpAddr)
{
    if(acceptFd > 0)
    {
        uint32_t sendLen = 0;
        int fputsReturnValue = 0;
        for(;dataLen < sendLen;)
        {
            if((fputsReturnValue = fputs(data + sendLen, writeFd)) != EOF)
            {
                sendLen += (uint32_t)fputsReturnValue;
                fflush(writeFd);
            }
            else
            {
                LINUX_ERROR_PRINT();
                std::cout<<"debug: the tcp socket is disconnect"<<std::endl;
                break;
            }
        }
        return PROCESS_SUCCESS;
    }
    return PROCESS_SUCCESS;
}

int tcpAcceptor::recvFromSocket(char *recvData, size_t dataBuffLen)
{
    if(acceptFd > 0)
    {
        //todo: change to use TLV method, must consider about image and other type file transport, expect of text file.
        fgets(recvData, dataBuffLen, readFd);

        return PROCESS_SUCCESS;
    }
    return PROCESS_SUCCESS;
}

epollMultiIOManager::epollMultiIOManager():
epollFd(INVALID_VALUE),
maxSpyFdNum(1)
{

}

epollMultiIOManager::~epollMultiIOManager()
{
    fdSocketMap_.clear();
}

int epollMultiIOManager::createMultiIOInstance()
{
    if((epollFd = epoll_create(maxSpyFdNum)) == INVALID_VALUE)
    {
        LINUX_ERROR_PRINT();
        std::cout<< "error: cant create epoll fd"<<std::endl;
        return PROCESS_FAIL;
    }

    timeout = timeout;

    return PROCESS_SUCCESS;
}

int epollMultiIOManager::initialize(int timeout, int maxFdNum)
{
    if(maxFdNum > 0)
    {
        maxSpyFdNum = maxFdNum;
    }
    if(createMultiIOInstance() == PROCESS_FAIL)
    {
        std::cout<< "error: cant initizalie epoll instance" <<std::endl;
        return PROCESS_FAIL;
    }

    timeout = timeout;

    return PROCESS_SUCCESS;
}

int epollMultiIOManager::addFd(int addFd, constractSocket *socket, int eventType)
{
    struct epoll_event epollEvent;
    epollEvent.events = eventType;
    epollEvent.data.fd = addFd;

    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, addFd, &epollEvent) == INVALID_VALUE)
    {
        LINUX_ERROR_PRINT();
        std::cout<<"error: cant add fd to epoll watch event "<<std::endl;
        return PROCESS_FAIL;
    }
    if(insertFdSocketMap(addFd, socket) == INVALID_VALUE)
    {
        std:: cout << "warn: dont the same socket twice"<<std::endl;
    }

    return PROCESS_SUCCESS;
};


int epollMultiIOManager::insertFdSocketMap(int fd, constractSocket *socket)
{
    auto iterator = fdSocketMap_.find(fd);
    if(iterator == fdSocketMap_.end())
    {
        fdSocketMap_.insert(std::make_pair(fd, socket));
        return PROCESS_SUCCESS;
    }

    return PROCESS_FAIL;

}

int epollMultiIOManager::removeFdSocketMap(int fd)
{
    auto iterator = fdSocketMap_.find(fd);
    if(iterator != fdSocketMap_.end())
    {
        fdSocketMap_.erase(fd);
        return PROCESS_SUCCESS;
    }

    std::cout<< "warn: fd dont exist"<<std::endl;
    return PROCESS_FAIL;
}

int epollMultiIOManager::removeFd(int deleteFd)
{
    if(epoll_ctl(epollFd, EPOLL_CTL_DEL, deleteFd, NULL) == INVALID_VALUE)
    {
        LINUX_ERROR_PRINT();
        std::cout<<"error: cant remove fd from epoll set"<<std::endl;
        return PROCESS_FAIL;
    } 

    removeFdSocketMap(deleteFd);
    return PROCESS_SUCCESS;
}

int epollMultiIOManager::setEventProcessor(eventProcessSubject*)
{
    return PROCESS_SUCCESS;
}

auto epollMultiIOManager::waitMessage()
{
    int triggerEventNum = 0;
    struct epoll_event *triggerEvent = nullptr;
    triggerEventNum = epoll_wait(epollFd, triggerEvent, maxSpyFdNum, timeout);
    
    // return PROCESS_SUCCESS;
}



};
