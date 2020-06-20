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

#include <sys/errno.h>
#undef errno
static inline int& errno()
{
#if defined(__ANDROID__)
    return (*__errno());
#elif defined(__APPLE__)
    return (*__error());
#elif defined(__linux__)
    return (*__errno_location());
#else
    return ::errno;
#endif
}
#define errno errno()

#include <sys/socket.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include "HackingSTL/semaphore"
#include "HackingSTL/thread"

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
