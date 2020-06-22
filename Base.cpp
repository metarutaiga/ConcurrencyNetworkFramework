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
    thiz.terminate = false;
}
//------------------------------------------------------------------------------
Base::~Base()
{
    for (std::function<void()>& f : thiz.delayDestroy)
    {
        f();
    }
}
//------------------------------------------------------------------------------
void Base::Terminate()
{
    thiz.terminate = true;
}
//------------------------------------------------------------------------------
bool Base::Terminating() const
{
    return thiz.terminate;
}
//------------------------------------------------------------------------------
void Base::DelayDestroy(std::function<void()>&& f)
{
    thiz.delayDestroy.emplace_back(f);
}
//------------------------------------------------------------------------------
