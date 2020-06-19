//==============================================================================
// ConcurrencyNetworkFramework : Connection Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"
#include "Buffer.h"

class Connection
{
protected:
    volatile bool terminate;

    bool readyTCP;
    bool readyUDP;
    int socketTCP;
    int socketUDP;

    std::vector<Buffer> sendBufferTCP;
    std::vector<Buffer> sendBufferUDP;
    std::mutex sendBufferMutexTCP;
    std::mutex sendBufferMutexUDP;
    std::counting_semaphore sendBufferSemaphoreTCP;
    std::counting_semaphore sendBufferSemaphoreUDP;

    char* sourceAddress;
    char* sourcePort;
    char* destinationAddress;
    char* destinationPort;

    std::thread threadRecvTCP;
    std::thread threadSendTCP;
    std::thread threadRecvUDP;
    std::thread threadSendUDP;

    static std::atomic_int activeThreadCount[4];

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

    virtual bool ConnectTCP();
    virtual bool ConnectUDP();
    virtual bool Alive();
    virtual void Disconnect();

    virtual void Send(const Buffer& buffer);
    virtual void Recv(const Buffer::element_type& buffer);

    static void GetAddressPort(const struct sockaddr_storage& addr, char*& address, char*& port);
    static int SetAddressPort(struct sockaddr_storage& addr, const char* address, const char* port);
    static int GetActiveThreadCount(int index);
};
