//==============================================================================
// ConcurrencyNetworkFramework : Socket Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include <sys/socket.h>

class Socket
{
public:
    static int (*accept)(int socket, struct sockaddr* addr, socklen_t* addrlen);
    static int (*bind)(int socket, const struct sockaddr* name, socklen_t namelen);
    static int (*connect)(int socket, const struct sockaddr* name, socklen_t namelen);
    static int (*close)(int socket);
    static int (*getpeername)(int socket, struct sockaddr* name, socklen_t* namelen);
    static int (*getsockname)(int socket, struct sockaddr* name, socklen_t* namelen);
    static int (*getsockopt)(int socket, int level, int optname, void* optval, socklen_t* optlen);
    static int (*listen)(int socket, int backlog);
    static ssize_t (*recv)(int socket, void* buf, size_t len, int flags);
    static int (*setsockopt)(int socket, int level, int optname, const void* optval, socklen_t optlen);
    static ssize_t (*send)(int socket, const void* buf, size_t len, int flags);
    static int (*shutdown)(int socket, int flags);
    static int (*socket)(int af, int type, int protocol);
};
