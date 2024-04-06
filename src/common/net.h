#ifndef _NET_H_
#define _NET_H_

#include <arpa/inet.h>
#include <memory.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <map>
#include <string_view>

#include "threadPool.h"
#include "type.h"

namespace dawn
{
    constexpr size_t UDP_MTU = 1500;
    enum class SOCK_TYPE_E
    {
        TCP_TYPE,
        UDP_TYPE
    };

    class epollMultiIOManager;
    class abstractSocket;
    class udpTransport;
    class tcpTransport;
    class tcpAcceptor;
    class abstractMultiIOManager;
    class abstractEventSubject;
    class eventProcessSubject;

    class abstractSocket
    {
    public:
        abstractSocket() : bindFd_(INVALID_VALUE) { memset(&clientIpAddr_, 0x0, sizeof(clientIpAddr_)); };
        abstractSocket(int bindFd, struct sockaddr_in clientIpAddr);
        virtual ~abstractSocket() = default;
        virtual int initialize(std::string_view ipAddr) { return PROCESS_FAIL; }
        virtual int bindSocket(std::string_view ipAddr) { return PROCESS_FAIL; }
        virtual int send2Socket(char *data, uint32_t dataLen, std::string_view sendIpAddr) { return PROCESS_FAIL; }
        virtual int send2Socket(char *data, uint32_t dataLen, struct sockaddr_in clientIpAddr);
        virtual int recvFromSocket(char *recvData, size_t dataBuffLen);
        virtual int recvFromSocket(char *recvData, size_t dataBuffLen, struct sockaddr_in &clientIpAddr);
        virtual int recvFromSpecificSocket(int fd, char *recvData, size_t dataBuffLen);
        virtual int recvFromSpecificSocket(int fd, char *recvData, size_t dataBuffLen, struct sockaddr_in &clientIpAddr);
        virtual int connect2Socket() { return PROCESS_FAIL; }
        virtual int accept2Socket() { return PROCESS_FAIL; }
        virtual int close2Socket() { return PROCESS_FAIL; }
        std::string_view getSourceIpAddr();
        struct sockaddr_in getClientIpAddr();
        int getBindFd();

        /// @brief translate ipv4 address to ip socket structure
        /// @param ipString e.g. 192.168.0.1:2152
        /// @param ipAddr_in
        /// @return PROCESS_SUCCESS or PROCESS_FAIL
        int translateStringToIpAddr(const std::string_view &ipString, struct sockaddr_in &ipAddr_in);

    protected:
        struct sockaddr_in clientIpAddr_;
        int bindFd_ = 0;
        std::string sourceIpStr_;
    };

    class udpTransport : public abstractSocket
    {
    public:
        udpTransport() = default;
        udpTransport(const udpTransport &) = delete;
        udpTransport operator=(const udpTransport &) = delete;

        /// @brief create a end point to another end point transport
        /// @param bindFd
        /// @param clientIpAddr
        udpTransport(int bindFd, struct sockaddr_in clientIpAddr);

        udpTransport(udpTransport &&udpSocketIns);

        virtual ~udpTransport();
        /**
         * @brief bind a ipv4 address
         * @param ipAddr e.g. 192.168.0.1:2152
         * @return PROCESS_SUCCESS or PROCESS_FAIL
         */
        virtual int initialize(std::string_view ipAddr) override;

        /**
         * @brief bind a ipv4 address
         * @param ipAddr e.g. 192.168.0.1:2152
         * @return PROCESS_SUCCESS or PROCESS_FAIL
         */
        virtual int bindSocket(std::string_view ipAddr) override;
        using abstractSocket::send2Socket;
        virtual int send2Socket(char *data, uint32_t dataLen, std::string_view sendIpAddr) override;
        using abstractSocket::recvFromSocket;
        virtual int recvFromSocket(char *recvData, size_t dataBuffLen) override;
        using abstractSocket::recvFromSpecificSocket;
    };

    class tcpTransport : public abstractSocket
    {
    public:
        tcpTransport();
        tcpTransport(int acceptFd, struct sockaddr_in clientIpAddr);
        virtual ~tcpTransport() = default;
        virtual int initialize(std::string_view sourceIpAddr) override;
        virtual int bindSocket(std::string_view sourceIpAddr) override;
    };

    class tcpAcceptor : public abstractSocket
    {
    public:
        tcpAcceptor() = default;
        tcpAcceptor(int givenBindFd);
        virtual ~tcpAcceptor();
        virtual int accept2Socket() override;
        virtual int send2Socket(char *data, uint32_t dataLen, std::string_view sendIpAddr) override;
        virtual int recvFromSocket(char *recvData, size_t dataBuffLen) override;

    protected:
        int acceptFd_;
        std::string clientIpString_;
        FILE *readFd_;
        FILE *writeFd_;
    };

    class abstractMultiIOManager
    {
    public:
        virtual ~abstractMultiIOManager() = default;
        virtual int initialize(int timeout, int maxFdNum) = 0;
        virtual int createMultiIOInstance() = 0;
        virtual int addListenSocket(std::unique_ptr<abstractSocket> uniq_listenSocket) = 0;
        virtual int addFd(int fd, abstractSocket *socket) = 0;
        virtual int removeFd(int fd) = 0;
        virtual int setEventCallback() = 0;
    };

    class epollMultiIOManager : public abstractMultiIOManager
    {
    public:
        virtual ~epollMultiIOManager();
        epollMultiIOManager();
        virtual int initialize(int timeout = 0, int maxFdNum = 1);
        virtual int createMultiIOInstance();
        virtual int addFd(int fd, abstractSocket *socket, int eventType = EPOLLIN);
        virtual int addListenSocket(std::unique_ptr<abstractSocket> &uniq_listenSocket);
        virtual int removeFd(int fd);
        virtual int setEventProcessor(eventProcessSubject *);
        auto waitMessage();
        int insertFdSocketMap(int fd, abstractSocket *socket);
        int removeFdSocketMap(int fd);
        int handleEpollEvent(struct epoll_event &event);

    protected:
        std::map<int, abstractSocket *> fdSocketMap_;
        std::unique_ptr<abstractSocket> uniq_listenSocket_;
        int epollFd_;
        int timeout_;
        int maxSpyFdNum_;
        eventProcessSubject *eventProcessor_;
        threadPool netThreadPool_;
    };
};

#endif
