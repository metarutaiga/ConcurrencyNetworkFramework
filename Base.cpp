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
    for (void(*f)(Base*) : thiz.delayDestroy)
    {
        f(this);
    }
}
//------------------------------------------------------------------------------
void Base::DelayDestroy(void(*f)(Base*))
{
    thiz.delayDestroy.emplace_back(f);
}
//------------------------------------------------------------------------------
