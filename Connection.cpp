//==============================================================================
// ConcurrencyNetworkFramework : Connection Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include "Listener.h"
#include "Log.h"
#include "Socket.h"
#include "Connection.h"

#define UDP_MAX_SIZE    1280

#define CONNECT_LOG(proto, level, format, ...) \
    Log::Format(level, "%s %d (%s:%s:%s@%s:%s) : " format, "Connect", thiz.socket ## proto, #proto, thiz.sourceAddress, thiz.sourcePort, thiz.destinationAddress, thiz.destinationPort, __VA_ARGS__)

std::atomic_int Connection::activeThreadCount[4];
//------------------------------------------------------------------------------
Connection::Connection(int socket, const char* address, const char* port, const struct sockaddr_storage& addr)
{
    thiz.terminate = false;

    thiz.socketTCP = socket;
    thiz.socketUDP = 0;
    thiz.readyTCP = true;
    thiz.readyUDP = false;
    thiz.sourceAddress = address ? ::strdup(address) : nullptr;
    thiz.sourcePort = port ? ::strdup(port) : nullptr;
    thiz.destinationAddress = nullptr;
    thiz.destinationPort = nullptr;

    GetAddressPort(addr, thiz.destinationAddress, thiz.destinationPort);
}
//------------------------------------------------------------------------------
Connection::Connection(const char* address, const char* port)
{
    thiz.terminate = false;

    thiz.socketTCP = 0;
    thiz.socketUDP = 0;
    thiz.readyTCP = false;
    thiz.readyUDP = false;
    thiz.sourceAddress = nullptr;
    thiz.sourcePort = nullptr;
    thiz.destinationAddress = address ? ::strdup(address) : nullptr;
    thiz.destinationPort = port ? ::strdup(port) : nullptr;

    struct addrinfo* addrinfo = nullptr;
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int error = ::getaddrinfo(thiz.destinationAddress, thiz.destinationPort, &hints, &addrinfo);
    if (addrinfo == nullptr)
    {
        CONNECT_LOG(TCP, -1, "%s %s", "getaddrinfo", ::gai_strerror(error));
        return;
    }

    thiz.socketTCP = Socket::socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (thiz.socketTCP <= 0)
    {
        CONNECT_LOG(TCP, -1, "%s %s", "socket", Socket::strerror(Socket::errno));
        ::freeaddrinfo(addrinfo);
        return;
    }

    if (Socket::connect(thiz.socketTCP, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0)
    {
        CONNECT_LOG(TCP, -1, "%s %s", "connect", Socket::strerror(Socket::errno));
        Socket::close(thiz.socketTCP);
        thiz.socketTCP = 0;
        ::freeaddrinfo(addrinfo);
        return;
    }
    thiz.readyTCP = true;

    struct sockaddr_storage addr = {};
    socklen_t size = sizeof(addr);
    if (Socket::getsockname(thiz.socketTCP, (struct sockaddr*)&addr, &size) == 0)
    {
        GetAddressPort(addr, thiz.sourceAddress, thiz.sourcePort);
    }

    ::freeaddrinfo(addrinfo);
}
//------------------------------------------------------------------------------
Connection::~Connection()
{
    thiz.terminate = true;

    if (thiz.socketTCP > 0)
    {
        Socket::shutdown(thiz.socketTCP, SHUT_RD);
    }
    if (thiz.socketUDP > 0)
    {
        Socket::shutdown(thiz.socketUDP, SHUT_RD);
    }
    thiz.sendBufferSemaphoreTCP.release();
    thiz.sendBufferSemaphoreUDP.release();
    if (thiz.threadRecvTCP.joinable())
    {
        thiz.threadRecvTCP.join();
    }
    if (thiz.threadSendTCP.joinable())
    {
        thiz.threadSendTCP.join();
    }
    if (thiz.threadRecvUDP.joinable())
    {
        thiz.threadRecvUDP.join();
    }
    if (thiz.threadSendUDP.joinable())
    {
        thiz.threadSendUDP.join();
    }
    if (thiz.socketTCP > 0)
    {
        Socket::close(thiz.socketTCP);
        thiz.socketTCP = 0;
    }
    if (thiz.socketUDP > 0)
    {
        Socket::close(thiz.socketUDP);
        thiz.socketUDP = 0;
    }
    if (thiz.sourceAddress)
    {
        ::free(thiz.sourceAddress);
        thiz.sourceAddress = nullptr;
    }
    if (thiz.sourcePort)
    {
        ::free(thiz.sourcePort);
        thiz.sourcePort = nullptr;
    }
    if (thiz.destinationAddress)
    {
        ::free(thiz.destinationAddress);
        thiz.destinationAddress = nullptr;
    }
    if (thiz.destinationPort)
    {
        ::free(thiz.destinationPort);
        thiz.destinationPort = nullptr;
    }
}
//------------------------------------------------------------------------------
void Connection::ProcedureRecvTCP()
{
    Connection::activeThreadCount[0].fetch_add(1, std::memory_order_acq_rel);

    Buffer::element_type buffer;

    while (thiz.terminate == false)
    {
        // Length
        unsigned short size = 0;
        if (Socket::recv(thiz.socketTCP, &size, sizeof(short), MSG_WAITALL | MSG_NOSIGNAL) <= 0)
        {
            CONNECT_LOG(TCP, -1, "%s %s", "recv", Socket::strerror(Socket::errno));
            break;
        }

        // Preserve
        if (size == 0)
        {
            CONNECT_LOG(TCP, -1, "%s %s", "recv", "empty");
            continue;
        }

        // Data
        buffer.resize(size);
        if (Socket::recv(thiz.socketTCP, &buffer.front(), size, MSG_WAITALL | MSG_NOSIGNAL) <= 0)
        {
            CONNECT_LOG(TCP, -1, "%s %s", "recv", Socket::strerror(Socket::errno));
            break;
        }
        Recv(buffer);
    }

    thiz.readyTCP = false;
    thiz.terminate = true;

    Connection::activeThreadCount[0].fetch_sub(1, std::memory_order_acq_rel);
}
//------------------------------------------------------------------------------
void Connection::ProcedureSendTCP()
{
    Connection::activeThreadCount[1].fetch_add(1, std::memory_order_acq_rel);

    char cork[Socket::CORK_SIZE] = {};

    while (thiz.terminate == false)
    {
        Buffer sendBuffer;
        thiz.sendBufferMutexTCP.lock();
        if (thiz.sendBufferTCP.empty() == false)
        {
            thiz.sendBufferTCP.front().swap(sendBuffer);
            thiz.sendBufferTCP.erase(thiz.sendBufferTCP.begin());
        }
        thiz.sendBufferMutexTCP.unlock();
        if (sendBuffer)
        {
            const auto& buffer = (*sendBuffer);
            if (buffer.size() <= USHRT_MAX)
            {
                // Length
                unsigned short size = (short)buffer.size();
                if (Socket::send(thiz.socketTCP, &size, sizeof(short), MSG_WAITALL | MSG_NOSIGNAL | MSG_MORE, cork) <= 0)
                {
                    CONNECT_LOG(TCP, -1, "%s %s", "send", Socket::strerror(Socket::errno));
                    break;
                }

                // Preserve
                if (size == 0)
                {
                    CONNECT_LOG(TCP, -1, "%s %s", "send", "empty");
                    continue;
                }

                // Data
                if (Socket::send(thiz.socketTCP, &buffer.front(), size, MSG_WAITALL | MSG_NOSIGNAL, cork) <= 0)
                {
                    CONNECT_LOG(TCP, -1, "%s %s", "send", Socket::strerror(Socket::errno));
                    break;
                }
            }
            else
            {
                CONNECT_LOG(TCP, -1, "%s %s %zd", "send", "buffer too long", buffer.size());
            }
        }
        if (thiz.sendBufferTCP.empty() == false)
            continue;

        thiz.sendBufferSemaphoreTCP.acquire();
    }

    thiz.readyTCP = false;
    thiz.terminate = true;

    Connection::activeThreadCount[1].fetch_sub(1, std::memory_order_acq_rel);
}
//------------------------------------------------------------------------------
void Connection::ProcedureRecvUDP()
{
    Connection::activeThreadCount[2].fetch_add(1, std::memory_order_acq_rel);

    Buffer::element_type buffer;

    while (thiz.terminate == false)
    {
        buffer.resize(UDP_MAX_SIZE);

        // Data
        long size = Socket::recv(thiz.socketUDP, &buffer.front(), buffer.size(), MSG_WAITALL | MSG_NOSIGNAL);
        if (size <= 0)
        {
            CONNECT_LOG(UDP, -1, "%s %s", "recv", Socket::strerror(Socket::errno));
            break;
        }
        buffer.resize(size);
        Recv(buffer);
    }

    thiz.readyUDP = false;

    Connection::activeThreadCount[2].fetch_sub(1, std::memory_order_acq_rel);
}
//------------------------------------------------------------------------------
void Connection::ProcedureSendUDP()
{
    Connection::activeThreadCount[3].fetch_add(1, std::memory_order_acq_rel);

    char cork[Socket::CORK_SIZE] = {};

    while (thiz.terminate == false)
    {
        Buffer sendBuffer;
        thiz.sendBufferMutexUDP.lock();
        if (thiz.sendBufferUDP.empty() == false)
        {
            thiz.sendBufferUDP.front().swap(sendBuffer);
            thiz.sendBufferUDP.erase(thiz.sendBufferUDP.begin());
        }
        thiz.sendBufferMutexUDP.unlock();
        if (sendBuffer)
        {
            const auto& buffer = (*sendBuffer);
            if (buffer.size() <= UDP_MAX_SIZE)
            {
                // Data
                if (Socket::send(thiz.socketUDP, &buffer.front(), buffer.size(), MSG_WAITALL | MSG_NOSIGNAL, cork) <= 0)
                {
                    CONNECT_LOG(UDP, -1, "%s %s", "send", Socket::strerror(Socket::errno));
                    break;
                }
            }
            else
            {
                CONNECT_LOG(UDP, -1, "%s %s %zd", "send", "buffer too long", buffer.size());
            }
        }
        if (thiz.sendBufferUDP.empty() == false)
            continue;

        thiz.sendBufferSemaphoreUDP.acquire();
    }

    thiz.readyUDP = false;

    Connection::activeThreadCount[3].fetch_sub(1, std::memory_order_acq_rel);
}
//------------------------------------------------------------------------------
bool Connection::ConnectTCP()
{
    if (thiz.socketTCP <= 0)
        return false;
    if (thiz.threadRecvTCP.joinable() || thiz.threadSendTCP.joinable())
        return true;

    int enable = 1;
    Socket::setsockopt(thiz.socketTCP, SOL_SOCKET, SO_KEEPALIVE, (void*)&enable, sizeof(enable));
    Socket::setsockopt(thiz.socketTCP, SOL_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));

    std::stacking_thread(65536, [this]{ thiz.ProcedureRecvTCP(); }).swap(thiz.threadRecvTCP);
    std::stacking_thread(65536, [this]{ thiz.ProcedureSendTCP(); }).swap(thiz.threadSendTCP);
    if (thiz.threadRecvTCP.joinable() == false || thiz.threadSendTCP.joinable() == false)
    {
        CONNECT_LOG(TCP, -1, "%s %s", "thread", ::strerror(errno));
        return false;
    }

    CONNECT_LOG(TCP, 0, "%s %s", "connect", "ready");
    return true;
}
//------------------------------------------------------------------------------
bool Connection::ConnectUDP()
{
    if (thiz.socketTCP <= 0)
        return false;
    if (thiz.sourceAddress == nullptr || thiz.sourcePort == nullptr || thiz.destinationAddress == nullptr || thiz.destinationPort == nullptr)
        return false;
    if (thiz.threadRecvUDP.joinable() || thiz.threadSendUDP.joinable())
        return true;

    struct sockaddr_storage sockaddrSource;
    struct sockaddr_storage sockaddrDestination;
    int sockaddSourceLength = SetAddressPort(sockaddrSource, thiz.sourceAddress, thiz.sourcePort);
    int sockaddrDestinationLength = SetAddressPort(sockaddrDestination, thiz.destinationAddress, thiz.destinationPort);

    thiz.socketUDP = Socket::socket(sockaddrSource.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (thiz.socketUDP <= 0)
    {
        CONNECT_LOG(UDP, -1, "%s %s", "socket", Socket::strerror(Socket::errno));
        return false;
    }

    int enable = 1;
    Socket::setsockopt(thiz.socketUDP, SOL_SOCKET, SO_REUSEADDR, (void*)&enable, sizeof(enable));
    Socket::setsockopt(thiz.socketUDP, SOL_SOCKET, SO_REUSEPORT, (void*)&enable, sizeof(enable));

    if (Socket::bind(thiz.socketUDP, (struct sockaddr*)&sockaddrSource, sockaddSourceLength) < 0)
    {
        CONNECT_LOG(UDP, -1, "%s %s", "bind", Socket::strerror(Socket::errno));
        Socket::close(thiz.socketUDP);
        thiz.socketUDP = 0;
        return false;
    }

    if (Socket::connect(thiz.socketUDP, (struct sockaddr*)&sockaddrDestination, sockaddrDestinationLength) < 0)
    {
        CONNECT_LOG(UDP, -1, "%s %s", "connect", Socket::strerror(Socket::errno));
        Socket::close(thiz.socketUDP);
        thiz.socketUDP = 0;
        return false;
    }

    std::stacking_thread(65536, [this]{ thiz.ProcedureRecvUDP(); }).swap(thiz.threadRecvUDP);
    std::stacking_thread(65536, [this]{ thiz.ProcedureSendUDP(); }).swap(thiz.threadSendUDP);
    if (thiz.threadRecvUDP.joinable() == false || thiz.threadSendUDP.joinable() == false)
    {
        CONNECT_LOG(UDP, -1, "%s %s", "thread", ::strerror(errno));
        return false;
    }

    CONNECT_LOG(UDP, 0, "%s %s", "connect", "ready");
    return true;
}
//------------------------------------------------------------------------------
bool Connection::Alive()
{
    return (thiz.terminate == false);
}
//------------------------------------------------------------------------------
void Connection::Disconnect()
{
    std::thread([this]{ delete this; }).detach();
}
//------------------------------------------------------------------------------
void Connection::Send(const Buffer& buffer)
{
    if (thiz.terminate)
        return;

    if (thiz.readyUDP && (*buffer).size() <= UDP_MAX_SIZE)
    {
        thiz.sendBufferMutexUDP.lock();
        thiz.sendBufferUDP.emplace_back(buffer);
        thiz.sendBufferMutexUDP.unlock();
        thiz.sendBufferSemaphoreUDP.release();
    }
    else if (thiz.readyTCP)
    {
        thiz.sendBufferMutexTCP.lock();
        thiz.sendBufferTCP.emplace_back(buffer);
        thiz.sendBufferMutexTCP.unlock();
        thiz.sendBufferSemaphoreTCP.release();
    }
}
//------------------------------------------------------------------------------
void Connection::Recv(const Buffer::element_type& buffer)
{
    CONNECT_LOG(TCP, 0, "%s %zd", "recv", buffer.size());
}
//------------------------------------------------------------------------------
void Connection::GetAddressPort(const struct sockaddr_storage& addr, char*& address, char*& port)
{
    if (addr.ss_family == AF_INET)
    {
        sockaddr_in& sa = *(sockaddr_in*)&addr;

        address = (char*)::realloc(address, INET_ADDRSTRLEN);
        ::inet_ntop(AF_INET, &sa.sin_addr, address, INET_ADDRSTRLEN);

        port = (char*)::realloc(port, 8);
        ::snprintf(port, 8, "%u", ntohs(sa.sin_port));
    }
    else if (addr.ss_family == AF_INET6)
    {
        sockaddr_in6& sa = *(sockaddr_in6*)&addr;

        address = (char*)::realloc(address, INET6_ADDRSTRLEN);
        ::inet_ntop(AF_INET6, &sa.sin6_addr, address, INET6_ADDRSTRLEN);

        port = (char*)::realloc(port, 8);
        ::snprintf(port, 8, "%u", ntohs(sa.sin6_port));
    }
}
//------------------------------------------------------------------------------
int Connection::SetAddressPort(struct sockaddr_storage& addr, const char* address, const char* port)
{
    if (strchr(address, '.'))
    {
        sockaddr_in& sa = *(sockaddr_in*)&addr;

        sa = sockaddr_in{};
#if defined(__APPLE__)
        sa.sin_len = sizeof(sockaddr_in);
#endif
        sa.sin_family = AF_INET;
        sa.sin_port = htons(::atoi(port));
        ::inet_pton(AF_INET, address, &sa.sin_addr);

        return sizeof(sockaddr_in);
    }
    else if (strchr(address, ':'))
    {
        sockaddr_in6& sa = *(sockaddr_in6*)&addr;

        sa = sockaddr_in6{};
#if defined(__APPLE__)
        sa.sin6_len = sizeof(sockaddr_in6);
#endif
        sa.sin6_family = AF_INET6;
        sa.sin6_port = htons(::atoi(port));
        ::inet_pton(AF_INET6, address, &sa.sin6_addr);

        return sizeof(sockaddr_in6);
    }

    return 0;
}
//------------------------------------------------------------------------------
int Connection::GetActiveThreadCount(int index)
{
    if (index < 0 || index > 4)
        return 0;
    return Connection::activeThreadCount[index];
}
//------------------------------------------------------------------------------
