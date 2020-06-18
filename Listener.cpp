//==============================================================================
// ConcurrencyNetworkFramework : Listener Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "Connection.h"
#include "Log.h"
#include "Socket.h"
#include "Listener.h"

#define LISTEN_LOG(level, format, ...) \
    Log::Format(level, "%s %d (%s:%s) : " format, "Listen", thiz.socket, thiz.address, thiz.port, __VA_ARGS__)

//------------------------------------------------------------------------------
Listener::Listener(const char* address, const char* port, int backlog)
{
    thiz.terminate = false;
    thiz.socket = 0;
    thiz.backlog = backlog;
    thiz.address = address ? ::strdup(address) : nullptr;
    thiz.port = port ? ::strdup(port) : ::strdup("7777");
    thiz.threadListen = pthread_t();
#if defined(__APPLE__)
    signal(SIGPIPE, SIG_IGN);
#endif
}
//------------------------------------------------------------------------------
Listener::~Listener()
{
    Stop();

    if (thiz.address)
    {
        ::free(thiz.address);
        thiz.address = nullptr;
    }
    if (thiz.port)
    {
        ::free(thiz.port);
        thiz.port = nullptr;
    }
}
//------------------------------------------------------------------------------
void Listener::ProcedureListen()
{
    std::vector<Connection*> connectionLocal;

    while (thiz.terminate == false)
    {
        struct sockaddr_storage addr = {};
        socklen_t size = sizeof(addr);
        int id = Socket::accept(thiz.socket, (struct sockaddr*)&addr, &size);
        if (id <= 0)
        {
            LISTEN_LOG(-1, "%s %s", "accept", Socket::strerror(Socket::errno));
            break;
        }
        char* address = nullptr;
        char* port = nullptr;
        Connection::GetAddressPort(addr, address, port);
        LISTEN_LOG(0, "%s %d (%s:%s)", "accept", id, address, port);
        ::free(address);
        ::free(port);

        Connection* connection = CreateConnection(id, addr);
        if (connection == nullptr)
        {
            Socket::close(id);
            continue;
        }
        if (connection->ConnectTCP() == false)
        {
            connection->Disconnect();
            continue;
        }

        connectionLocal.clear();
        thiz.connectionMutex.lock();
        for (Connection* connection : thiz.connectionArray)
        {
            if (connection == nullptr)
                continue;
            if (connection->Alive() == false)
            {
                connection->Disconnect();
                continue;
            }
            connectionLocal.emplace_back(connection);
        }
        thiz.connectionArray.swap(connectionLocal);
        thiz.connectionArray.emplace_back(connection);
        thiz.connectionMutex.unlock();
    }

    thiz.terminate = true;
}
//------------------------------------------------------------------------------
bool Listener::Start()
{
    struct addrinfo* addrinfo = nullptr;
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int error = ::getaddrinfo(thiz.address, thiz.port, &hints, &addrinfo);
    if (addrinfo == nullptr)
    {
        LISTEN_LOG(-1, "%s %s", "getaddrinfo", ::gai_strerror(error));
        return false;
    }

    thiz.socket = Socket::socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (thiz.socket <= 0)
    {
        LISTEN_LOG(-1, "%s %s", "socket", Socket::strerror(Socket::errno));
        ::freeaddrinfo(addrinfo);
        return false;
    }

    int enable = 1;
    Socket::setsockopt(thiz.socket, SOL_SOCKET, SO_REUSEADDR, (void*)&enable, sizeof(enable));
    Socket::setsockopt(thiz.socket, SOL_SOCKET, SO_REUSEPORT, (void*)&enable, sizeof(enable));

    if (Socket::bind(thiz.socket, addrinfo->ai_addr, addrinfo->ai_addrlen) != 0)
    {
        LISTEN_LOG(-1, "%s %s", "bind", Socket::strerror(Socket::errno));
        ::freeaddrinfo(addrinfo);
        return false;
    }
    ::freeaddrinfo(addrinfo);

    int fastOpen = 5;
    Socket::setsockopt(thiz.socket, SOL_TCP, TCP_FASTOPEN, &fastOpen, sizeof(fastOpen));

    if (Socket::listen(thiz.socket, thiz.backlog) != 0)
    {
        LISTEN_LOG(-1, "%s %s", "listen", Socket::strerror(Socket::errno));
        return false;
    }

    pthread_attr_t attr;
    ::pthread_attr_init(&attr);
    ::pthread_attr_setstacksize(&attr, 65536);

    ::pthread_create(&thiz.threadListen, &attr, [](void* arg) -> void*
    {
        Listener& listen = *(Listener*)arg;
        listen.ProcedureListen();
        return nullptr;
    }, this);

    ::pthread_attr_destroy(&attr);
    if (thiz.threadListen == pthread_t())
    {
        LISTEN_LOG(-1, "%s %s", "thread", ::strerror(errno));
        return false;
    }

    LISTEN_LOG(0, "%s %s", "listen", "ready");
    return true;
}
//------------------------------------------------------------------------------
void Listener::Stop()
{
    thiz.terminate = true;

    if (thiz.socket > 0)
    {
        Socket::close(thiz.socket);
        thiz.socket = 0;
    }
    if (thiz.threadListen)
    {
        ::pthread_join(thiz.threadListen, nullptr);
        thiz.threadListen = pthread_t();
    }

    std::vector<Connection*> connectionLocal;
    thiz.connectionMutex.lock();
    thiz.connectionArray.swap(connectionLocal);
    thiz.connectionMutex.unlock();
    for (Connection* connection : connectionLocal)
    {
        if (connection == nullptr)
            continue;
        connection->Disconnect();
    }
}
//------------------------------------------------------------------------------
Connection* Listener::CreateConnection(int socket, const struct sockaddr_storage& addr)
{
    return new Connection(socket, thiz.address, thiz.port, addr);
}
//------------------------------------------------------------------------------
