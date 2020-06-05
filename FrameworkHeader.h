//==============================================================================
// ConcurrencyNetworkFramework : Common Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include <pthread.h>
#include <semaphore.h>

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
