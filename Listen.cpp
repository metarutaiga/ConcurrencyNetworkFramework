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
#include "Listen.h"

//------------------------------------------------------------------------------
Listen::Listen(const char* address, const char* port, int backlog)
{
    (*this).socket = 0;
    (*this).backlog = backlog;
    (*this).address = address ? strdup(address) : nullptr;
    (*this).port = port ? strdup(port) : strdup("7777");
    (*this).addrinfo = nullptr;
    (*this).thread = nullptr;
    ::pthread_create(&thread, nullptr, ProcedureThread, this);
    (*this).terminate = false;
}
//------------------------------------------------------------------------------
Listen::~Listen()
{
    terminate = true;

    if (socket > 0)
    {
        ::close(socket);
    }
    if (address)
    {
        free(address);
    }
    if (port)
    {
        free(port);
    }
    if (thread)
    {
        ::pthread_join(thread, nullptr);
    }
    if (addrinfo)
    {
        ::freeaddrinfo(addrinfo);
    }

    std::vector<Connect*> connectLocal;
    connectMutex.lock();
    connectArray.swap(connectLocal);
    connectMutex.unlock();
    for (Connect* connect : connectLocal)
        delete connect;
}
//------------------------------------------------------------------------------
void Listen::Procedure()
{
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    ::getaddrinfo(address, port, &hints, &addrinfo);
    if (addrinfo == nullptr)
        return;

    socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket <= 0)
        return;

    if (::bind(socket, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0)
        return;

    if (::listen(socket, backlog) < 0)
        return;

    while (terminate == false)
    {
        struct sockaddr_storage addr = {};
        socklen_t size = 0;
        int id = ::accept(socket, (struct sockaddr*)&addr, &size);
        if (id < 0)
            return;

        Connect* connect = CreateConnect(id, addr);
        if (connect == nullptr)
        {
            ::close(id);
            continue;
        }
        AttachConnect(connect);
    }

    terminate = true;
}
//------------------------------------------------------------------------------
void* Listen::ProcedureThread(void* arg)
{
    Listen& listen = *(Listen*)arg;
    listen.Procedure();
    return nullptr;
}
//------------------------------------------------------------------------------
Connect* Listen::CreateConnect(int socket, const sockaddr_storage& addr)
{
    return new Connect(socket, addr);
}
//------------------------------------------------------------------------------
void Listen::AttachConnect(Connect* connect)
{
    connectMutex.lock();
    connectArray.emplace_back(connect);
    connectMutex.unlock();
}
//------------------------------------------------------------------------------
void Listen::DetachConnect(Connect* connect)
{
    connectMutex.lock();
    auto it = std::find(connectArray.begin(), connectArray.end(), connect);
    if (it != connectArray.end())
    {
        (*it) = connectArray.back();
        connectArray.pop_back();
    }
    connectMutex.unlock();
}
//------------------------------------------------------------------------------
