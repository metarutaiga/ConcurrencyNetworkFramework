//==============================================================================
// ConcurrencyNetworkFramework : Connection Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <arpa/inet.h>
#include <netinet/tcp.h>
#if defined(__linux__)
#   include <linux/filter.h>
#endif
#include "Listener.h"
#include "Log.h"
#include "Socket.h"
#include "Connection.h"

#define CONNECT_LOG(proto, level, format, ...) \
    Log::Format(Log::level, "%s %d (%s:%s:%s@%s:%s) : " format, "Connect", thiz.socket ## proto, #proto, thiz.sourceAddress, thiz.sourcePort, thiz.destinationAddress, thiz.destinationPort, __VA_ARGS__)

std::atomic_int Connection::activeThreadCount[4];
//------------------------------------------------------------------------------
Connection::Connection(int socket, const char* address, const char* port, const struct sockaddr_storage& addr)
{
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
    thiz.socketTCP = 0;
    thiz.socketUDP = 0;
    thiz.readyTCP = false;
    thiz.readyUDP = false;
    thiz.sourceAddress = nullptr;
    thiz.sourcePort = nullptr;
    thiz.destinationAddress = address ? ::strdup(address) : nullptr;
    thiz.destinationPort = port ? ::strdup(port) : nullptr;

    struct sockaddr_storage sockaddrDestination = {};
    socklen_t sockaddrDestinationLength = SetAddressPort(sockaddrDestination, thiz.destinationAddress, thiz.destinationPort);
    thiz.socketTCP = Socket::socket(sockaddrDestination.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (thiz.socketTCP <= 0)
    {
        CONNECT_LOG(TCP, ERROR, "%s %s", "socket", Socket::strerror(Socket::errno));
        return;
    }

    if (Socket::connect(thiz.socketTCP, reinterpret_cast<struct sockaddr*>(&sockaddrDestination), sockaddrDestinationLength) < 0)
    {
        CONNECT_LOG(TCP, ERROR, "%s %s", "connect", Socket::strerror(Socket::errno));
        Socket::close(thiz.socketTCP);
        thiz.socketTCP = 0;
        return;
    }
    thiz.readyTCP = true;

    struct sockaddr_storage sockaddrSource = {};
    socklen_t sockaddSourceLength = sizeof(sockaddrSource);
    if (Socket::getsockname(thiz.socketTCP, reinterpret_cast<struct sockaddr*>(&sockaddrSource), &sockaddSourceLength) == 0)
    {
        GetAddressPort(sockaddrSource, thiz.sourceAddress, thiz.sourcePort);
    }
}
//------------------------------------------------------------------------------
Connection::~Connection()
{
    Base::Terminate();

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

    Buffer recvBuffer = Buffer::Get();

    while (Base::Terminating() == false && recvBuffer)
    {
        Buffer::element_type& buffer = (*recvBuffer);

        // Length
        unsigned short size = 0;
        if (Socket::recv(thiz.socketTCP, &size, sizeof(short), MSG_WAITALL | MSG_NOSIGNAL) <= 0)
        {
            CONNECT_LOG(TCP, ERROR, "%s %s", "recv", Socket::strerror(Socket::errno));
            break;
        }

        // Preserve
        if (size == 0)
        {
            CONNECT_LOG(TCP, ERROR, "%s %s", "recv", "empty");
            continue;
        }

        // Data
        buffer.resize(size);
        if (Socket::recv(thiz.socketTCP, &buffer.front(), size, MSG_WAITALL | MSG_NOSIGNAL) <= 0)
        {
            CONNECT_LOG(TCP, ERROR, "%s %s", "recv", Socket::strerror(Socket::errno));
            break;
        }

        ProcessRecvTCP(buffer, buffer);
        Recv(recvBuffer, MODE_TCP);
    }
    Base::Terminate();

    thiz.readyTCP = false;

    Connection::activeThreadCount[0].fetch_sub(1, std::memory_order_acq_rel);
}
//------------------------------------------------------------------------------
void Connection::ProcedureSendTCP()
{
    Connection::activeThreadCount[1].fetch_add(1, std::memory_order_acq_rel);

    bool disconnect = false;
    Buffer::element_type buffer;
    char cork[Socket::CORK_SIZE] = {};
    std::vector<Buffer> sendBufferTCPLocal;

    while (Base::Terminating() == false && disconnect == false)
    {
        thiz.sendBufferSemaphoreTCP.acquire();
        thiz.sendBufferMutexTCP.lock();
        thiz.sendBufferTCP.swap(sendBufferTCPLocal);
        thiz.sendBufferMutexTCP.unlock();
        if (Base::Terminating())
            break;
        for (const Buffer& sendBuffer : sendBufferTCPLocal)
        {
            if (sendBuffer == nullptr)
                continue;

            ProcessSendTCP((*sendBuffer), buffer);
            if (buffer.size() <= USHRT_MAX)
            {
                // Length
                unsigned short size = (short)buffer.size();
                if (Socket::send(thiz.socketTCP, &size, sizeof(short), MSG_WAITALL | MSG_NOSIGNAL | MSG_MORE, cork) <= 0)
                {
                    disconnect = true;
                    CONNECT_LOG(TCP, ERROR, "%s %s", "send", Socket::strerror(Socket::errno));
                    break;
                }

                // Preserve
                if (size == 0)
                {
                    CONNECT_LOG(TCP, ERROR, "%s %s", "send", "empty");
                    continue;
                }

                // Data
                if (Socket::send(thiz.socketTCP, &buffer.front(), size, MSG_WAITALL | MSG_NOSIGNAL, cork) <= 0)
                {
                    disconnect = true;
                    CONNECT_LOG(TCP, ERROR, "%s %s", "send", Socket::strerror(Socket::errno));
                    break;
                }
            }
            else
            {
                CONNECT_LOG(TCP, ERROR, "%s %s %zd", "send", "buffer too long", buffer.size());
            }
        }
        sendBufferTCPLocal.clear();
    }
    Base::Terminate();

    thiz.readyTCP = false;

    Connection::activeThreadCount[1].fetch_sub(1, std::memory_order_acq_rel);
}
//------------------------------------------------------------------------------
void Connection::ProcedureRecvUDP()
{
    Connection::activeThreadCount[2].fetch_add(1, std::memory_order_acq_rel);

    Buffer recvBuffer = Buffer::Get();

    while (Base::Terminating() == false && recvBuffer)
    {
        Buffer::element_type& buffer = (*recvBuffer);
        buffer.resize(UDP_SIZE_MAX);

        // Data
        long size = Socket::recv(thiz.socketUDP, &buffer.front(), buffer.size(), MSG_WAITALL | MSG_NOSIGNAL);
        if (size <= 0)
        {
            CONNECT_LOG(UDP, ERROR, "%s %s", "recv", Socket::strerror(Socket::errno));
            break;
        }
        buffer.resize(size);

        ProcessRecvUDP(buffer, buffer);
        Recv(recvBuffer, MODE_UDP);
    }

    thiz.readyUDP = false;

    Connection::activeThreadCount[2].fetch_sub(1, std::memory_order_acq_rel);
}
//------------------------------------------------------------------------------
void Connection::ProcedureSendUDP()
{
    Connection::activeThreadCount[3].fetch_add(1, std::memory_order_acq_rel);

    bool disconnect = false;
    Buffer::element_type buffer;
    char cork[Socket::CORK_SIZE] = {};
    std::vector<Buffer> sendBufferUDPLocal;

    while (Base::Terminating() == false && disconnect == false)
    {
        thiz.sendBufferSemaphoreUDP.acquire();
        thiz.sendBufferMutexUDP.lock();
        thiz.sendBufferUDP.swap(sendBufferUDPLocal);
        thiz.sendBufferMutexUDP.unlock();
        if (Base::Terminating())
            break;
        for (const Buffer& sendBuffer : sendBufferUDPLocal)
        {
            if (sendBuffer == nullptr)
                continue;

            ProcessSendUDP((*sendBuffer), buffer);
            if (buffer.size() <= UDP_SIZE_MAX)
            {
                // Data
                if (Socket::send(thiz.socketUDP, &buffer.front(), buffer.size(), MSG_WAITALL | MSG_NOSIGNAL, cork) <= 0)
                {
                    disconnect = true;
                    CONNECT_LOG(UDP, ERROR, "%s %s", "send", Socket::strerror(Socket::errno));
                    break;
                }
            }
            else
            {
                CONNECT_LOG(UDP, ERROR, "%s %s %zd", "send", "buffer too long", buffer.size());
            }
        }
        sendBufferUDPLocal.clear();
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
    if (Socket::setsockopt(thiz.socketTCP, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) < 0)
    {
        CONNECT_LOG(TCP, ERROR, "%s %s", "setsockopt", "SO_KEEPALIVE");
    }
    if (Socket::setsockopt(thiz.socketTCP, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable)) < 0)
    {
        CONNECT_LOG(TCP, ERROR, "%s %s", "setsockopt", "TCP_NODELAY");
    }

    thiz.threadRecvTCP = std::stacking_thread(65536, [this]{ thiz.ProcedureRecvTCP(); });
    thiz.threadSendTCP = std::stacking_thread(65536, [this]{ thiz.ProcedureSendTCP(); });
    if (thiz.threadRecvTCP.joinable() == false || thiz.threadSendTCP.joinable() == false)
    {
        CONNECT_LOG(TCP, ERROR, "%s %s", "thread", ::strerror(errno));
        return false;
    }

    CONNECT_LOG(TCP, INFO, "%s %s", "connect", "ready");
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

    struct sockaddr_storage sockaddrSource = {};
    socklen_t sockaddSourceLength = SetAddressPort(sockaddrSource, thiz.sourceAddress, thiz.sourcePort);
    thiz.socketUDP = Socket::socket(sockaddrSource.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (thiz.socketUDP <= 0)
    {
        CONNECT_LOG(UDP, ERROR, "%s %s", "socket", Socket::strerror(Socket::errno));
        return false;
    }

    int enable = 1;
    if (Socket::setsockopt(thiz.socketUDP, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
    {
        CONNECT_LOG(UDP, ERROR, "%s %s", "setsockopt", "SO_REUSEADDR");
    }
    if (Socket::setsockopt(thiz.socketUDP, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) < 0)
    {
        CONNECT_LOG(UDP, ERROR, "%s %s", "setsockopt", "SO_REUSEPORT");
    }

#if defined(__linux__)
    struct sock_filter filter[] =
    {
        /* A = raw_smp_processor_id() */
        { BPF_LD  | BPF_W | BPF_ABS, 0, 0, (unsigned int)(SKF_AD_OFF + SKF_AD_CPU) },
        /* return A */
        { BPF_RET | BPF_A, 0, 0, 0 },
    };
    struct sock_fprog cbpf =
    {
        .len = 2,
        .filter = filter,
    };
    if (Socket::setsockopt(thiz.socketUDP, SOL_SOCKET, SO_ATTACH_REUSEPORT_CBPF, &cbpf, sizeof(cbpf)) < 0)
    {
        CONNECT_LOG(UDP, ERROR, "%s %s", "setsockopt", "SO_ATTACH_REUSEPORT_CBPF");
    }
#endif

    if (Socket::bind(thiz.socketUDP, reinterpret_cast<struct sockaddr*>(&sockaddrSource), sockaddSourceLength) < 0)
    {
        CONNECT_LOG(UDP, ERROR, "%s %s", "bind", Socket::strerror(Socket::errno));
        Socket::close(thiz.socketUDP);
        thiz.socketUDP = 0;
        return false;
    }

    struct sockaddr_storage sockaddrDestination = {};
    socklen_t sockaddrDestinationLength = SetAddressPort(sockaddrDestination, thiz.destinationAddress, thiz.destinationPort);
    if (Socket::connect(thiz.socketUDP, reinterpret_cast<struct sockaddr*>(&sockaddrDestination), sockaddrDestinationLength) < 0)
    {
        CONNECT_LOG(UDP, ERROR, "%s %s", "connect", Socket::strerror(Socket::errno));
        Socket::close(thiz.socketUDP);
        thiz.socketUDP = 0;
        return false;
    }

    thiz.threadRecvUDP = std::stacking_thread(65536, [this]{ thiz.ProcedureRecvUDP(); });
    thiz.threadSendUDP = std::stacking_thread(65536, [this]{ thiz.ProcedureSendUDP(); });
    if (thiz.threadRecvUDP.joinable() == false || thiz.threadSendUDP.joinable() == false)
    {
        CONNECT_LOG(UDP, ERROR, "%s %s", "thread", ::strerror(errno));
        return false;
    }

    CONNECT_LOG(UDP, INFO, "%s %s", "connect", "ready");
    return true;
}
//------------------------------------------------------------------------------
bool Connection::Alive()
{
    return (Base::Terminating() == false);
}
//------------------------------------------------------------------------------
void Connection::Disconnect()
{
    std::thread([this]{ delete this; }).detach();
}
//------------------------------------------------------------------------------
void Connection::Send(const Buffer& sendBuffer, int mode)
{
    if (Base::Terminating())
        return;

    if (thiz.readyUDP && (mode != MODE_TCP && (*sendBuffer).size() <= UDP_SIZE_MAX))
    {
        thiz.sendBufferMutexUDP.lock();
        thiz.sendBufferUDP.emplace_back(sendBuffer);
        thiz.sendBufferMutexUDP.unlock();
        thiz.sendBufferSemaphoreUDP.release();
    }
    else if (thiz.readyTCP)
    {
        thiz.sendBufferMutexTCP.lock();
        thiz.sendBufferTCP.emplace_back(sendBuffer);
        thiz.sendBufferMutexTCP.unlock();
        thiz.sendBufferSemaphoreTCP.release();
    }
}
//------------------------------------------------------------------------------
void Connection::Recv(const Buffer& recvBuffer, int mode)
{
    if (recvBuffer == nullptr)
        return;

    if (mode == MODE_UDP)
    {
        CONNECT_LOG(UDP, INFO, "%s %zd", "recv", (*recvBuffer).size());
    }
    else if (mode == MODE_TCP)
    {
        CONNECT_LOG(TCP, INFO, "%s %zd", "recv", (*recvBuffer).size());
    }
}
//------------------------------------------------------------------------------
void Connection::ProcessSendTCP(const Buffer::element_type& source, Buffer::element_type& destination)
{
    destination = source;
}
//------------------------------------------------------------------------------
void Connection::ProcessRecvTCP(const Buffer::element_type& source, Buffer::element_type& destination)
{
    destination = source;
}
//------------------------------------------------------------------------------
void Connection::ProcessSendUDP(const Buffer::element_type& source, Buffer::element_type& destination)
{
    destination = source;
}
//------------------------------------------------------------------------------
void Connection::ProcessRecvUDP(const Buffer::element_type& source, Buffer::element_type& destination)
{
    destination = source;
}
//------------------------------------------------------------------------------
void Connection::GetAddressPort(const struct sockaddr_storage& addr, char*& address, char*& port)
{
    if (addr.ss_family == AF_INET)
    {
        const struct sockaddr_in& sa = reinterpret_cast<const struct sockaddr_in&>(addr);

        address = static_cast<char*>(::realloc(address, INET_ADDRSTRLEN));
        ::inet_ntop(AF_INET, &sa.sin_addr, address, INET_ADDRSTRLEN);

        port = static_cast<char*>(::realloc(port, 8));
        ::snprintf(port, 8, "%u", ntohs(sa.sin_port));
    }
    else if (addr.ss_family == AF_INET6)
    {
        const struct sockaddr_in6& sa = reinterpret_cast<const struct sockaddr_in6&>(addr);

        address = static_cast<char*>(::realloc(address, INET6_ADDRSTRLEN));
        ::inet_ntop(AF_INET6, &sa.sin6_addr, address, INET6_ADDRSTRLEN);

        port = static_cast<char*>(::realloc(port, 8));
        ::snprintf(port, 8, "%u", ntohs(sa.sin6_port));
    }
}
//------------------------------------------------------------------------------
int Connection::SetAddressPort(struct sockaddr_storage& addr, const char* address, const char* port)
{
    if (address == nullptr || port == nullptr)
        return 0;

    if (strchr(address, '.'))
    {
        struct sockaddr_in& sa = reinterpret_cast<struct sockaddr_in&>(addr);

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
        struct sockaddr_in6& sa = reinterpret_cast<struct sockaddr_in6&>(addr);

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
    if (index < 0 || index >= 4)
        return 0;

    return Connection::activeThreadCount[index];
}
//------------------------------------------------------------------------------
