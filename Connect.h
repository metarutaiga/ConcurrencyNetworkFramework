//==============================================================================
// ConcurrencyNetworkFramework : Connect Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <mutex>
#include <vector>

class Connect
{
    int socket;
    struct sockaddr_storage addr;
    pthread_t threadRecv;
    pthread_t threadSend;

    volatile bool terminate;

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
    Connect(int socket, const sockaddr_storage& addr);
    virtual ~Connect();

    virtual void Enqueue(std::vector<char>&& buffer);
    virtual void Dequeue(const std::vector<char>& buffer) const;
};
