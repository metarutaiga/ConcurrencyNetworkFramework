//==============================================================================
// ConcurrencyNetworkFramework : Socket Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"

#undef errno

class FRAMEWORK_API Socket
{
public:
    enum { CORK_SIZE = 1280 };

public:
    static socket_t (SOCKET_API *accept)(socket_t socket, struct sockaddr* addr, socklen_t* addrlen);
    static int (SOCKET_API *bind)(socket_t socket, const struct sockaddr* name, socklen_t namelen);
    static int (SOCKET_API *connect)(socket_t socket, const struct sockaddr* name, socklen_t namelen);
    static int (SOCKET_API *close)(socket_t socket);
    static int& (SOCKET_API *errno)();
    static int (SOCKET_API *getpeername)(socket_t socket, struct sockaddr* name, socklen_t* namelen);
    static int (SOCKET_API *getsockname)(socket_t socket, struct sockaddr* name, socklen_t* namelen);
    static int (SOCKET_API *getsockopt)(socket_t socket, int level, int optname, void* optval, socklen_t* optlen);
    static int (SOCKET_API *listen)(socket_t socket, int backlog);
    static ssize_t (SOCKET_API *recv)(socket_t socket, void* buf, size_t len, int flags);
    static ssize_t (SOCKET_API *recvfrom)(socket_t socket, void* buf, size_t len, int flags, struct sockaddr* addr, socklen_t* addrlen);
    static int (SOCKET_API *setsockopt)(socket_t socket, int level, int optname, const void* optval, socklen_t optlen);
    static ssize_t (SOCKET_API *send)(socket_t socket, const void* buf, size_t len, int flags, char corkbuf[CORK_SIZE]);
    static ssize_t (SOCKET_API *sendto)(socket_t socket, const void* buf, size_t len, int flags, const struct sockaddr* name, socklen_t namelen);
    static int (SOCKET_API *shutdown)(socket_t socket, int flags);
    static socket_t (SOCKET_API *socket)(int af, int type, int protocol);
    static char* (SOCKET_API *strerror)(int errnum);

public:
    static void startup();

protected:
    static socket_t SOCKET_API acceptloop(socket_t socket, struct sockaddr* addr, socklen_t* addrlen);
    static int& SOCKET_API errnoloop();
    static ssize_t SOCKET_API recvloop(socket_t socket, void* buf, size_t len, int flags);
    static ssize_t SOCKET_API sendloop(socket_t socket, const void* buf, size_t len, int flags, char corkbuf[Socket::CORK_SIZE]);
    static char* SOCKET_API strerrorloop(int errnum);
};

#define errno errno()
