//==============================================================================
// ConcurrencyNetworkFramework : Base Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"

class Base
{
protected:
    volatile bool terminate;

    std::vector<void(*)(Base*)> delayDestroy;

protected:
    Base();
    virtual ~Base();

    void DelayDestroy(void(*f)(Base*));
};
