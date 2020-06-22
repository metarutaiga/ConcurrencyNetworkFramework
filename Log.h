//==============================================================================
// ConcurrencyNetworkFramework : Log Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

class Log
{
public:
    enum Level
    {
        FATAL   = -2,
        ERROR   = -1,
        WARNING = 0,
        INFO    = 1,
        DEBUG   = 2,
    };
public:
    static void (*Output)(int level, const char* output);
    static void (*Format)(int level, const char* format, ...) __attribute__((format(printf, 2, 3)));
public:
    static void SetOutput(void(*output)(int, const char*));
    static void SetFormat(void(*format)(int, const char*, ...));
private:
    static void DefaultOutput(int level, const char* output);
    static void DefaultFormat(int level, const char* format, ...);
};
