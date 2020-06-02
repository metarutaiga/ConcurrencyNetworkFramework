//==============================================================================
// ConcurrencyNetworkFramework : Event Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

class Event
{
public:
    Event() {}
    virtual ~Event() {}

    virtual bool Execute() = 0;
};
