//==============================================================================
// ConcurrencyNetworkFramework : Socket Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include "Socket.h"
#if defined(__APPLE__) || defined(__unix__)
#   include <unistd.h>
#   include <string.h>
#elif defined(_WIN32)
#   include <WinSock2.h>
#   pragma comment(lib, "ws2_32")
    static inline int SOCKET_API close(SOCKET socket)
    {
        return ::closesocket(socket);
    }
#endif

//------------------------------------------------------------------------------
#undef errno
//------------------------------------------------------------------------------
socket_t (SOCKET_API *Socket::accept)(socket_t socket, struct sockaddr* addr, socklen_t* addrlen);
int (SOCKET_API *Socket::bind)(socket_t socket, const struct sockaddr* name, socklen_t namelen);
int (SOCKET_API *Socket::connect)(socket_t socket, const struct sockaddr* name, socklen_t namelen);
int (SOCKET_API *Socket::close)(socket_t socket);
int& (SOCKET_API *Socket::errno)();
int (SOCKET_API *Socket::getpeername)(socket_t socket, struct sockaddr* name, socklen_t* namelen);
int (SOCKET_API *Socket::getsockname)(socket_t socket, struct sockaddr* name, socklen_t* namelen);
int (SOCKET_API *Socket::getsockopt)(socket_t socket, int level, int optname, void* optval, socklen_t* optlen);
int (SOCKET_API *Socket::listen)(socket_t socket, int backlog);
ssize_t (SOCKET_API *Socket::recv)(socket_t socket, void* buf, size_t len, int flags);
ssize_t (SOCKET_API *Socket::recvfrom)(socket_t socket, void* buf, size_t len, int flags, struct sockaddr* addr, socklen_t* addrlen);
int (SOCKET_API *Socket::setsockopt)(socket_t socket, int level, int optname, const void* optval, socklen_t optlen);
ssize_t (SOCKET_API *Socket::send)(socket_t socket, const void* buf, size_t len, int flags, char so_temp[Socket::CORK_SIZE]);
ssize_t (SOCKET_API *Socket::sendto)(socket_t socket, const void* buf, size_t len, int flags, const struct sockaddr* name, socklen_t namelen);
int (SOCKET_API *Socket::shutdown)(socket_t socket, int flags);
socket_t (SOCKET_API *Socket::socket)(int af, int type, int protocol);
char* (SOCKET_API *Socket::strerror)(int errnum);
//------------------------------------------------------------------------------
void Socket::startup()
{
#if defined(__APPLE__)
    signal(SIGPIPE, SIG_IGN);
#elif defined(_WIN32)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif
    (void*&)Socket::accept = (void*)Socket::acceptloop;
    (void*&)Socket::bind = (void*)::bind;
    (void*&)Socket::connect = (void*)::connect;
    (void*&)Socket::close = (void*)::close;
    (void*&)Socket::errno = (void*)Socket::errnoloop;
    (void*&)Socket::getpeername = (void*)::getpeername;
    (void*&)Socket::getsockname = (void*)::getsockname;
    (void*&)Socket::getsockopt = (void*)::getsockopt;
    (void*&)Socket::listen = (void*)::listen;
    (void*&)Socket::recv = (void*)Socket::recvloop;
    (void*&)Socket::recvfrom = (void*)::recvfrom;
    (void*&)Socket::setsockopt = (void*)::setsockopt;
    (void*&)Socket::send = (void*)Socket::sendloop;
    (void*&)Socket::sendto = (void*)::sendto;
    (void*&)Socket::shutdown = (void*)::shutdown;
    (void*&)Socket::socket = (void*)::socket;
    (void*&)Socket::strerror = (void*)Socket::strerrorloop;
}
//------------------------------------------------------------------------------
#define errno errno()
//------------------------------------------------------------------------------
socket_t Socket::acceptloop(socket_t socket, struct sockaddr* addr, socklen_t* addrlen)
{
    socket_t result = 0;
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
ssize_t Socket::recvloop(socket_t socket, void* buf, size_t len, int flags)
{
    ssize_t result = 0;
    for (;;)
    {
        result = ::recv(socket, (char*)buf, (socklen_t)len, flags);
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
ssize_t Socket::sendloop(socket_t socket, const void* buf, size_t len, int flags, char corkbuf[Socket::CORK_SIZE])
{
    struct CORK
    {
        size_t length;
        char buffer[Socket::CORK_SIZE - sizeof(size_t)];
    };
    CORK& cork = reinterpret_cast<CORK&>(*corkbuf);

    if (flags & MSG_MORE || cork.length)
    {
        if (len + cork.length > sizeof(cork.buffer))
        {
            size_t length = cork.length;
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
                return (ssize_t)len;
            buf = cork.buffer;
            len = cork.length;
            flags &= ~MSG_MORE;
            cork.length = 0;
        }
    }

    ssize_t result = 0;
    for (;;)
    {
        result = ::send(socket, (char*)buf, (socklen_t)len, flags);
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
int& Socket::errnoloop()
{
#if defined(__APPLE__) || defined(__unix__)
    return ::errno;
#elif defined(_WIN32)
    thread_local static int error;
    error = WSAGetLastError();
    return error;
#endif
}
//------------------------------------------------------------------------------
char* Socket::strerrorloop(int errnum)
{
#if defined(__APPLE__) || defined(__unix__)
    if (errnum == 0)
        return const_cast<char*>("Connection lost");
    return ::strerror(errnum);
#elif defined(_WIN32)
    thread_local static std::string message;
    message.resize(256);
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errnum, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), &message.front(), 256, nullptr);
    size_t crlf = message.find("\r");
    if (crlf != std::string::npos)
        message.resize(crlf);
    return &message.front();
#endif
}
//------------------------------------------------------------------------------
