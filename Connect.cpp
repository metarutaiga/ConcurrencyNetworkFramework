//==============================================================================
// ConcurrencyNetworkFramework : Connect Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include "Listen.h"
#include "Log.h"
#include "Connect.h"

#define CONNECT_LOG(level, format, ...) \
    Log::Format(level, "%s %d (%s:%s@%s:%s) : " format, "Connect", thiz.socket, thiz.sourceAddress, thiz.sourcePort, thiz.destinationAddress, thiz.destinationPort, __VA_ARGS__)

//------------------------------------------------------------------------------
Connect::Connect(int socket, const char* address, const char* port, const struct sockaddr_storage& addr)
{
    thiz.terminate = false;
    thiz.socket = socket;
    thiz.sourceAddress = address ? strdup(address) : nullptr;
    thiz.sourcePort = port ? strdup(port) : nullptr;
    thiz.destinationAddress = nullptr;
    thiz.destinationPort = nullptr;
    thiz.threadRecv = nullptr;
    thiz.threadSend = nullptr;
    thiz.sendBufferAvailable = 1;
    thiz.sendBufferSemaphore = sem_t();
    sem_init(&thiz.sendBufferSemaphore, 0, 0);

    GetAddressPort(addr, thiz.destinationAddress, thiz.destinationPort);
}
//------------------------------------------------------------------------------
Connect::Connect(const char* address, const char* port)
{
    thiz.terminate = false;
    thiz.socket = 0;
    thiz.sourceAddress = nullptr;
    thiz.sourcePort = nullptr;
    thiz.destinationAddress = address ? strdup(address) : nullptr;
    thiz.destinationPort = port ? strdup(port) : nullptr;
    thiz.threadRecv = nullptr;
    thiz.threadSend = nullptr;
    thiz.sendBufferAvailable = 1;
    thiz.sendBufferSemaphore = sem_t();
    sem_init(&thiz.sendBufferSemaphore, 0, 0);

    struct addrinfo* addrinfo = nullptr;
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int error = ::getaddrinfo(thiz.destinationAddress, thiz.destinationPort, &hints, &addrinfo);
    if (addrinfo == nullptr)
    {
        CONNECT_LOG(-1, "%s %s", "getaddrinfo", gai_strerror(error));
        return;
    }

    thiz.socket = ::socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (thiz.socket <= 0)
    {
        CONNECT_LOG(-1, "%s %s", "socket", strerror(errno));
        ::freeaddrinfo(addrinfo);
        return;
    }

    if (::connect(thiz.socket, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0)
    {
        CONNECT_LOG(-1, "%s %s", "connect", strerror(errno));
        ::freeaddrinfo(addrinfo);
        return;
    }

    struct sockaddr_storage addr = {};
    socklen_t size = sizeof(addr);
    if (::getsockname(thiz.socket, (struct sockaddr*)&addr, &size) == 0)
    {
        GetAddressPort(addr, thiz.sourceAddress, thiz.sourcePort);
    }

    ::freeaddrinfo(addrinfo);
}
//------------------------------------------------------------------------------
Connect::~Connect()
{
    Stop();

    if (thiz.threadRecv)
    {
        ::pthread_join(thiz.threadRecv, nullptr);
        thiz.threadRecv = nullptr;
    }
    if (thiz.threadSend)
    {
        ::pthread_join(thiz.threadSend, nullptr);
        thiz.threadSend = nullptr;
    }
    if (thiz.sendBufferSemaphore)
    {
        sem_destroy(&thiz.sendBufferSemaphore);
        thiz.sendBufferSemaphore = sem_t();
    }
    if (thiz.sourceAddress)
    {
        free(thiz.sourceAddress);
        thiz.sourceAddress = nullptr;
    }
    if (thiz.sourcePort)
    {
        free(thiz.sourcePort);
        thiz.sourcePort = nullptr;
    }
    if (thiz.destinationAddress)
    {
        free(thiz.destinationAddress);
        thiz.destinationAddress = nullptr;
    }
    if (thiz.destinationPort)
    {
        free(thiz.destinationPort);
        thiz.destinationPort = nullptr;
    }
}
//------------------------------------------------------------------------------
void Connect::ProcedureRecv()
{
    std::vector<char> buffer;

    thiz.terminate = false;

    while (thiz.terminate == false)
    {
        unsigned short size = 0;
        if (::recv(thiz.socket, &size, sizeof(short), MSG_WAITALL | MSG_NOSIGNAL) <= 0)
        {
            CONNECT_LOG(-1, "%s %s", "recv", strerror(errno));
            break;
        }
        buffer.resize(size);
        if (::recv(thiz.socket, &buffer.front(), size, MSG_WAITALL | MSG_NOSIGNAL) <= 0)
        {
            CONNECT_LOG(-1, "%s %s", "recv", strerror(errno));
            break;
        }

        Recv(buffer);
    }

    thiz.terminate = true;
}
//------------------------------------------------------------------------------
void Connect::ProcedureSend()
{
    std::vector<char> buffer;

    thiz.terminate = false;

    while (thiz.terminate == false)
    {
        for (int available = 1; available > 0; available = __atomic_sub_fetch(&thiz.sendBufferAvailable, 1, __ATOMIC_ACQ_REL))
        {
            thiz.sendBufferMutex.lock();
            if (thiz.sendBuffer.empty() == false)
            {
                thiz.sendBuffer.front().swap(buffer);
                thiz.sendBuffer.erase(thiz.sendBuffer.begin());
            }
            thiz.sendBufferMutex.unlock();
            if (buffer.empty())
                continue;

            unsigned short size = (short)buffer.size();
            if (::send(thiz.socket, &size, sizeof(short), MSG_WAITALL | MSG_NOSIGNAL | MSG_MORE) <= 0)
            {
                CONNECT_LOG(-1, "%s %s", "send", strerror(errno));
                break;
            }
            if (::send(thiz.socket, &buffer.front(), size, MSG_WAITALL | MSG_NOSIGNAL) <= 0)
            {
                CONNECT_LOG(-1, "%s %s", "send", strerror(errno));
                break;
            }

            buffer.clear();
        }

#if 1
        struct timespec timespec;
        timespec.tv_sec = 10;
        timespec.tv_nsec = 0;
        sem_timedwait(&thiz.sendBufferSemaphore, &timespec);
#else
        sem_wait(&thiz.sendBufferSemaphore);
#endif
    }

    thiz.terminate = true;
}
//------------------------------------------------------------------------------
void* Connect::ProcedureRecvThread(void* arg)
{
    Connect& connect = *(Connect*)arg;
    connect.ProcedureRecv();
    return nullptr;
}
//------------------------------------------------------------------------------
void* Connect::ProcedureSendThread(void* arg)
{
    Connect& connect = *(Connect*)arg;
    connect.ProcedureSend();
    return nullptr;
}
//------------------------------------------------------------------------------
bool Connect::Start()
{
    if (thiz.socket <= 0)
        return false;
    if (thiz.threadRecv || thiz.threadSend)
        return true;

    int enable = 1;
    ::setsockopt(thiz.socket, SOL_SOCKET, SO_KEEPALIVE, (void*)&enable, sizeof(enable));
    ::setsockopt(thiz.socket, SOL_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));

    pthread_attr_t attr;
    ::pthread_attr_init(&attr);
    ::pthread_attr_setstacksize(&attr, 65536);

    ::pthread_create(&thiz.threadRecv, &attr, ProcedureRecvThread, this);
    ::pthread_create(&thiz.threadSend, &attr, ProcedureSendThread, this);
    if (thiz.threadRecv == nullptr || thiz.threadSend == nullptr)
    {
        CONNECT_LOG(-1, "%s %s", "thread", strerror(errno));
        return false;
    }

    CONNECT_LOG(0, "%s %s", "connect", "ready");
    return true;
}
//------------------------------------------------------------------------------
void Connect::Stop()
{
    thiz.terminate = true;

    if (thiz.socket > 0)
    {
        ::close(thiz.socket);
        thiz.socket = 0;
    }
    if (thiz.sendBufferSemaphore)
    {
        sem_post(&thiz.sendBufferSemaphore);
    }
}
//------------------------------------------------------------------------------
bool Connect::Alive()
{
    return (terminate == false);
}
//------------------------------------------------------------------------------
void Connect::Disconnect()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t thread;
    pthread_create(&thread, &attr, [](void* arg) -> void* { delete (Connect*)arg; return nullptr; }, this);
}
//------------------------------------------------------------------------------
void Connect::Send(std::vector<char>&& buffer)
{
    thiz.sendBufferMutex.lock();
    thiz.sendBuffer.emplace_back(buffer);
    thiz.sendBufferMutex.unlock();

    int available = __atomic_add_fetch(&thiz.sendBufferAvailable, 1, __ATOMIC_ACQ_REL);
    if (available <= 1)
    {
        __atomic_add_fetch(&thiz.sendBufferAvailable, 1, __ATOMIC_ACQ_REL);
        sem_post(&thiz.sendBufferSemaphore);
    }
}
//------------------------------------------------------------------------------
void Connect::Recv(const std::vector<char>& buffer) const
{
    CONNECT_LOG(0, "%s %zd", "recv", buffer.size());
}
//------------------------------------------------------------------------------
void Connect::GetAddressPort(const struct sockaddr_storage& addr, char*& address, char*& port)
{
    if (addr.ss_family == AF_INET)
    {
        sockaddr_in& sa = *(sockaddr_in*)&addr;

        address = (char*)realloc(address, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &sa.sin_addr, address, INET_ADDRSTRLEN);

        port = (char*)realloc(port, 8);
        snprintf(port, 8, "%u", ntohs(sa.sin_port));
    }
    else if (addr.ss_family == AF_INET6)
    {
        sockaddr_in6& sa = *(sockaddr_in6*)&addr;

        address = (char*)realloc(address, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &sa.sin6_addr, address, INET6_ADDRSTRLEN);

        port = (char*)realloc(port, 8);
        snprintf(port, 8, "%u", ntohs(sa.sin6_port));
    }
}
//------------------------------------------------------------------------------
