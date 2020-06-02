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
    server = nullptr;
    terminate = false;
}
//------------------------------------------------------------------------------
Framework::~Framework()
{

}
//------------------------------------------------------------------------------
void Framework::Push(Event* event)
{
    eventMutex.lock();
    eventArray.emplace_back(event);
    eventMutex.unlock();
}
//------------------------------------------------------------------------------
int Framework::Dispatch()
{
    std::vector<Event*> eventLocal;

    while (terminate == false)
    {
        if (eventArray.empty())
        {
            struct timespec timespec;
            timespec.tv_sec = 0;
            timespec.tv_nsec = 1000 * 1000 * 1000 / 60;
            nanosleep(&timespec, nullptr);
            continue;
        }

        eventMutex.lock();
        eventArray.swap(eventLocal);
        eventMutex.unlock();

        for (Event* event : eventLocal)
        {
            event->Execute();
            delete event;
        }
        eventLocal.clear();
    }

    delete server;
    server = nullptr;

    eventMutex.lock();
    eventArray.swap(eventLocal);
    eventMutex.unlock();

    for (Event* event : eventLocal)
        delete event;
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
