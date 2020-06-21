//==============================================================================
// ConcurrencyNetworkFramework : Listener Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "Base.h"

class Connection;
class Listener : public Base
{
protected:
    std::vector<int> socket;
    std::vector<std::thread> threadListen;

    std::vector<Connection*> connectionArray;
    std::mutex connectionMutex;

    char* address;
    char* port;
    int backlog;

protected:
    virtual void ProcedureListen(int socket);

public:
    Listener(const char* address, const char* port, int backlog = 128);
    virtual ~Listener();

    virtual bool Start(size_t count);
    virtual void Stop();

    virtual Connection* CreateConnection(int socket, const struct sockaddr_storage& addr);
};
