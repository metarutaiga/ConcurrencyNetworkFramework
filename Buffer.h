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
    static void Clean();
    static Buffer Get(size_t size);
    static int Used();
    static int Unused();
};
