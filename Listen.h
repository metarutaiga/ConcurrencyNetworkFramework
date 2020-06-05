//==============================================================================
// ConcurrencyNetworkFramework : Listen Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"

class Connect;
class Listen
{
    volatile bool terminate;

    int socket;
    int backlog;
    char* address;
    char* port;
    pthread_t thread;

    std::vector<Connect*> connectArray;
    std::mutex connectMutex;

private:
    virtual void Procedure();
    static void* ProcedureThread(void* arg);

public:
    Listen(const char* address, const char* port, int backlog = 128);
    virtual ~Listen();

    virtual bool Start();
    virtual void Stop();

    virtual Connect* CreateConnect(int socket, const struct sockaddr_storage& addr);
    virtual void AttachConnect(Connect* connect);
    virtual void DetachConnect(Connect* connect);
};
