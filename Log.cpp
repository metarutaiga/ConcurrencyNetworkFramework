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
    ::printf("%s", output);
}
//------------------------------------------------------------------------------
void Log::DefaultFormat(int level, const char* format, ...)
{
    time_t now = ::time(0);
    struct tm tm;
    ::localtime_r(&now, &tm);

    char timestamp[32];
    ::strftime(timestamp, 32, "%Y-%m-%d %H:%M:%S", &tm);

    static const char* const levelName[DEBUG - FATAL + 1] =
    {
        "FATAL",
        "ERROR",
        "WARNING",
        "INFO",
        "DEBUG",
    };

    if (level < FATAL)
        level = FATAL;
    else if (level > DEBUG)
        level = DEBUG;

    char replaceFormat[256];
    ::snprintf(replaceFormat, 256, "%s [%s] %s\n", timestamp, levelName[level - FATAL], format);

    va_list va;
    va_start(va, format);

    va_list vaCount;
    va_copy(vaCount, va);
    size_t count = ::vsnprintf(nullptr, 0, replaceFormat, vaCount);
    va_end(vaCount);

    va_list vaOutput;
    va_copy(vaOutput, va);
    char* buffer = static_cast<char*>(::alloca(count + 1));
    ::vsnprintf(buffer, count + 1, replaceFormat, vaOutput);
    Log::Output(level, buffer);
    va_end(vaOutput);

    va_end(va);
}
//------------------------------------------------------------------------------
