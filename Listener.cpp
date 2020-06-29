//==============================================================================
// ConcurrencyNetworkFramework : Listener Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include "Connection.h"
#include "Log.h"
#include "Socket.h"
#include "Listener.h"
#if defined(__APPLE__) || defined(__unix__)
#   include <netdb.h>
#   include <netinet/in.h>
#   include <netinet/tcp.h>
#   if defined(__linux__)
#      include <linux/filter.h>
#   endif
#endif

#define LISTEN_LOG(level, format, ...) \
    Log::Format(Log::level, "%s %d (%s:%s) : " format, "Listen", socket, thiz.address, thiz.port, __VA_ARGS__)

//------------------------------------------------------------------------------
Listener::Listener(const char* address, const char* port, int backlog)
{
    thiz.address = address ? ::strdup(address) : ::strdup("0.0.0.0");
    thiz.port = port ? ::strdup(port) : ::strdup("7777");
    thiz.backlog = backlog;
}
//------------------------------------------------------------------------------
Listener::~Listener()
{
    Stop();

    if (thiz.address)
    {
        ::free(thiz.address);
        thiz.address = nullptr;
    }
    if (thiz.port)
    {
        ::free(thiz.port);
        thiz.port = nullptr;
    }
}
//------------------------------------------------------------------------------
void Listener::ProcedureListen(socket_t socket)
{
    std::vector<Connection*> connectionLocal;

    while (Base::Terminating() == false)
    {
        struct sockaddr_storage sockaddr = {};
        socklen_t sockaddrLength = sizeof(sockaddr);
        socket_t id = Socket::accept(socket, reinterpret_cast<struct sockaddr*>(&sockaddr), &sockaddrLength);
        if (id <= 0 || Base::Terminating())
        {
            LISTEN_LOG(ERROR, "%s %s", "accept", Socket::strerror(Socket::errno));
            break;
        }
        char* address = nullptr;
        char* port = nullptr;
        Connection::GetAddressPort(sockaddr, address, port);
        LISTEN_LOG(INFO, "%s %d (%s:%s)", "accept", id, address, port);
        ::free(address);
        ::free(port);

        Connection* connection = CreateConnection(id, sockaddr);
        if (connection == nullptr)
        {
            Socket::close(id);
            continue;
        }
        if (connection->ConnectTCP() == false)
        {
            connection->Disconnect();
            continue;
        }

        thiz.connectionMutex.lock();
        thiz.connectionArray.emplace_back(connection);
        thiz.connectionMutex.unlock();

        CheckAlive(connectionLocal);
    }
    Base::Terminate();
}
//------------------------------------------------------------------------------
bool Listener::Start(size_t count)
{
    struct sockaddr_storage sockaddr = {};
    socklen_t sockaddrLength = Connection::SetAddressPort(sockaddr, thiz.address, thiz.port);
    for (size_t i = 0; i < count; ++i)
    {
        socket_t socket = Socket::socket(sockaddr.ss_family, SOCK_STREAM, IPPROTO_TCP);
        if (socket <= 0)
        {
            LISTEN_LOG(ERROR, "%s %s", "socket", Socket::strerror(Socket::errno));
            continue;
        }

        int enable = 1;
        if (Socket::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        {
            LISTEN_LOG(ERROR, "%s %s", "setsockopt", "SO_REUSEADDR");
        }
        if (Socket::setsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) < 0)
        {
            LISTEN_LOG(ERROR, "%s %s", "setsockopt", "SO_REUSEPORT");
        }

#if defined(__linux__)
        struct sock_filter filter[] =
        {
            /* A = raw_smp_processor_id() */
            { BPF_LD  | BPF_W | BPF_ABS, 0, 0, (unsigned int)(SKF_AD_OFF + SKF_AD_CPU) },
            /* return A */
            { BPF_RET | BPF_A, 0, 0, 0 },
        };
        struct sock_fprog cbpf =
        {
            .len = 2,
            .filter = filter,
        };
        if (Socket::setsockopt(socket, SOL_SOCKET, SO_ATTACH_REUSEPORT_CBPF, &cbpf, sizeof(cbpf)) < 0)
        {
            LISTEN_LOG(ERROR, "%s %s", "setsockopt", "SO_ATTACH_REUSEPORT_CBPF");
        }
#endif

        if (Socket::bind(socket, reinterpret_cast<struct sockaddr*>(&sockaddr), sockaddrLength) < 0)
        {
            LISTEN_LOG(ERROR, "%s %s", "bind", Socket::strerror(Socket::errno));
            Socket::close(socket);
            continue;
        }

        if (Socket::listen(socket, thiz.backlog) < 0)
        {
            LISTEN_LOG(ERROR, "%s %s", "listen", Socket::strerror(Socket::errno));
            Socket::close(socket);
            continue;
        }

        thiz.threadListen.emplace_back(std::stacking_thread(65536, [this, socket]{ thiz.ProcedureListen(socket); }));
        if (thiz.threadListen.back().joinable() == false)
        {
            LISTEN_LOG(ERROR, "%s %s", "thread", ::strerror(errno));
            thiz.threadListen.pop_back();
            Socket::close(socket);
            continue;
        }
        thiz.socket.emplace_back(socket);

        LISTEN_LOG(INFO, "%s %s", "listen", "ready");
    }

    return true;
}
//------------------------------------------------------------------------------
void Listener::Stop()
{
    Base::Terminate();

    for (socket_t socket : thiz.socket)
    {
        if (socket > 0)
        {
            Socket::shutdown(socket, SHUT_RD);
            Socket::close(socket);
        }
    }
    thiz.socket.clear();
    for (std::thread& thread : thiz.threadListen)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    thiz.threadListen.clear();

    std::vector<Connection*> connectionLocal;
    thiz.connectionMutex.lock();
    thiz.connectionArray.swap(connectionLocal);
    thiz.connectionMutex.unlock();
    for (Connection* connection : connectionLocal)
    {
        if (connection == nullptr)
            continue;
        connection->Disconnect();
    }
}
//------------------------------------------------------------------------------
void Listener::CheckAlive(std::vector<Connection*>& connectionTemp)
{
    connectionTemp.clear();

    thiz.connectionMutex.lock();
    for (Connection* connection : thiz.connectionArray)
    {
        if (connection == nullptr)
            continue;
        if (connection->Alive() == false)
        {
            connection->Disconnect();
            continue;
        }
        connectionTemp.emplace_back(connection);
    }
    thiz.connectionArray.swap(connectionTemp);
    thiz.connectionMutex.unlock();
}
//------------------------------------------------------------------------------
Connection* Listener::CreateConnection(socket_t socket, const struct sockaddr_storage& addr)
{
    return new Connection(socket, thiz.address, thiz.port, addr);
}
//------------------------------------------------------------------------------
