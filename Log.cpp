//==============================================================================
// ConcurrencyNetworkFramework : Log Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include <stdio.h>
#include <stdarg.h>
#include <alloca.h>
#include "Log.h"

//------------------------------------------------------------------------------
void (*Log::Output)(const char* format) = Log::DefaultOutput;
void (*Log::Format)(int level, const char* format, ...) = Log::DefaultFormat;
//------------------------------------------------------------------------------
void Log::SetOutput(void(*output)(const char*))
{
    Log::Output = output;
}
//------------------------------------------------------------------------------
void Log::SetFormat(void(*format)(int, const char*, ...))
{
    Log::Format = format;
}
//------------------------------------------------------------------------------
void Log::DefaultOutput(const char* output)
{
    printf("%s", output);
}
//------------------------------------------------------------------------------
void Log::DefaultFormat(int level, const char* format, ...)
{
    char replaceFormat[256];
    switch (level)
    {
    case -1:
        snprintf(replaceFormat, 256, "[%s] %s\n", "ERROR", format);
        break;

    default:
        snprintf(replaceFormat, 256, "[%s] %s\n", "INFO", format);
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
    Log::Output(buffer);
    va_end(vaOutput);

    va_end(va);
}
//------------------------------------------------------------------------------
