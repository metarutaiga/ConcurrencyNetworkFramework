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
    _LIBCPP_BEGIN_NAMESPACE_STD
    static inline size_t __libcpp_thread_setstacksize(size_t __s = 0)
    {
        thread_local static size_t __s_ = 0;
        size_t __r = __s_;
        __s_ = __s;
        return __r;
    }
    static inline int __libcpp_thread_create_with_stack(__libcpp_thread_t *__t, void *(*__func)(void *), void *__arg)
    {
        size_t __s = __libcpp_thread_setstacksize();

        pthread_attr_t __a;
        ::pthread_attr_init(&__a);
        if (__s) ::pthread_attr_setstacksize(&__a, __s);
        int __r = ::pthread_create(__t, 0, __func, __arg);
        ::pthread_attr_destroy(&__a);
        return __r;
    }
    _LIBCPP_END_NAMESPACE_STD
#   define __libcpp_thread_create __libcpp_thread_create_with_stack
#   include <thread>
#   undef __libcpp_thread_create
    _LIBCPP_BEGIN_NAMESPACE_STD
    struct stacking_thread : public thread
    {
        template<class _Fp, class ..._Args>
        _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
        stacking_thread(size_t __s, _Fp&& __f, _Args&&... __args)
        {
            __libcpp_thread_setstacksize(__s);
            thread(__f, __args...).swap(*this);
        }
    };
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
        counting_semaphore(ptrdiff_t count = 0) { semaphore = dispatch_semaphore_create(count); }
        ~counting_semaphore() { dispatch_release(semaphore); }
        void release(ptrdiff_t update = 1) { while(update--) dispatch_semaphore_signal(semaphore); }
        void acquire() { dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER); }
#   else
        sem_t semaphore;
        counting_semaphore(ptrdiff_t count = 0) { sem_init(&semaphore, 0, count); }
        ~counting_semaphore() { sem_destroy(&semaphore); }
        void release(ptrdiff_t update = 1) { while(update--) sem_post(&semaphore); }
        void acquire() { while (sem_wait(&semaphore) == -1 && errno == EINTR); }
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
