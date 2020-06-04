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
    switch (level)
    {
    case -1:
        printf("[%s] ", "ERROR");
        break;

    default:
        printf("[%s] ", "INFO");
        break;
    }

    va_list va;
    va_start(va, format);
    vprintf(format, va);
    printf("%s", "\n");
    va_end(va);
}
//------------------------------------------------------------------------------
