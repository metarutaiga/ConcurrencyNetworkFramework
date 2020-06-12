//==============================================================================
// ConcurrencyNetworkFramework : Common Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include <pthread.h>
#if defined(__APPLE__)
#   include <dispatch/dispatch.h>
#   define sem_t                dispatch_semaphore_t
#   define sem_init(s, p, v)    (*s) = dispatch_semaphore_create(v)
#   define sem_destroy(s)       dispatch_release(*s)
#   define sem_post(s)          dispatch_semaphore_signal(*s)
#   define sem_wait(s)          dispatch_semaphore_wait(*s, DISPATCH_TIME_FOREVER)
#   define sem_timedwait(s, a)  dispatch_semaphore_wait(*s, dispatch_walltime(a, 0))
#else
#   include <semaphore.h>
    struct sem_comparable_t : public sem_t
    {
        operator bool()
        {
            return true;
        }
        bool operator == (const sem_t& other) const
        {
            return false;
        }
    };
#   define sem_t                sem_comparable_t
#endif

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#if defined(HAVE_CONFIG_H)
#   include "config.h"
#endif

#ifndef MSG_NOSIGNAL
#   define MSG_NOSIGNAL 0
#endif

#ifndef MSG_MORE
#   define MSG_MORE 0
#endif

#ifndef SOL_TCP
#   define SOL_TCP IPPROTO_TCP
#endif

#ifndef thiz
#   define thiz (*this)
#endif
