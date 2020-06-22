//==============================================================================
// ConcurrencyNetworkFramework : Buffer Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"

class Buffer : public std::shared_ptr<std::vector<char>>
{
protected:
    static void Recycle(element_type* pointer);

public:
    template<typename T>
    T& cast()
    {
        element_type* pointer = get();
        pointer->resize(sizeof(T));
        return reinterpret_cast<T&>(pointer->front());
    }

public:
    static void Clean();
    static Buffer Get(size_t size = 0);
    static int Used();
    static int Unused();
};
