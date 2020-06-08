//==============================================================================
// ConcurrencyNetworkFramework : Connection Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"

class Listener;
class Connection
{
protected:
    volatile bool terminate;

    int socket;
    char* sourceAddress;
    char* sourcePort;
    char* destinationAddress;
    char* destinationPort;
    pthread_t threadRecv;
    pthread_t threadSend;

    std::vector<std::vector<char>> sendBuffer;
    std::mutex sendBufferMutex;
    sem_t sendBufferSemaphore;

    static int activeThreadCount;

protected:
    virtual void ProcedureRecv();
    virtual void ProcedureSend();
    static void* ProcedureRecvThread(void* arg);
    static void* ProcedureSendThread(void* arg);

protected:
    virtual ~Connection();

public:
    Connection(int socket, const char* address, const char* port, const struct sockaddr_storage& addr);
    Connection(const char* address, const char* port);

    virtual bool Connect();
    virtual bool Alive();
    virtual void Disconnect();

    virtual void Send(std::vector<char>&& buffer);
    virtual void Recv(const std::vector<char>& buffer);

    static void GetAddressPort(const struct sockaddr_storage& addr, char*& address, char*& port);
    static int GetActiveThreadCount();
};
