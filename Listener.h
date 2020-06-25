//==============================================================================
// ConcurrencyNetworkFramework : Listener Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "Base.h"

class FRAMEWORK_API Connection;
class FRAMEWORK_API Listener : public Base
{
protected:
    std::vector<socket_t> socket;
    std::vector<std::thread> threadListen;

    std::vector<Connection*> connectionArray;
    std::mutex connectionMutex;

    char* address;
    char* port;
    int backlog;

protected:
    virtual void ProcedureListen(socket_t socket);

public:
    Listener(const char* address, const char* port, int backlog = 128);
    virtual ~Listener();

    virtual bool Start(size_t count);
    virtual void Stop();

    virtual Connection* CreateConnection(socket_t socket, const struct sockaddr_storage& addr);
};
