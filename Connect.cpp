//==============================================================================
// ConcurrencyNetworkFramework : Connect Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <unistd.h>
#include "Connect.h"

//------------------------------------------------------------------------------
Connect::Connect(int socket, const sockaddr_storage& addr)
{
    (*this).socket = socket;
    (*this).addr = addr;
    ::pthread_create(&threadRecv, nullptr, ProcedureRecvThread, this);
    ::pthread_create(&threadSend, nullptr, ProcedureSendThread, this);
    (*this).terminate = false;
    (*this).sendBufferAvailable = 0;
    (*this).sendBufferSemaphore = sem_open("", O_CREAT);
}
//------------------------------------------------------------------------------
Connect::~Connect()
{
    terminate = true;

    if (socket > 0)
    {
        ::close(socket);
    }
    if (sendBufferSemaphore)
    {
        sem_post(sendBufferSemaphore);
    }
    if (threadRecv)
    {
        ::pthread_join(threadRecv, nullptr);
    }
    if (threadSend)
    {
        ::pthread_join(threadSend, nullptr);
    }
    if (sendBufferSemaphore)
    {
        sem_close(sendBufferSemaphore);
    }
}
//------------------------------------------------------------------------------
void Connect::ProcedureRecv()
{
    std::vector<char> buffer;

    while (terminate == false)
    {
        ssize_t result;

        unsigned short size = 0;
        result = ::recv(socket, &size, sizeof(short), MSG_WAITALL);
        if (result < 0)
            break;
        buffer.resize(size);
        result = ::recv(socket, &buffer.front(), size, MSG_WAITALL);
        if (result < 0)
            break;

        Recv(buffer);
    }

    terminate = true;
}
//------------------------------------------------------------------------------
void Connect::ProcedureSend()
{
    while (terminate == false)
    {
        sem_wait(sendBufferSemaphore);

        int available = 0;
        do
        {
            std::vector<char> buffer;
            sendBufferMutex.lock();
            if (sendBuffer.empty() == false)
            {
                sendBuffer.front().swap(buffer);
                sendBuffer.erase(sendBuffer.begin());
            }
            sendBufferMutex.unlock();
            if (buffer.empty())
                continue;
            ssize_t result;

            unsigned short size = (short)buffer.size();
            result = ::send(socket, &size, sizeof(short), MSG_WAITALL);
            if (result < 0)
                break;
            result = ::send(socket, &buffer.front(), size, MSG_WAITALL);
            if (result < 0)
                break;

            available = __sync_sub_and_fetch(&sendBufferAvailable, 1);
        } while (available > 0);
    }

    terminate = true;
}
//------------------------------------------------------------------------------
void* Connect::ProcedureRecvThread(void* arg)
{
    Connect& connect = *(Connect*)arg;
    connect.ProcedureRecv();
    return nullptr;
}
//------------------------------------------------------------------------------
void* Connect::ProcedureSendThread(void* arg)
{
    Connect& connect = *(Connect*)arg;
    connect.ProcedureSend();
    return nullptr;
}
//------------------------------------------------------------------------------
void Connect::Send(std::vector<char>&& buffer)
{
    sendBufferMutex.lock();
    sendBuffer.emplace_back(buffer);
    sendBufferMutex.unlock();

    int available = __sync_add_and_fetch(&sendBufferAvailable, 1);
    if (available <= 1)
    {
        available = 1;
        sem_post(sendBufferSemaphore);
    }
}
//------------------------------------------------------------------------------
void Connect::Recv(const std::vector<char>& buffer) const
{
    
}
//------------------------------------------------------------------------------
