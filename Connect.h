//==============================================================================
// ConcurrencyNetworkFramework : Connect Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"

class Connect
{
    volatile bool terminate;

    int socket;
    struct sockaddr_storage addr;
    pthread_t threadRecv;
    pthread_t threadSend;

    std::vector<std::vector<char>> sendBuffer;
    std::mutex sendBufferMutex;
    int sendBufferAvailable;
    sem_t* sendBufferSemaphore;

private:
    virtual void ProcedureRecv();
    virtual void ProcedureSend();
    static void* ProcedureRecvThread(void* arg);
    static void* ProcedureSendThread(void* arg);

public:
    Connect(int socket, const struct sockaddr_storage& addr);
    virtual ~Connect();

    virtual bool Start();
    virtual void Stop();

    virtual void Send(std::vector<char>&& buffer);
    virtual void Recv(const std::vector<char>& buffer) const;
};
