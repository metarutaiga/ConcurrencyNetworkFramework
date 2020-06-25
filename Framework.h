//==============================================================================
// ConcurrencyNetworkFramework : Framework Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

#include "Base.h"

class FRAMEWORK_API Event;
class FRAMEWORK_API Listener;
class FRAMEWORK_API Framework : public Base
{
protected:
    std::counting_semaphore eventSemaphore;
    std::vector<Event*> eventArray;
    std::mutex eventMutex;

    std::vector<Listener*> serverArray;

public:
    Framework();
    virtual ~Framework();

    virtual void Terminate();

    virtual void Push(Event* event);
    virtual void Server(Listener* server);

    virtual int Dispatch(size_t listenCount);
};
