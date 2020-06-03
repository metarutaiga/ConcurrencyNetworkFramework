//==============================================================================
// ConcurrencyNetworkFramework : Listen Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
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
    thiz.addrinfo = nullptr;
    thiz.thread = nullptr;

    pthread_attr_t attr;
    ::pthread_attr_init(&attr);
    ::pthread_attr_setstacksize(&attr, 65536);

    ::pthread_create(&thiz.thread, &attr, ProcedureThread, this);
}
//------------------------------------------------------------------------------
Listen::~Listen()
{
    thiz.terminate = true;

    if (thiz.socket > 0)
    {
        ::close(thiz.socket);
    }
    if (thiz.address)
    {
        free(thiz.address);
    }
    if (thiz.port)
    {
        free(thiz.port);
    }
    if (thiz.thread)
    {
        ::pthread_join(thiz.thread, nullptr);
    }
    if (thiz.addrinfo)
    {
        ::freeaddrinfo(thiz.addrinfo);
    }

    std::vector<Connect*> connectLocal;
    thiz.connectMutex.lock();
    thiz.connectArray.swap(connectLocal);
    thiz.connectMutex.unlock();
    for (Connect* connect : connectLocal)
    {
        delete connect;
    }
}
//------------------------------------------------------------------------------
void Listen::Procedure()
{
    thiz.terminate = true;

    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int error = ::getaddrinfo(thiz.address, thiz.port, &hints, &thiz.addrinfo);
    if (thiz.addrinfo == nullptr)
    {
        Log::Format(-1, "Socket %d : %s %s", thiz.socket, "getaddrinfo", gai_strerror(error));
        return;
    }

    thiz.socket = ::socket(thiz.addrinfo->ai_family, thiz.addrinfo->ai_socktype, thiz.addrinfo->ai_protocol);
    if (thiz.socket <= 0)
    {
        Log::Format(-1, "Socket %d : %s %s", thiz.socket, "socket", strerror(errno));
        return;
    }

    if (::bind(thiz.socket, thiz.addrinfo->ai_addr, thiz.addrinfo->ai_addrlen) != 0)
    {
        Log::Format(-1, "Socket %d : %s %s", thiz.socket, "bind", strerror(errno));
        return;
    }

    if (::listen(thiz.socket, thiz.backlog) != 0)
    {
        Log::Format(-1, "Socket %d : %s %s", thiz.socket, "listen", strerror(errno));
        return;
    }

    thiz.terminate = false;

    while (thiz.terminate == false)
    {
        struct sockaddr_storage addr = {};
        socklen_t size = 0;
        int id = ::accept(thiz.socket, (struct sockaddr*)&addr, &size);
        if (id <= 0)
        {
            Log::Format(-1, "Socket %d : %s %s", thiz.socket, "accept", strerror(errno));
            break;
        }

        Connect* connect = CreateConnect(id, addr);
        if (connect == nullptr)
        {
            ::close(id);
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
Connect* Listen::CreateConnect(int socket, const struct sockaddr_storage& addr)
{
    return new Connect(socket, addr);
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
