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

#include <cstddef>

#if _LIBCPP_VERSION
#   include <__threading_support>
#   define __libcpp_thread_create __libcpp_thread_create_with_stack
#   include <thread>
#   undef __libcpp_thread_create
    _LIBCPP_BEGIN_NAMESPACE_STD
    static inline int __libcpp_thread_create_with_stack(__libcpp_thread_t *__t, void *(*__func)(void *), void *__arg)
    {
        pthread_attr_t attr;
        ::pthread_attr_init(&attr);
        ::pthread_attr_setstacksize(&attr, 65536);
        int result = ::pthread_create(__t, 0, __func, __arg);
        ::pthread_attr_destroy(&attr);
        return result;
    }
    _LIBCPP_END_NAMESPACE_STD
#endif

#if _LIBCPP_VERSION
#   if defined(__APPLE__)
#       include <dispatch/dispatch.h>
#   else
#       include <semaphore.h>
#   endif
    _LIBCPP_BEGIN_NAMESPACE_STD
    struct counting_semaphore
    {
#   if defined(__APPLE__)
        dispatch_semaphore_t semaphore;
        counting_semaphore() { semaphore = dispatch_semaphore_create(0); }
        ~counting_semaphore() { dispatch_release(semaphore); }
        void release() { dispatch_semaphore_signal(semaphore); }
        void acquire() { dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER); }
#   else
        sem_t semaphore;
        counting_semaphore() { sem_init(&semaphore, 0, 0); }
        ~counting_semaphore() { sem_destroy(&semaphore); }
        void release() { sem_post(&semaphore); }
        void acquire() { sem_wait(&semaphore); }
#   endif
    };
    _LIBCPP_END_NAMESPACE_STD
#endif

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#ifndef MSG_NOSIGNAL
#   define MSG_NOSIGNAL 0
#endif

#ifndef MSG_MORE
#   define MSG_MORE 0x80000000
#endif

#ifndef SOL_TCP
#   define SOL_TCP IPPROTO_TCP
#endif

#ifndef thiz
#   define thiz (*this)
#endif
