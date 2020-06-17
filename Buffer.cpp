//==============================================================================
// ConcurrencyNetworkFramework : Event Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include "FrameworkHeader.h"
#include "Buffer.h"

static std::vector<Buffer> bufferPool;
static std::mutex bufferPoolMutex;
static bool bufferPoolTerminate;
//------------------------------------------------------------------------------
void Buffer::Recycle(Buffer::element_type* pointer)
{
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
    bufferPoolMutex.lock();
    if (bufferPool.empty() == false)
    {
        bufferPool.back().swap(buffer);
        bufferPool.pop_back();
    }
    bufferPoolMutex.unlock();
    Buffer::element_type* pointer = buffer.get();
    if (pointer == nullptr)
    {
        pointer = new Buffer::element_type;
        buffer.reset(pointer, Buffer::Recycle);
    }
    if (pointer)
    {
        pointer->resize(size);
    }
    return buffer;
}
//------------------------------------------------------------------------------
size_t Buffer::Count()
{
    return bufferPool.size();
}
//------------------------------------------------------------------------------
