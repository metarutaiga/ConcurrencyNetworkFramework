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
    static void (*Output)(int level, const char* output);
    static void (*Format)(int level, const char* format, ...) __attribute__((format(printf, 2, 3)));
public:
    static void SetOutput(void(*output)(int, const char*));
    static void SetFormat(void(*format)(int, const char*, ...));
private:
    static void DefaultOutput(int level, const char* output);
    static void DefaultFormat(int level, const char* format, ...);
};
