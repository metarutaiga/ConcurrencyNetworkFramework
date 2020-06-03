//==============================================================================
// ConcurrencyNetworkFramework : Framework Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "FrameworkHeader.h"

class Event;
class Listen;
class Framework
{
    volatile bool terminate;

    Listen* server;

    std::vector<Event*> eventArray;
    std::mutex eventMutex;

public:
    Framework();
    virtual ~Framework();

    void Push(Event* event);

    int Dispatch();

    static Framework& Instance();
};
