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

    int socketTCP;
    int socketUDP;
    char* sourceAddress;
    char* sourcePort;
    char* destinationAddress;
    char* destinationPort;
    pthread_t threadRecvTCP;
    pthread_t threadSendTCP;
    pthread_t threadRecvUDP;
    pthread_t threadSendUDP;

    std::vector<BufferPtr> sendBuffer;
    std::mutex sendBufferMutex;
    sem_t sendBufferSemaphoreTCP;
    sem_t sendBufferSemaphoreUDP;

    static std::atomic_uint activeThreadCount;

protected:
    virtual void ProcedureRecvTCP();
    virtual void ProcedureSendTCP();
    virtual void ProcedureRecvUDP();
    virtual void ProcedureSendUDP();

protected:
    virtual ~Connection();

public:
    Connection(int socket, const char* address, const char* port, const struct sockaddr_storage& addr);
    Connection(const char* address, const char* port);

    virtual bool Connect();
    virtual bool ConnectUDP();
    virtual bool Alive();
    virtual void Disconnect();

    virtual void Send(const BufferPtr& bufferPtr);
    virtual void Recv(const BufferPtr::element_type& buffer);

    static void GetAddressPort(const struct sockaddr_storage& addr, char*& address, char*& port);
    static int SetAddressPort(struct sockaddr_storage& addr, const char* address, const char* port);
    static unsigned int GetActiveThreadCount();
};
