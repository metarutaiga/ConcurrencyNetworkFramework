//==============================================================================
// ConcurrencyNetworkFramework : Connection Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "Base.h"
#include "Buffer.h"

class FRAMEWORK_API Connection : public Base
{
protected:
    bool readyTCP;
    bool readyUDP;
    socket_t socketTCP;
    socket_t socketUDP;

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
    virtual void Terminate() override;
    virtual void ProcedureRecvTCP();
    virtual void ProcedureSendTCP();
    virtual void ProcedureRecvUDP();
    virtual void ProcedureSendUDP();

protected:
    virtual ~Connection();

public:
    enum Mode
    {
        MODE_UDP    = 0x0,
        MODE_TCP    = 0x1,
        MODE_AUTO   = 0xF,
    };

    // For IPv4, EMTU_S is the smaller of 576 bytes and the first-hop MTU [RFC1122].
    // For IPv6, EMTU_S is 1280 bytes [RFC2460].
    enum { UDP_SIZE_MAX = 1280 };

public:
    Connection(socket_t socket, const char* address, const char* port, const struct sockaddr_storage& addr);
    Connection(const char* address, const char* port);

    virtual bool ConnectTCP();
    virtual bool ConnectUDP();
    virtual bool Alive();
    virtual void Disconnect();

    virtual void Send(const Buffer& sendBuffer, int mode = MODE_AUTO);
    virtual void Recv(const Buffer& recvBuffer, int mode = MODE_AUTO);

    virtual void ProcessSendTCP(const Buffer::element_type& source, Buffer::element_type& destination);
    virtual void ProcessRecvTCP(const Buffer::element_type& source, Buffer::element_type& destination);
    virtual void ProcessSendUDP(const Buffer::element_type& source, Buffer::element_type& destination);
    virtual void ProcessRecvUDP(const Buffer::element_type& source, Buffer::element_type& destination);

    static void GetAddressPort(const struct sockaddr_storage& addr, char*& address, char*& port);
    static int SetAddressPort(struct sockaddr_storage& addr, const char* address, const char* port);
    static int GetActiveThreadCount(int index);
};
