//==============================================================================
// ConcurrencyNetworkFramework : Log Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <stdio.h>
#include <stdarg.h>
#include "Log.h"

//------------------------------------------------------------------------------
void Log::Format(int level, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    vprintf(format, va);
    va_end(va);
}
//------------------------------------------------------------------------------
