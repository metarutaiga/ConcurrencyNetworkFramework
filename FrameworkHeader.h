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
#   define sem_timedwait(s, a)  dispatch_semaphore_wait(*s, dispatch_time(DISPATCH_TIME_NOW, (*a).tv_sec * NSEC_PER_SEC + (*a).tv_nsec))
#else
#   include <semaphore.h>
#endif

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

#define thiz (*this)
