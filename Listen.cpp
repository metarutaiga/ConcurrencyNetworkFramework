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
#include "Connect.h"
#include "Log.h"
#include "Listen.h"

//------------------------------------------------------------------------------
Listen::Listen(const char* address, const char* port, int backlog)
{
    thiz.terminate = false;
    thiz.socket = 0;
    thiz.backlog = backlog;
    thiz.address = address ? strdup(address) : nullptr;
    thiz.port = port ? strdup(port) : strdup("7777");
    thiz.thread = nullptr;
}
//------------------------------------------------------------------------------
Listen::~Listen()
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
void Listen::Procedure()
{
    thiz.terminate = false;

    while (thiz.terminate == false)
    {
        struct sockaddr_storage addr = {};
        socklen_t size = sizeof(addr);
        int id = ::accept(thiz.socket, (struct sockaddr*)&addr, &size);
        if (id <= 0)
        {
            Log::Format(-1, "%s %d (%s:%s) : %s %s", "Listen", thiz.socket, thiz.address, thiz.port, "accept", strerror(errno));
            break;
        }
        char* address = nullptr;
        char* port = nullptr;
        Connect::GetAddressPort(addr, address, port);
        Log::Format(0, "%s %d (%s:%s) : %s %d (%s:%s)", "Listen", thiz.socket, thiz.address, thiz.port, "accept", id, address, port);
        free(address);
        free(port);

        Connect* connect = CreateConnect(id, addr);
        if (connect == nullptr)
        {
            ::close(id);
            continue;
        }
        if (connect->Start() == false)
        {
            connect->Stop();
            delete connect;
            continue;
        }
        AttachConnect(connect);
    }

    thiz.terminate = true;
}
//------------------------------------------------------------------------------
void* Listen::ProcedureThread(void* arg)
{
    Listen& listen = *(Listen*)arg;
    listen.Procedure();
    return nullptr;
}
//------------------------------------------------------------------------------
bool Listen::Start()
{
    struct addrinfo* addrinfo = nullptr;
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int error = ::getaddrinfo(thiz.address, thiz.port, &hints, &addrinfo);
    if (addrinfo == nullptr)
    {
        Log::Format(-1, "%s %d (%s:%s) : %s %s", "Listen", thiz.socket, thiz.address, thiz.port, "getaddrinfo", gai_strerror(error));
        return false;
    }

    thiz.socket = ::socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (thiz.socket <= 0)
    {
        Log::Format(-1, "%s %d (%s:%s) : %s %s", "Listen", thiz.socket, thiz.address, thiz.port, "socket", strerror(errno));
        freeaddrinfo(addrinfo);
        return false;
    }

    int enable = 1;
    ::setsockopt(thiz.socket, SOL_SOCKET, SO_REUSEADDR, (void*)&enable, sizeof(enable));
    ::setsockopt(thiz.socket, SOL_SOCKET, SO_REUSEPORT, (void*)&enable, sizeof(enable));

    if (::bind(thiz.socket, addrinfo->ai_addr, addrinfo->ai_addrlen) != 0)
    {
        Log::Format(-1, "%s %d (%s:%s) : %s %s", "Listen", thiz.socket, thiz.address, thiz.port, "bind", strerror(errno));
        freeaddrinfo(addrinfo);
        return false;
    }
    freeaddrinfo(addrinfo);

    int fastOpen = 5;
    ::setsockopt(thiz.socket, SOL_TCP, TCP_FASTOPEN, &fastOpen, sizeof(fastOpen));

    if (::listen(thiz.socket, thiz.backlog) != 0)
    {
        Log::Format(-1, "%s %d (%s:%s) : %s %s", "Listen", thiz.socket, thiz.address, thiz.port, "listen", strerror(errno));
        return false;
    }

    pthread_attr_t attr;
    ::pthread_attr_init(&attr);
    ::pthread_attr_setstacksize(&attr, 65536);

    ::pthread_create(&thiz.thread, &attr, ProcedureThread, this);
    if (thiz.thread == nullptr)
    {
        Log::Format(-1, "%s %d (%s:%s) : %s %s", "Listen", thiz.socket, thiz.address, thiz.port, "thread", strerror(errno));
        return false;
    }

    Log::Format(0, "%s %d (%s:%s) : %s %s", "Listen", thiz.socket, thiz.address, thiz.port, "listen", "ready");
    return true;
}
//------------------------------------------------------------------------------
void Listen::Stop()
{
    thiz.terminate = true;

    if (thiz.socket > 0)
    {
        ::close(thiz.socket);
        thiz.socket = 0;
    }
    if (thiz.thread)
    {
        ::pthread_join(thiz.thread, nullptr);
        thiz.thread = nullptr;
    }

    std::vector<Connect*> connectLocal;
    thiz.connectMutex.lock();
    thiz.connectArray.swap(connectLocal);
    thiz.connectMutex.unlock();
    for (Connect* connect : connectLocal)
    {
        connect->Stop();
        delete connect;
    }
}
//------------------------------------------------------------------------------
Connect* Listen::CreateConnect(int socket, const struct sockaddr_storage& addr)
{
    return new Connect(socket, thiz.address, thiz.port, addr);
}
//------------------------------------------------------------------------------
void Listen::AttachConnect(Connect* connect)
{
    thiz.connectMutex.lock();
    thiz.connectArray.emplace_back(connect);
    thiz.connectMutex.unlock();
}
//------------------------------------------------------------------------------
void Listen::DetachConnect(Connect* connect)
{
    thiz.connectMutex.lock();
    auto it = std::find(thiz.connectArray.begin(), thiz.connectArray.end(), connect);
    if (it != thiz.connectArray.end())
    {
        (*it) = thiz.connectArray.back();
        thiz.connectArray.pop_back();
    }
    thiz.connectMutex.unlock();
}
//------------------------------------------------------------------------------
