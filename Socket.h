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
    static socket_t (*accept)(socket_t socket, struct sockaddr* addr, socklen_t* addrlen);
    static int (*bind)(socket_t socket, const struct sockaddr* name, socklen_t namelen);
    static int (*connect)(socket_t socket, const struct sockaddr* name, socklen_t namelen);
    static int (*close)(socket_t socket);
    static int& (*errno)();
    static int (*getpeername)(socket_t socket, struct sockaddr* name, socklen_t* namelen);
    static int (*getsockname)(socket_t socket, struct sockaddr* name, socklen_t* namelen);
    static int (*getsockopt)(socket_t socket, int level, int optname, void* optval, socklen_t* optlen);
    static int (*listen)(socket_t socket, int backlog);
    static ssize_t (*recv)(socket_t socket, void* buf, size_t len, int flags);
    static ssize_t (*recvfrom)(socket_t socket, void* buf, size_t len, int flags, struct sockaddr* addr, socklen_t* addrlen);
    static int (*setsockopt)(socket_t socket, int level, int optname, const void* optval, socklen_t optlen);
    static ssize_t (*send)(socket_t socket, const void* buf, size_t len, int flags, char corkbuf[CORK_SIZE]);
    static ssize_t (*sendto)(socket_t socket, const void* buf, size_t len, int flags, const struct sockaddr* name, socklen_t namelen);
    static int (*shutdown)(socket_t socket, int flags);
    static socket_t (*socket)(int af, int type, int protocol);
    static char* (*strerror)(int errnum);

public:
    static void startup();

protected:
    static socket_t acceptInternal(socket_t socket, struct sockaddr* addr, socklen_t* addrlen);
    static int& errnoInternal();
    static ssize_t recvInternal(socket_t socket, void* buf, size_t len, int flags);
    static ssize_t sendInternal(socket_t socket, const void* buf, size_t len, int flags, char corkbuf[Socket::CORK_SIZE]);
    static char* strerrorInternal(int errnum);
};

#define errno errno()
