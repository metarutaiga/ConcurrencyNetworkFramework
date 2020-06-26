//==============================================================================
// ConcurrencyNetworkFramework : Common Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#if defined(HAVE_CONFIG_H)
#   include "config.h"
#endif

#if defined(__APPLE__) || defined(__unix__)
#   define FRAMEWORK_API
#   include <sys/errno.h>
#elif defined(_WIN32)
#   pragma warning(disable:4251)
#   if defined(_BUILD)
#       define FRAMEWORK_API __declspec(dllexport)
#   else
#       define FRAMEWORK_API __declspec(dllimport)
#   endif
#   define _CRT_NONSTDC_NO_WARNINGS
#   define _CRT_SECURE_NO_WARNINGS
#   define NOMINMAX
#   define VC_EXTRALEAN
#   define WIN32_LEAN_AND_MEAN
#   include <errno.h>
#endif
#undef errno
static inline int& errno()
{
#if defined(__ANDROID__)
    return (*__errno());
#elif defined(__APPLE__)
    return (*__error());
#elif defined(__linux__)
    return (*__errno_location());
#elif defined(_WIN32)
    return (*_errno());
#else
    return ::errno;
#endif
}
#include <stddef.h>
#include <stdlib.h>
#undef errno
#define errno errno()

#if defined(__APPLE__) || defined(__unix__)
#   include <sys/socket.h>
#elif defined(_WIN32)
#   include <WinSock2.h>
#   include <WS2tcpip.h>
#   undef ERROR
#   define __attribute__(unused)
#   define SHUT_RD SD_RECEIVE
#   define SO_REUSEPORT SO_REUSEADDR
#   define localtime_r(a,b) localtime_s(b,a)
    typedef int socklen_t;
    typedef int ssize_t;
    typedef SOCKET socket_t;
#endif

#include "HackingSTL/semaphore"
#include "HackingSTL/thread"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#ifndef MSG_NOSIGNAL
#   define MSG_NOSIGNAL 0
#endif

#ifndef MSG_MORE
#   define MSG_MORE 0x8000
#endif

#ifndef SOL_TCP
#   define SOL_TCP IPPROTO_TCP
#endif

#ifndef thiz
#   define thiz (*this)
#endif
