//==============================================================================
// ConcurrencyNetworkFramework : Framework Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include "time.h"
#include "Event.h"
#include "Listen.h"
#include "Framework.h"

//------------------------------------------------------------------------------
Framework::Framework()
{
    thiz.terminate = false;
}
//------------------------------------------------------------------------------
Framework::~Framework()
{

}
//------------------------------------------------------------------------------
void Framework::Server(Listen* server)
{
    thiz.serverArray.emplace_back(server);
}
//------------------------------------------------------------------------------
void Framework::Push(Event* event)
{
    thiz.eventMutex.lock();
    thiz.eventArray.emplace_back(event);
    thiz.eventMutex.unlock();
}
//------------------------------------------------------------------------------
int Framework::Dispatch()
{
    std::vector<Event*> eventLocal;

    while (thiz.terminate == false)
    {
        if (thiz.eventArray.empty())
        {
            struct timespec timespec;
            timespec.tv_sec = 0;
            timespec.tv_nsec = 1000 * 1000 * 1000 / 60;
            nanosleep(&timespec, nullptr);
            continue;
        }

        thiz.eventMutex.lock();
        thiz.eventArray.swap(eventLocal);
        thiz.eventMutex.unlock();

        for (Event* event : eventLocal)
        {
            event->Execute();
            delete event;
        }
        eventLocal.clear();
    }

    for (Listen* server : thiz.serverArray)
    {
        delete server;
    }
    thiz.serverArray.clear();

    thiz.eventMutex.lock();
    thiz.eventArray.swap(eventLocal);
    thiz.eventMutex.unlock();
    for (Event* event : eventLocal)
    {
        delete event;
    }
    eventLocal.clear();

    return 0;
}
//------------------------------------------------------------------------------
Framework& Framework::Instance()
{
    static Framework framework;
    return framework;
}
//------------------------------------------------------------------------------
