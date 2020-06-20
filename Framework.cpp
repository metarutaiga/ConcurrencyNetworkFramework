//==============================================================================
// ConcurrencyNetworkFramework : Framework Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include "Buffer.h"
#include "Event.h"
#include "Listener.h"
#include "Log.h"
#include "Framework.h"

//------------------------------------------------------------------------------
Framework::Framework()
{
    thiz.terminate = false;
}
//------------------------------------------------------------------------------
Framework::~Framework()
{
    Buffer::Clean();
}
//------------------------------------------------------------------------------
void Framework::Terminate()
{
    thiz.terminate = true;
}
//------------------------------------------------------------------------------
void Framework::Server(Listener* server)
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
    int idleTime = 0;
    std::vector<Event*> eventLocal;

    Log::Format(0, "Framework : Start");
    for (Listener* server : thiz.serverArray)
    {
        server->Start();
    }

    Log::Format(0, "Framework : Loop");
    while (thiz.terminate == false)
    {
        if (thiz.eventArray.empty())
        {
            idleTime++;
            if (idleTime >= 60 * 60)
            {
                idleTime = 0;
                Log::Format(0, "Framework : Idle");
            }

            struct timespec timespec;
            timespec.tv_sec = 0;
            timespec.tv_nsec = 1000 * 1000 * 1000 / 60;
            ::nanosleep(&timespec, nullptr);
            continue;
        }
        idleTime = 0;

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
    Log::Format(0, "Framework : Shutdown");

    for (Listener* server : thiz.serverArray)
    {
        server->Stop();
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

    Log::Format(0, "Framework : Terminate");
    return 0;
}
//------------------------------------------------------------------------------
