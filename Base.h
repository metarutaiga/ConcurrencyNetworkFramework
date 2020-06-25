//==============================================================================
// ConcurrencyNetworkFramework : Base Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"

class FRAMEWORK_API Base
{
    volatile bool terminate;

    std::vector<std::function<void()>> delayDestroy;

protected:
    Base();
    virtual ~Base();

    virtual void Terminate();
    virtual bool Terminating() const;

    virtual void DelayDestroy(std::function<void()>&& f);
};
