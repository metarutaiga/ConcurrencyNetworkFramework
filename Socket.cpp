//==============================================================================
// ConcurrencyNetworkFramework : Socket Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <string.h>
#include <unistd.h>
#include "Socket.h"

//------------------------------------------------------------------------------
#undef errno
//------------------------------------------------------------------------------
int (*Socket::accept)(int socket, struct sockaddr* addr, socklen_t* addrlen) = acceptloop;
int (*Socket::bind)(int socket, const struct sockaddr* name, socklen_t namelen) = ::bind;
int (*Socket::connect)(int socket, const struct sockaddr* name, socklen_t namelen) = ::connect;
int (*Socket::close)(int socket) = ::close;
int& (*Socket::errno)() = ::errno;
int (*Socket::getpeername)(int socket, struct sockaddr* name, socklen_t* namelen) = ::getpeername;
int (*Socket::getsockname)(int socket, struct sockaddr* name, socklen_t* namelen) = ::getsockname;
int (*Socket::getsockopt)(int socket, int level, int optname, void* optval, socklen_t* optlen) = ::getsockopt;
int (*Socket::listen)(int socket, int backlog) = ::listen;
ssize_t (*Socket::recv)(int socket, void* buf, size_t len, int flags) = Socket::recvloop;
ssize_t (*Socket::recvfrom)(int socket, void* buf, size_t len, int flags, struct sockaddr* addr, socklen_t* addrlen) = ::recvfrom;
int (*Socket::setsockopt)(int socket, int level, int optname, const void* optval, socklen_t optlen) = ::setsockopt;
ssize_t (*Socket::send)(int socket, const void* buf, size_t len, int flags, char so_temp[Socket::CORK_SIZE]) = Socket::sendloop;
ssize_t (*Socket::sendto)(int socket, const void* buf, size_t len, int flags, const struct sockaddr* name, socklen_t namelen) = ::sendto;
int (*Socket::shutdown)(int socket, int flags) = ::shutdown;
int (*Socket::socket)(int af, int type, int protocol) = ::socket;
char* (*Socket::strerror)(int errnum) = ::strerror;
//------------------------------------------------------------------------------
#define errno errno()
//------------------------------------------------------------------------------
int Socket::acceptloop(int socket, struct sockaddr* addr, socklen_t* addrlen)
{
    int result = 0;
    for (;;)
    {
        result = ::accept(socket, addr, addrlen);
        if (result >= 0)
            break;
        if (Socket::errno == EAGAIN || Socket::errno == EWOULDBLOCK || Socket::errno == EINTR)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
            continue;
        }
        break;
    }
    return result;
}
//------------------------------------------------------------------------------
ssize_t Socket::recvloop(int socket, void* buf, size_t len, int flags)
{
    ssize_t result = 0;
    for (;;)
    {
        result = ::recv(socket, buf, len, flags);
        if (result >= 0)
            break;
        if (Socket::errno == EAGAIN || Socket::errno == EWOULDBLOCK || Socket::errno == EINTR)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
            continue;
        }
        break;
    }
    return result;
}
//------------------------------------------------------------------------------
ssize_t Socket::sendloop(int socket, const void* buf, size_t len, int flags, char corkbuf[Socket::CORK_SIZE])
{
    struct CORK
    {
        unsigned int length;
        char buffer[Socket::CORK_SIZE - sizeof(int)];
    };
    CORK& cork = reinterpret_cast<CORK&>(*corkbuf);

    if (flags & MSG_MORE || cork.length)
    {
        if (len + cork.length > sizeof(cork.buffer))
        {
            unsigned int length = cork.length;
            cork.length = 0;
            ssize_t result = sendloop(socket, cork.buffer, length, flags & ~MSG_MORE, corkbuf);
            if (result < 0)
                return result;
        }
        else
        {
            ::memcpy(&cork.buffer[cork.length], buf, len);
            cork.length += len;
            if (flags & MSG_MORE)
                return len;
            buf = cork.buffer;
            len = cork.length;
            flags &= ~MSG_MORE;
            cork.length = 0;
        }
    }

    ssize_t result = 0;
    for (;;)
    {
        result = ::send(socket, buf, len, flags);
        if (result >= 0)
            break;
        if (Socket::errno == EAGAIN || Socket::errno == EWOULDBLOCK || Socket::errno == EINTR)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
            continue;
        }
        break;
    }
    return result;
}
//------------------------------------------------------------------------------
