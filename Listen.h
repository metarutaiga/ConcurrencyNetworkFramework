//==============================================================================
// ConcurrencyNetworkFramework : Listen Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include <pthread.h>
#include <sys/socket.h>
#include <mutex>
#include <vector>

class Connect;
class Listen
{
    int socket;
    int backlog;
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
    Listen(const char* address, const char* port, int backlog = 128);
    virtual ~Listen();

    virtual Connect* CreateConnect(int socket, const sockaddr_storage& addr);
    virtual void AttachConnect(Connect* connect);
    virtual void DetachConnect(Connect* connect);
};
