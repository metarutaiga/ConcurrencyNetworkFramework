//==============================================================================
// ConcurrencyNetworkFramework : Framework Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "Base.h"

class Event;
class Listener;
class Framework : public Base
{
protected:
    std::vector<Listener*> serverArray;

    std::counting_semaphore eventSemaphore;
    std::vector<Event*> eventArray;
    std::mutex eventMutex;

public:
    Framework();
    virtual ~Framework();

    virtual void Terminate();

    virtual void Server(Listener* server);

    virtual void Push(Event* event);

    virtual int Dispatch(size_t listenCount);
};
