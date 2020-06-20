//==============================================================================
// ConcurrencyNetworkFramework : Listener Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#if defined(__linux__)
#   include <linux/filter.h>
#endif
#include "Connection.h"
#include "Log.h"
#include "Socket.h"
#include "Listener.h"

#define LISTEN_LOG(level, format, ...) \
    Log::Format(level, "%s %d (%s:%s) : " format, "Listen", thiz.socket, thiz.address, thiz.port, __VA_ARGS__)

//------------------------------------------------------------------------------
Listener::Listener(const char* address, const char* port, int backlog)
{
    thiz.terminate = false;
    thiz.socket = 0;
    thiz.backlog = backlog;
    thiz.address = address ? ::strdup(address) : nullptr;
    thiz.port = port ? ::strdup(port) : ::strdup("7777");
#if defined(__APPLE__)
    signal(SIGPIPE, SIG_IGN);
#endif
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
void Listener::ProcedureListen()
{
    std::vector<Connection*> connectionLocal;

    while (thiz.terminate == false)
    {
        struct sockaddr_storage addr = {};
        socklen_t size = sizeof(addr);
        int id = Socket::accept(thiz.socket, (struct sockaddr*)&addr, &size);
        if (id <= 0 || thiz.terminate)
        {
            LISTEN_LOG(-1, "%s %s", "accept", Socket::strerror(Socket::errno));
            break;
        }
        char* address = nullptr;
        char* port = nullptr;
        Connection::GetAddressPort(addr, address, port);
        LISTEN_LOG(0, "%s %d (%s:%s)", "accept", id, address, port);
        ::free(address);
        ::free(port);

        Connection* connection = CreateConnection(id, addr);
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

        connectionLocal.clear();
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
            connectionLocal.emplace_back(connection);
        }
        thiz.connectionArray.swap(connectionLocal);
        thiz.connectionArray.emplace_back(connection);
        thiz.connectionMutex.unlock();
    }

    thiz.terminate = true;
}
//------------------------------------------------------------------------------
bool Listener::Start()
{
    struct sockaddr_storage sockaddr = {};
    socklen_t sockaddrLength = Connection::SetAddressPort(sockaddr, thiz.address, thiz.port);
    thiz.socket = Socket::socket(sockaddr.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (thiz.socket <= 0)
    {
        LISTEN_LOG(-1, "%s %s", "socket", Socket::strerror(Socket::errno));
        return false;
    }

    int enable = 1;
    Socket::setsockopt(thiz.socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    Socket::setsockopt(thiz.socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

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
    if (Socket::setsockopt(thiz.socket, SOL_SOCKET, SO_ATTACH_REUSEPORT_CBPF, &cbpf, sizeof(cbpf)) == 0)
    {
        LISTEN_LOG(0, "%s %s", "setsockopt", "Classic BPF");
    }
#endif

    if (Socket::bind(thiz.socket, (struct sockaddr*)&sockaddr, sockaddrLength) != 0)
    {
        LISTEN_LOG(-1, "%s %s", "bind", Socket::strerror(Socket::errno));
        return false;
    }

    int fastOpen = 5;
    Socket::setsockopt(thiz.socket, SOL_TCP, TCP_FASTOPEN, &fastOpen, sizeof(fastOpen));

    if (Socket::listen(thiz.socket, thiz.backlog) != 0)
    {
        LISTEN_LOG(-1, "%s %s", "listen", Socket::strerror(Socket::errno));
        return false;
    }

    thiz.threadListen = std::stacking_thread(65536, [this]{ thiz.ProcedureListen(); });
    if (thiz.threadListen.joinable() == false)
    {
        LISTEN_LOG(-1, "%s %s", "thread", ::strerror(errno));
        return false;
    }

    LISTEN_LOG(0, "%s %s", "listen", "ready");
    return true;
}
//------------------------------------------------------------------------------
void Listener::Stop()
{
    thiz.terminate = true;

    if (thiz.socket > 0)
    {
        Socket::shutdown(thiz.socket, SHUT_RD);
        Socket::close(thiz.socket);
        thiz.socket = 0;
    }
    if (thiz.threadListen.joinable())
    {
        thiz.threadListen.join();
    }

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
Connection* Listener::CreateConnection(int socket, const struct sockaddr_storage& addr)
{
    return new Connection(socket, thiz.address, thiz.port, addr);
}
//------------------------------------------------------------------------------
