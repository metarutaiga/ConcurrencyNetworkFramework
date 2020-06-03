//==============================================================================
// ConcurrencyNetworkFramework : Connect Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <unistd.h>
#include "Log.h"
#include "Connect.h"

//------------------------------------------------------------------------------
Connect::Connect(int socket, const struct sockaddr_storage& addr)
{
    thiz.terminate = false;
    thiz.socket = socket;
    thiz.addr = addr;
    thiz.threadRecv = nullptr;
    thiz.threadSend = nullptr;
    thiz.sendBufferAvailable = 0;
    thiz.sendBufferSemaphore = ::sem_open("", O_CREAT);

    pthread_attr_t attr;
    ::pthread_attr_init(&attr);
    ::pthread_attr_setstacksize(&attr, 65536);

    ::pthread_create(&thiz.threadRecv, &attr, ProcedureRecvThread, this);
    ::pthread_create(&thiz.threadSend, &attr, ProcedureSendThread, this);
}
//------------------------------------------------------------------------------
Connect::~Connect()
{
    thiz.terminate = true;

    if (thiz.socket > 0)
    {
        ::close(thiz.socket);
    }
    if (thiz.sendBufferSemaphore)
    {
        ::sem_post(thiz.sendBufferSemaphore);
    }
    if (thiz.threadRecv)
    {
        ::pthread_join(thiz.threadRecv, nullptr);
    }
    if (thiz.threadSend)
    {
        ::pthread_join(thiz.threadSend, nullptr);
    }
    if (thiz.sendBufferSemaphore)
    {
        ::sem_close(thiz.sendBufferSemaphore);
    }
}
//------------------------------------------------------------------------------
void Connect::ProcedureRecv()
{
    std::vector<char> buffer;

    thiz.terminate = false;

    while (thiz.terminate == false)
    {
        unsigned short size = 0;
        if (::recv(thiz.socket, &size, sizeof(short), MSG_WAITALL) <= 0)
        {
            Log::Format(-1, "Socket %d : %s %s", thiz.socket, "recv", strerror(errno));
            break;
        }
        buffer.resize(size);
        if (::recv(thiz.socket, &buffer.front(), size, MSG_WAITALL) <= 0)
        {
            Log::Format(-1, "Socket %d : %s %s", thiz.socket, "recv", strerror(errno));
            break;
        }

        Recv(buffer);
    }

    thiz.terminate = true;
}
//------------------------------------------------------------------------------
void Connect::ProcedureSend()
{
    thiz.terminate = false;

    while (thiz.terminate == false)
    {
        sem_wait(thiz.sendBufferSemaphore);

        for (int available = 1; available > 0; available = thiz.sendBufferAvailable.fetch_sub(1, std::memory_order_acq_rel) - 1)
        {
            std::vector<char> buffer;
            thiz.sendBufferMutex.lock();
            if (thiz.sendBuffer.empty() == false)
            {
                thiz.sendBuffer.front().swap(buffer);
                thiz.sendBuffer.erase(thiz.sendBuffer.begin());
            }
            thiz.sendBufferMutex.unlock();
            if (buffer.empty())
                continue;

            unsigned short size = (short)buffer.size();
            if (::send(thiz.socket, &size, sizeof(short), MSG_WAITALL) <= 0)
            {
                Log::Format(-1, "Socket %d : %s %s", thiz.socket, "send", strerror(errno));
                break;
            }
            if (::send(thiz.socket, &buffer.front(), size, MSG_WAITALL) <= 0)
            {
                Log::Format(-1, "Socket %d : %s %s", thiz.socket, "send", strerror(errno));
                break;
            }
        }
    }

    thiz.terminate = true;
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
    thiz.sendBufferMutex.lock();
    thiz.sendBuffer.emplace_back(buffer);
    thiz.sendBufferMutex.unlock();

    int available = thiz.sendBufferAvailable.fetch_add(1, std::memory_order_acq_rel) + 1;
    if (available <= 1)
    {
        thiz.sendBufferAvailable.fetch_add(1, std::memory_order_acq_rel);
        sem_post(thiz.sendBufferSemaphore);
    }
}
//------------------------------------------------------------------------------
void Connect::Recv(const std::vector<char>& buffer) const
{
    
}
//------------------------------------------------------------------------------
