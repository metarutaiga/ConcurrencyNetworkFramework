//==============================================================================
// ConcurrencyNetworkFramework : Listener Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"

class Connection;
class Listener
{
protected:
    volatile bool terminate;

    int socket;
    int backlog;
    char* address;
    char* port;
    std::thread* threadListen;

    std::vector<Connection*> connectionArray;
    std::mutex connectionMutex;

protected:
    virtual void ProcedureListen();

public:
    Listener(const char* address, const char* port, int backlog = 128);
    virtual ~Listener();

    virtual bool Start();
    virtual void Stop();

    virtual Connection* CreateConnection(int socket, const struct sockaddr_storage& addr);
};
