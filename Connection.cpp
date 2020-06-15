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

#define TIMED_SEMAPHORE 1

#define CONNECT_LOG(level, format, ...) \
    Log::Format(level, "%s %d (%s:%s@%s:%s) : " format, "Connect", thiz.socket, thiz.sourceAddress, thiz.sourcePort, thiz.destinationAddress, thiz.destinationPort, __VA_ARGS__)

std::atomic_uint Connection::activeThreadCount;
//------------------------------------------------------------------------------
Connection::Connection(int socket, const char* address, const char* port, const struct sockaddr_storage& addr)
{
    thiz.terminate = false;
    thiz.socket = socket;
    thiz.sourceAddress = address ? strdup(address) : nullptr;
    thiz.sourcePort = port ? strdup(port) : nullptr;
    thiz.destinationAddress = nullptr;
    thiz.destinationPort = nullptr;
    thiz.threadRecv = pthread_t();
    thiz.threadSend = pthread_t();
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
    thiz.threadRecv = pthread_t();
    thiz.threadSend = pthread_t();
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

    thiz.socket = Socket::socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (thiz.socket <= 0)
    {
        CONNECT_LOG(-1, "%s %s", "socket", strerror(errno));
        ::freeaddrinfo(addrinfo);
        return;
    }

    if (Socket::connect(thiz.socket, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0)
    {
        CONNECT_LOG(-1, "%s %s", "connect", strerror(errno));
        ::freeaddrinfo(addrinfo);
        return;
    }

    struct sockaddr_storage addr = {};
    socklen_t size = sizeof(addr);
    if (Socket::getsockname(thiz.socket, (struct sockaddr*)&addr, &size) == 0)
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
        Socket::shutdown(thiz.socket, SHUT_RD);
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
        thiz.threadRecv = pthread_t();
    }
    if (thiz.threadSend)
    {
        ::pthread_join(thiz.threadSend, nullptr);
        thiz.threadSend = pthread_t();
    }
    if (thiz.socket > 0)
    {
        Socket::close(thiz.socket);
        thiz.socket = 0;
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
    Connection::activeThreadCount.fetch_add(1, std::memory_order_acq_rel);

    BufferPtr::element_type buffer;

    auto recv = [this](int socket, void* buffer, size_t bufferSize, int flags)
    {
        ssize_t result = 0;
        while (thiz.terminate == false)
        {
            result = Socket::recv(socket, buffer, bufferSize, flags);
            if (result >= 0)
                break;
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                struct timespec timespec;
                timespec.tv_sec = 0;
                timespec.tv_nsec = 1000 * 1000 * 1000 / 60;
                nanosleep(&timespec, nullptr);
                continue;
            }
            break;
        }
        return result;
    };

    while (thiz.terminate == false)
    {
        // Length
        unsigned short size = 0;
        if (recv(thiz.socket, &size, sizeof(short), MSG_WAITALL | MSG_NOSIGNAL) <= 0)
        {
            CONNECT_LOG(-1, "%s %s", "recv", strerror(errno));
            break;
        }

        // Preserve
        if (size == 0)
        {
            CONNECT_LOG(-1, "%s %s", "recv", "empty");
            continue;
        }

        // Data
        buffer.resize(size);
        if (recv(thiz.socket, &buffer.front(), size, MSG_WAITALL | MSG_NOSIGNAL) <= 0)
        {
            CONNECT_LOG(-1, "%s %s", "recv", strerror(errno));
            break;
        }
        Recv(buffer);
    }

    thiz.terminate = true;

    Connection::activeThreadCount.fetch_sub(1, std::memory_order_acq_rel);
}
//------------------------------------------------------------------------------
void Connection::ProcedureSend()
{
    Connection::activeThreadCount.fetch_add(1, std::memory_order_acq_rel);

    auto send = [this](int socket, const void* buffer, size_t bufferSize, int flags)
    {
        ssize_t result = 0;
        while (thiz.terminate == false)
        {
            result = Socket::send(socket, buffer, bufferSize, flags);
            if (result >= 0)
                break;
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                struct timespec timespec;
                timespec.tv_sec = 0;
                timespec.tv_nsec = 1000 * 1000 * 1000 / 60;
                nanosleep(&timespec, nullptr);
                continue;
            }
            break;
        }
        return result;
    };

    while (thiz.terminate == false)
    {
        BufferPtr bufferPtr;
        thiz.sendBufferMutex.lock();
        if (thiz.sendBuffer.empty() == false)
        {
            thiz.sendBuffer.front().swap(bufferPtr);
            thiz.sendBuffer.erase(thiz.sendBuffer.begin());
        }
        thiz.sendBufferMutex.unlock();
        if (bufferPtr)
        {
            const auto& buffer = (*bufferPtr);
            if (buffer.size() <= USHRT_MAX)
            {
                // Length
                unsigned short size = (short)buffer.size();
                if (send(thiz.socket, &size, sizeof(short), MSG_WAITALL | MSG_NOSIGNAL | MSG_MORE) <= 0)
                {
                    CONNECT_LOG(-1, "%s %s", "send", strerror(errno));
                    break;
                }

                // Preserve
                if (size == 0)
                {
                    CONNECT_LOG(-1, "%s %s", "send", "empty");
                    continue;
                }

                // Data
                if (send(thiz.socket, &buffer.front(), size, MSG_WAITALL | MSG_NOSIGNAL) <= 0)
                {
                    CONNECT_LOG(-1, "%s %s", "send", strerror(errno));
                    break;
                }
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

    Connection::activeThreadCount.fetch_sub(1, std::memory_order_acq_rel);
}
//------------------------------------------------------------------------------
bool Connection::Connect()
{
    if (thiz.socket <= 0)
        return false;
    if (thiz.threadRecv || thiz.threadSend)
        return true;

    int enable = 1;
    Socket::setsockopt(thiz.socket, SOL_SOCKET, SO_KEEPALIVE, (void*)&enable, sizeof(enable));
    Socket::setsockopt(thiz.socket, SOL_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));

    pthread_attr_t attr;
    ::pthread_attr_init(&attr);
    ::pthread_attr_setstacksize(&attr, 65536);

    ::pthread_create(&thiz.threadRecv, &attr, [](void* arg) -> void*
    {
        Connection& connect = *(Connection*)arg;
        connect.ProcedureRecv();
        return nullptr;
    }, this);
    ::pthread_create(&thiz.threadSend, &attr, [](void* arg) -> void*
    {
        Connection& connect = *(Connection*)arg;
        connect.ProcedureSend();
        return nullptr;
    }, this);

    ::pthread_attr_destroy(&attr);
    if (thiz.threadRecv == pthread_t() || thiz.threadSend == pthread_t())
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

    pthread_t thread = pthread_t();
    ::pthread_create(&thread, &attr, [](void* arg) -> void*
    {
        Connection& connect = *(Connection*)arg;
        delete &connect;
        return nullptr;
    }, this);

    ::pthread_attr_destroy(&attr);
}
//------------------------------------------------------------------------------
void Connection::Send(const BufferPtr& bufferPtr)
{
    thiz.sendBufferMutex.lock();
    thiz.sendBuffer.emplace_back(bufferPtr);
    thiz.sendBufferMutex.unlock();

    if (sem_post(&thiz.sendBufferSemaphore) < 0)
    {
        CONNECT_LOG(-1, "%s %s", "sem_post", strerror(errno));
    }
}
//------------------------------------------------------------------------------
void Connection::Recv(const BufferPtr::element_type& buffer)
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
unsigned int Connection::GetActiveThreadCount()
{
    return Connection::activeThreadCount;
}
//------------------------------------------------------------------------------
