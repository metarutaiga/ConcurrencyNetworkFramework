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

    std::vector<std::function<void()>> delayDestroy;

protected:
    Base();
    virtual ~Base();

    void DelayDestroy(std::function<void()>&& f);
};
