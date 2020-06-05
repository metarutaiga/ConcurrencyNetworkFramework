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
    pthread_t threadListen;
    pthread_t threadCheck;

    std::vector<Connect*> connectArray;
    std::mutex connectMutex;

private:
    virtual void ProcedureListen();
    virtual void ProcedureCheck();
    static void* ProcedureListenThread(void* arg);
    static void* ProcedureCheckThread(void* arg);

public:
    Listen(const char* address, const char* port, int backlog = 128);
    virtual ~Listen();

    virtual bool Start();
    virtual void Stop();

    virtual Connect* CreateConnect(int socket, const struct sockaddr_storage& addr);
};
