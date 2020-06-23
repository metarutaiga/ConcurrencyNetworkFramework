//==============================================================================
// ConcurrencyNetworkFramework : Framework Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include "Buffer.h"
#include "Connection.h"
#include "Event.h"
#include "Listener.h"
#include "Log.h"
#include "Framework.h"

#define FRAMEWORK_LOG(level, format, ...) \
    Log::Format(Log::level, "%s : " format, "Framework", __VA_ARGS__)

//------------------------------------------------------------------------------
Framework::Framework()
{

}
//------------------------------------------------------------------------------
Framework::~Framework()
{
    Buffer::Clean();
}
//------------------------------------------------------------------------------
void Framework::Terminate()
{
    Base::Terminate();
    thiz.eventSemaphore.release();
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
void Framework::Server(Listener* server)
{
    thiz.serverArray.emplace_back(server);
}
//------------------------------------------------------------------------------
int Framework::Dispatch(size_t listenCount)
{
    std::vector<Event*> eventLocal;

    FRAMEWORK_LOG(INFO, "%s", "Start");
    for (Listener* server : thiz.serverArray)
    {
        server->Start(listenCount);
    }

    FRAMEWORK_LOG(INFO, "%s", "Loop");
    while (Base::Terminating() == false)
    {
        if (thiz.eventSemaphore.try_acquire_for(std::chrono::seconds(60)) == false)
        {
            FRAMEWORK_LOG(INFO, "Idle (%d/%d/%d/%d/%d/%d)",
                          Connection::GetActiveThreadCount(0),
                          Connection::GetActiveThreadCount(1),
                          Connection::GetActiveThreadCount(2),
                          Connection::GetActiveThreadCount(3),
                          Buffer::Used(),
                          Buffer::Unused());
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
    FRAMEWORK_LOG(INFO, "%s", "Shutdown");

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

    FRAMEWORK_LOG(INFO, "%s", "Terminate");
    return 0;
}
//------------------------------------------------------------------------------
