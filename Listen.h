//==============================================================================
// ConcurrencyNetworkFramework : Listen Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include <mutex>
#include <vector>
#include <pthread.h>

class Connect;
class Listen
{
    int socket;
    char* address;
    char* port;
    struct addrinfo* addrinfo;
    pthread_t thread;

    volatile bool terminate;

    std::vector<Connect*> connectArray;
    std::mutex connectMutex;

private:
    virtual void Procedure();
    static void* ProcedureThread(void* arg);

public:
    Listen(const char* address, const char* port);
    virtual ~Listen();

    void AttachConnect(Connect* connect);
    void DetachConnect(Connect* connect);
};
