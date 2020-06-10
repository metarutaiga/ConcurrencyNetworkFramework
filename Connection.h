//==============================================================================
// ConcurrencyNetworkFramework : Connection Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"

typedef std::shared_ptr<std::vector<char>> BufferPtr;

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

    std::vector<BufferPtr> sendBuffer;
    std::mutex sendBufferMutex;
    sem_t sendBufferSemaphore;

    static std::atomic_uint activeThreadCount;

protected:
    virtual void ProcedureRecv();
    virtual void ProcedureSend();

protected:
    virtual ~Connection();

public:
    Connection(int socket, const char* address, const char* port, const struct sockaddr_storage& addr);
    Connection(const char* address, const char* port);

    virtual bool Connect();
    virtual bool Alive();
    virtual void Disconnect();

    virtual void Send(const BufferPtr& bufferPtr);
    virtual void Recv(const BufferPtr::element_type& buffer);

    static void GetAddressPort(const struct sockaddr_storage& addr, char*& address, char*& port);
    static unsigned int GetActiveThreadCount();
};
