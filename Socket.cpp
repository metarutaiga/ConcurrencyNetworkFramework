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
    static inline int close(SOCKET socket)
    {
        return ::closesocket(socket);
    }
#endif

//------------------------------------------------------------------------------
#undef errno
//------------------------------------------------------------------------------
socket_t (*Socket::accept)(socket_t socket, struct sockaddr* addr, socklen_t* addrlen);
int (*Socket::bind)(socket_t socket, const struct sockaddr* name, socklen_t namelen);
int (*Socket::connect)(socket_t socket, const struct sockaddr* name, socklen_t namelen);
int (*Socket::close)(socket_t socket);
int& (*Socket::errno)();
int (*Socket::getpeername)(socket_t socket, struct sockaddr* name, socklen_t* namelen);
int (*Socket::getsockname)(socket_t socket, struct sockaddr* name, socklen_t* namelen);
int (*Socket::getsockopt)(socket_t socket, int level, int optname, void* optval, socklen_t* optlen);
int (*Socket::listen)(socket_t socket, int backlog);
ssize_t (*Socket::recv)(socket_t socket, void* buf, size_t len, int flags);
ssize_t (*Socket::recvfrom)(socket_t socket, void* buf, size_t len, int flags, struct sockaddr* addr, socklen_t* addrlen);
int (*Socket::setsockopt)(socket_t socket, int level, int optname, const void* optval, socklen_t optlen);
ssize_t (*Socket::send)(socket_t socket, const void* buf, size_t len, int flags, char so_temp[Socket::CORK_SIZE]);
ssize_t (*Socket::sendto)(socket_t socket, const void* buf, size_t len, int flags, const struct sockaddr* name, socklen_t namelen);
int (*Socket::shutdown)(socket_t socket, int flags);
socket_t (*Socket::socket)(int af, int type, int protocol);
char* (*Socket::strerror)(int errnum);
//------------------------------------------------------------------------------
void Socket::startup()
{
#if defined(__APPLE__)
    signal(SIGPIPE, SIG_IGN);
#elif defined(_WIN32)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif

    Socket::accept = [](socket_t socket, struct sockaddr* addr, socklen_t* addrlen) { return ::accept(socket, addr, addrlen); };
    Socket::bind = [](socket_t socket, const struct sockaddr* name, socklen_t namelen) { return ::bind(socket, name, namelen); };
    Socket::connect = [](socket_t socket, const struct sockaddr* name, socklen_t namelen) { return ::connect(socket, name, namelen); };
    Socket::close = [](socket_t socket) { return ::close(socket); };
    Socket::errno = []() -> int& { return ::errno(); };
    Socket::getpeername = [](socket_t socket, struct sockaddr* name, socklen_t* namelen) { return ::getpeername(socket, name, namelen); };
    Socket::getsockname = [](socket_t socket, struct sockaddr* name, socklen_t* namelen) { return ::getsockname(socket, name, namelen); };
    Socket::getsockopt = [](socket_t socket, int level, int optname, void* optval, socklen_t* optlen) { return ::getsockopt(socket, level, optname, optval, optlen); };
    Socket::listen = [](socket_t socket, int backlog) { return ::listen(socket, backlog); };
    Socket::recv = [](socket_t socket, void* buf, size_t len, int flags) { return ::recv(socket, buf, len, flags); };
    Socket::recvfrom = [](socket_t socket, void* buf, size_t len, int flags, struct sockaddr* addr, socklen_t* addrlen) { return ::recvfrom(socket, buf, len, flags, addr, addrlen); };
    Socket::setsockopt = [](socket_t socket, int level, int optname, const void* optval, socklen_t optlen) { return ::setsockopt(socket, level, optname, optval, optlen); };
    Socket::send = [](socket_t socket, const void* buf, size_t len, int flags, char so_temp[Socket::CORK_SIZE]) { return ::send(socket, buf, len, flags); };
    Socket::sendto = [](socket_t socket, const void* buf, size_t len, int flags, const struct sockaddr* name, socklen_t namelen) { return ::sendto(socket, buf, len, flags, name, namelen); };
    Socket::shutdown = [](socket_t socket, int flags) { return ::shutdown(socket, flags); };
    Socket::socket = [](int af, int type, int protocol) { return ::socket(af, type, protocol); };
    Socket::strerror = [](int errnum) { return ::strerror(errnum); };

    Socket::accept = Socket::acceptInternal;
    Socket::errno = Socket::errnoInternal;
    Socket::recv = Socket::recvInternal;
    Socket::send = Socket::sendInternal;
    Socket::strerror = Socket::strerrorInternal;
}
//------------------------------------------------------------------------------
#define errno errno()
//------------------------------------------------------------------------------
socket_t Socket::acceptInternal(socket_t socket, struct sockaddr* addr, socklen_t* addrlen)
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
ssize_t Socket::recvInternal(socket_t socket, void* buf, size_t len, int flags)
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
ssize_t Socket::sendInternal(socket_t socket, const void* buf, size_t len, int flags, char corkbuf[Socket::CORK_SIZE])
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
            ssize_t result = sendInternal(socket, cork.buffer, length, flags & ~MSG_MORE, corkbuf);
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
int& Socket::errnoInternal()
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
char* Socket::strerrorInternal(int errnum)
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
