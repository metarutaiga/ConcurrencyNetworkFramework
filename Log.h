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
    static void (*Output)(const char* format);
    static void (*Format)(int level, const char* format, ...);
public:
    static void SetOutput(void(*output)(const char*));
    static void SetFormat(void(*format)(int, const char*, ...));
private:
    static void DefaultOutput(const char*);
    static void DefaultFormat(int level, const char* format, ...);
};
