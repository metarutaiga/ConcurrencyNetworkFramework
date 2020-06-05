//==============================================================================
// ConcurrencyNetworkFramework : Log Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <alloca.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "Log.h"

//------------------------------------------------------------------------------
void (*Log::Output)(int level, const char* output) = Log::DefaultOutput;
void (*Log::Format)(int level, const char* format, ...) = Log::DefaultFormat;
//------------------------------------------------------------------------------
void Log::SetOutput(void(*output)(int, const char*))
{
    Log::Output = output;
}
//------------------------------------------------------------------------------
void Log::SetFormat(void(*format)(int, const char*, ...))
{
    Log::Format = format;
}
//------------------------------------------------------------------------------
void Log::DefaultOutput(int level, const char* output)
{
    printf("%s", output);
}
//------------------------------------------------------------------------------
void Log::DefaultFormat(int level, const char* format, ...)
{
    time_t now = time(0);
    struct tm tm;
    gmtime_r(&now, &tm);

    char timestamp[32];
    strftime(timestamp, 32, "%Y-%m-%d %H:%M:%S", &tm);

    char replaceFormat[256];
    switch (level)
    {
    case -1:
        snprintf(replaceFormat, 256, "%s [%s] %s\n", timestamp, "ERROR", format);
        break;

    default:
        snprintf(replaceFormat, 256, "%s [%s] %s\n", timestamp, "INFO", format);
        break;
    }

    va_list va;
    va_start(va, format);

    va_list vaCount;
    va_copy(vaCount, va);
    size_t count = vsnprintf(nullptr, 0, replaceFormat, vaCount);
    va_end(vaCount);

    va_list vaOutput;
    va_copy(vaOutput, va);
    char* buffer = (char*)alloca(count + 1);
    vsnprintf(buffer, count + 1, replaceFormat, vaOutput);
    Log::Output(level, buffer);
    va_end(vaOutput);

    va_end(va);
}
//------------------------------------------------------------------------------
