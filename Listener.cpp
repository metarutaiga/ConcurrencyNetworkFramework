//==============================================================================
// ConcurrencyNetworkFramework : Listen Source
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
#include "Listener.h"

#define LISTEN_LOG(level, format, ...) \
    Log::Format(level, "%s %d (%s:%s) : " format, "Listen", thiz.socket, thiz.address, thiz.port, __VA_ARGS__)

//------------------------------------------------------------------------------
Listener::Listener(const char* address, const char* port, int backlog)
{
    thiz.terminate = false;
    thiz.socket = 0;
    thiz.backlog = backlog;
    thiz.address = address ? strdup(address) : nullptr;
    thiz.port = port ? strdup(port) : strdup("7777");
    thiz.threadListen = nullptr;
    thiz.threadCheck = nullptr;
}
//------------------------------------------------------------------------------
Listener::~Listener()
{
    Stop();

    if (thiz.address)
    {
        free(thiz.address);
        thiz.address = nullptr;
    }
    if (thiz.port)
    {
        free(thiz.port);
        thiz.port = nullptr;
    }
}
//------------------------------------------------------------------------------
void Listener::ProcedureListen()
{
    std::vector<Connection*> connectLocal;

    while (thiz.terminate == false)
    {
        struct sockaddr_storage addr = {};
        socklen_t size = sizeof(addr);
        int id = ::accept(thiz.socket, (struct sockaddr*)&addr, &size);
        if (id <= 0)
        {
            LISTEN_LOG(-1, "%s %s", "accept", strerror(errno));
            break;
        }
        char* address = nullptr;
        char* port = nullptr;
        Connection::GetAddressPort(addr, address, port);
        LISTEN_LOG(0, "%s %d (%s:%s)", "accept", id, address, port);
        free(address);
        free(port);

        Connection* connect = CreateConnection(id, addr);
        if (connect == nullptr)
        {
            ::close(id);
            continue;
        }
        if (connect->Connect() == false)
        {
            connect->Disconnect();
            continue;
        }

        connectLocal.clear();
        thiz.connectMutex.lock();
        for (auto it = thiz.connectArray.begin(); it != thiz.connectArray.end(); ++it)
        {
            Connection* connect = (*it);
            if (connect->Alive() == false)
            {
                connect->Disconnect();
                continue;
            }
            connectLocal.emplace_back(connect);
        }
        thiz.connectArray.swap(connectLocal);
        thiz.connectArray.emplace_back(connect);
        thiz.connectMutex.unlock();
    }

    thiz.terminate = true;
}
//------------------------------------------------------------------------------
void* Listener::ProcedureListenThread(void* arg)
{
    Listener& listen = *(Listener*)arg;
    listen.ProcedureListen();
    return nullptr;
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
        LISTEN_LOG(-1, "%s %s", "getaddrinfo", gai_strerror(error));
        return false;
    }

    thiz.socket = ::socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (thiz.socket <= 0)
    {
        LISTEN_LOG(-1, "%s %s", "socket", strerror(errno));
        freeaddrinfo(addrinfo);
        return false;
    }

    int enable = 1;
    ::setsockopt(thiz.socket, SOL_SOCKET, SO_REUSEADDR, (void*)&enable, sizeof(enable));
    ::setsockopt(thiz.socket, SOL_SOCKET, SO_REUSEPORT, (void*)&enable, sizeof(enable));

    if (::bind(thiz.socket, addrinfo->ai_addr, addrinfo->ai_addrlen) != 0)
    {
        LISTEN_LOG(-1, "%s %s", "bind", strerror(errno));
        freeaddrinfo(addrinfo);
        return false;
    }
    freeaddrinfo(addrinfo);

    int fastOpen = 5;
    ::setsockopt(thiz.socket, SOL_TCP, TCP_FASTOPEN, &fastOpen, sizeof(fastOpen));

    if (::listen(thiz.socket, thiz.backlog) != 0)
    {
        LISTEN_LOG(-1, "%s %s", "listen", strerror(errno));
        return false;
    }

    pthread_attr_t attr;
    ::pthread_attr_init(&attr);
    ::pthread_attr_setstacksize(&attr, 65536);

    ::pthread_create(&thiz.threadListen, &attr, ProcedureListenThread, this);

    ::pthread_attr_destroy(&attr);
    if (thiz.threadListen == nullptr)
    {
        LISTEN_LOG(-1, "%s %s", "thread", strerror(errno));
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
        ::close(thiz.socket);
        thiz.socket = 0;
    }
    if (thiz.threadListen)
    {
        ::pthread_join(thiz.threadListen, nullptr);
        thiz.threadListen = nullptr;
    }

    std::vector<Connection*> connectLocal;
    thiz.connectMutex.lock();
    thiz.connectArray.swap(connectLocal);
    thiz.connectMutex.unlock();
    for (Connection* connect : connectLocal)
    {
        connect->Disconnect();
    }
}
//------------------------------------------------------------------------------
Connection* Listener::CreateConnection(int socket, const struct sockaddr_storage& addr)
{
    return new Connection(socket, thiz.address, thiz.port, addr);
}
//------------------------------------------------------------------------------