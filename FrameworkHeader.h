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
#else
    return ::errno;
#endif
}
#define errno errno()

#if defined(__APPLE__)
#   include <dispatch/dispatch.h>
#   define sem_t                dispatch_semaphore_t
#   define sem_init(s, p, v)    atoi(((*s) = dispatch_semaphore_create(v)) ? "0" : "-1")
#   define sem_destroy(s)       dispatch_release(*s)
#   define sem_post(s)          dispatch_semaphore_signal(*s)
#   define sem_wait(s)          dispatch_semaphore_wait(*s, DISPATCH_TIME_FOREVER)
#   define sem_timedwait(s, a)  dispatch_semaphore_wait(*s, dispatch_walltime(a, 0))
#else
#   include <semaphore.h>
    struct sem_comparable_t : public sem_t { operator bool() { return true; } };
#   define sem_t                sem_comparable_t
#endif

#include <pthread.h>
#if _LIBCPP_VERSION
#   include <__threading_support>
#   define __libcpp_thread_create __libcpp_thread_create_with_stack
#   include <thread>
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
