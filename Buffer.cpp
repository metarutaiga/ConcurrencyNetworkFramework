//==============================================================================
// ConcurrencyNetworkFramework : Buffer Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include "Buffer.h"

static std::atomic_int bufferPoolAllocated;
static std::vector<Buffer> bufferPool;
static std::mutex bufferPoolMutex;
static bool bufferPoolTerminate;
static struct BufferPoolExit { ~BufferPoolExit() { bufferPoolTerminate = true; } } bufferPoolExit;
//------------------------------------------------------------------------------
void Buffer::Recycle(element_type* pointer)
{
    if (pointer == nullptr)
        return;

    if (bufferPoolTerminate)
    {
        delete pointer;
        return;
    }

    bufferPoolMutex.lock();
    bufferPool.push_back(Buffer());
    bufferPool.back().reset(pointer, Buffer::Recycle);
    bufferPoolMutex.unlock();
}
//------------------------------------------------------------------------------
void Buffer::Clean()
{
    bufferPoolAllocated.exchange(0, std::memory_order_acq_rel);
    bufferPoolTerminate = true;
    bufferPoolMutex.lock();
    bufferPool.clear();
    bufferPoolMutex.unlock();
    bufferPoolTerminate = false;
}
//------------------------------------------------------------------------------
Buffer Buffer::Get(size_t size)
{
    Buffer buffer;
    if (bufferPool.empty() == false)
    {
        bufferPoolMutex.lock();
        if (bufferPool.empty() == false)
        {
            bufferPool.back().swap(buffer);
            bufferPool.pop_back();
        }
        bufferPoolMutex.unlock();
    }
    Buffer::element_type* pointer = buffer.get();
    if (pointer == nullptr)
    {
        pointer = new Buffer::element_type;
        if (pointer)
        {
            buffer.reset(pointer, Buffer::Recycle);
            bufferPoolAllocated.fetch_add(1, std::memory_order_acq_rel);
        }
    }
    if (pointer)
    {
        pointer->resize(size);
    }
    return buffer;
}
//------------------------------------------------------------------------------
int Buffer::Used()
{
    return bufferPoolAllocated - (int)bufferPool.size();
}
//------------------------------------------------------------------------------
int Buffer::Unused()
{
    return (int)bufferPool.size();
}
//------------------------------------------------------------------------------
