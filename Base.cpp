//==============================================================================
// ConcurrencyNetworkFramework : Base Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include "Base.h"

//------------------------------------------------------------------------------
Base::Base()
{
    
}
//------------------------------------------------------------------------------
Base::~Base()
{
    for (std::function<void()> f : thiz.delayDestroy)
    {
        f();
    }
}
//------------------------------------------------------------------------------
void Base::DelayDestroy(std::function<void()>&& f)
{
    thiz.delayDestroy.emplace_back(f);
}
//------------------------------------------------------------------------------
