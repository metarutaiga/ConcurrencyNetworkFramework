//==============================================================================
// ConcurrencyNetworkFramework : Socket Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <unistd.h>
#include "Socket.h"

int (*Socket::accept)(int socket, struct sockaddr* addr, socklen_t* addrlen) = ::accept;
int (*Socket::bind)(int socket, const struct sockaddr* name, socklen_t namelen) = ::bind;
int (*Socket::connect)(int socket, const struct sockaddr* name, socklen_t namelen) = ::connect;
int (*Socket::close)(int socket) = ::close;
int (*Socket::getpeername)(int socket, struct sockaddr* name, socklen_t* namelen) = ::getpeername;
int (*Socket::getsockname)(int socket, struct sockaddr* name, socklen_t* namelen) = ::getsockname;
int (*Socket::getsockopt)(int socket, int level, int optname, void* optval, socklen_t* optlen) = ::getsockopt;
int (*Socket::listen)(int socket, int backlog) = ::listen;
ssize_t (*Socket::recv)(int socket, void* buf, size_t len, int flags) = ::recv;
int (*Socket::setsockopt)(int socket, int level, int optname, const void* optval, socklen_t optlen) = ::setsockopt;
ssize_t (*Socket::send)(int socket, const void* buf, size_t len, int flags) = ::send;
int (*Socket::shutdown)(int socket, int flags) = ::shutdown;
int (*Socket::socket)(int af, int type, int protocol) = ::socket;
