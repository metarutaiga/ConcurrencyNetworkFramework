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
#include "Connection.h"

#define TIMED_SEMAPHORE 1

#define CONNECT_LOG(level, format, ...) \
    Log::Format(level, "%s %d (%s:%s@%s:%s) : " format, "Connect", thiz.socket, thiz.sourceAddress, thiz.sourcePort, thiz.destinationAddress, thiz.destinationPort, __VA_ARGS__)

int Connection::activeThreadCount = 0;
//------------------------------------------------------------------------------
Connection::Connection(int socket, const char* address, const char* port, const struct sockaddr_storage& addr)
{
    thiz.terminate = false;
    thiz.socket = socket;
    thiz.sourceAddress = address ? strdup(address) : nullptr;
    thiz.sourcePort = port ? strdup(port) : nullptr;
    thiz.destinationAddress = nullptr;
    thiz.destinationPort = nullptr;
    thiz.threadRecv = nullptr;
    thiz.threadSend = nullptr;
    thiz.sendBufferSemaphore = sem_t();
    sem_init(&thiz.sendBufferSemaphore, 0, 0);
    if (thiz.sendBufferSemaphore == sem_t())
    {
        CONNECT_LOG(-1, "%s %s", "sem_init", strerror(errno));
    }

    GetAddressPort(addr, thiz.destinationAddress, thiz.destinationPort);
}
//------------------------------------------------------------------------------
Connection::Connection(const char* address, const char* port)
{
    thiz.terminate = false;
    thiz.socket = 0;
    thiz.sourceAddress = nullptr;
    thiz.sourcePort = nullptr;
    thiz.destinationAddress = address ? strdup(address) : nullptr;
    thiz.destinationPort = port ? strdup(port) : nullptr;
    thiz.threadRecv = nullptr;
    thiz.threadSend = nullptr;
    thiz.sendBufferSemaphore = sem_t();
    sem_init(&thiz.sendBufferSemaphore, 0, 0);
    if (thiz.sendBufferSemaphore == sem_t())
    {
        CONNECT_LOG(-1, "%s %s", "sem_init", strerror(errno));
    }

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
Connection::~Connection()
{
    thiz.terminate = true;

    if (thiz.socket > 0)
    {
        ::close(thiz.socket);
        thiz.socket = 0;
    }
    if (thiz.sendBufferSemaphore)
    {
#if TIMED_SEMAPHORE
        if (sem_post(&thiz.sendBufferSemaphore) < 0)
        {
            CONNECT_LOG(-1, "%s %s", "sem_post", strerror(errno));
        }
#else
        while (sem_post(&thiz.sendBufferSemaphore) < 0)
        {
            struct timespec timespec;
            timespec.tv_sec = 0;
            timespec.tv_nsec = 1000 * 1000 * 1000 / 60;
            nanosleep(&timespec, nullptr);
        }
#endif
    }
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
void Connection::ProcedureRecv()
{
    std::vector<char> buffer;

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
void Connection::ProcedureSend()
{
    std::vector<char> buffer;

    while (thiz.terminate == false)
    {
        buffer.clear();
        thiz.sendBufferMutex.lock();
        if (thiz.sendBuffer.empty() == false)
        {
            thiz.sendBuffer.front().swap(buffer);
            thiz.sendBuffer.erase(thiz.sendBuffer.begin());
        }
        thiz.sendBufferMutex.unlock();
        if (buffer.empty() == false)
        {
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
        }

#if TIMED_SEMAPHORE
        struct timeval timeval;
        gettimeofday(&timeval, nullptr);
        struct timespec timespec;
        timespec.tv_sec = timeval.tv_sec + 10;
        timespec.tv_nsec = timeval.tv_usec * 1000;
        sem_timedwait(&thiz.sendBufferSemaphore, &timespec);
#else
        sem_wait(&thiz.sendBufferSemaphore);
#endif
    }

    thiz.terminate = true;
}
//------------------------------------------------------------------------------
void* Connection::ProcedureRecvThread(void* arg)
{
    Connection& connect = *(Connection*)arg;
    __atomic_add_fetch(&activeThreadCount, 1, __ATOMIC_ACQ_REL);
    connect.ProcedureRecv();
    __atomic_sub_fetch(&activeThreadCount, 1, __ATOMIC_ACQ_REL);
    return nullptr;
}
//------------------------------------------------------------------------------
void* Connection::ProcedureSendThread(void* arg)
{
    Connection& connect = *(Connection*)arg;
    __atomic_add_fetch(&activeThreadCount, 1, __ATOMIC_ACQ_REL);
    connect.ProcedureSend();
    __atomic_sub_fetch(&activeThreadCount, 1, __ATOMIC_ACQ_REL);
    return nullptr;
}
//------------------------------------------------------------------------------
bool Connection::Connect()
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

    ::pthread_attr_destroy(&attr);
    if (thiz.threadRecv == nullptr || thiz.threadSend == nullptr)
    {
        CONNECT_LOG(-1, "%s %s", "thread", strerror(errno));
        return false;
    }

    CONNECT_LOG(0, "%s %s", "connect", "ready");
    return true;
}
//------------------------------------------------------------------------------
bool Connection::Alive()
{
    return (terminate == false);
}
//------------------------------------------------------------------------------
void Connection::Disconnect()
{
    pthread_attr_t attr;
    ::pthread_attr_init(&attr);
    ::pthread_attr_setstacksize(&attr, 65536);
    ::pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t thread = nullptr;
    ::pthread_create(&thread, &attr, [](void* arg) -> void* { delete (Connection*)arg; return nullptr; }, this);

    ::pthread_attr_destroy(&attr);
}
//------------------------------------------------------------------------------
void Connection::Send(std::vector<char>&& buffer)
{
    thiz.sendBufferMutex.lock();
    thiz.sendBuffer.emplace_back(buffer);
    thiz.sendBufferMutex.unlock();

    if (sem_post(&thiz.sendBufferSemaphore) < 0)
    {
        CONNECT_LOG(-1, "%s %s", "sem_post", strerror(errno));
    }
}
//------------------------------------------------------------------------------
void Connection::Recv(const std::vector<char>& buffer)
{
    CONNECT_LOG(0, "%s %zd", "recv", buffer.size());
}
//------------------------------------------------------------------------------
void Connection::GetAddressPort(const struct sockaddr_storage& addr, char*& address, char*& port)
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
int Connection::GetActiveThreadCount()
{
    return activeThreadCount;
}
//------------------------------------------------------------------------------
