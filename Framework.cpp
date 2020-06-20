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
    thiz.eventSemaphore.release();
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
    thiz.eventSemaphore.release();
}
//------------------------------------------------------------------------------
int Framework::Dispatch(size_t listenCount)
{
    std::vector<Event*> eventLocal;

    Log::Format(0, "Framework : Start");
    for (Listener* server : thiz.serverArray)
    {
        server->Start(listenCount);
    }

    Log::Format(0, "Framework : Loop");
    while (thiz.terminate == false)
    {
        if (thiz.eventSemaphore.try_acquire_for(std::chrono::seconds(60)) == false)
        {
            Log::Format(0, "Framework : Idle");
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
