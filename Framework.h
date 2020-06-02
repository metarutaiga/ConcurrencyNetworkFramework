//==============================================================================
// ConcurrencyNetworkFramework : Framework Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include <mutex>
#include <vector>

class Event;
class Listen;
class Framework
{
    std::vector<Event*> eventArray;
    std::mutex eventMutex;

    Listen* server;

    volatile bool terminate;

public:
    Framework();
    virtual ~Framework();

    void Push(Event* event);

    int Dispatch();

    static Framework& Instance();
};
