//==============================================================================
// ConcurrencyNetworkFramework : Event Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#include "FrameworkHeader.h"
#include "Buffer.h"

std::vector<BufferPtr> bufferPool;
std::mutex bufferPoolMutex;
//------------------------------------------------------------------------------
BufferPtr BufferPool::Get(size_t size)
{
    BufferPtr bufferPtr;
    bufferPoolMutex.lock();
    if (bufferPool.empty() == false)
    {
        bufferPool.back().swap(bufferPtr);
        bufferPool.pop_back();
    }
    bufferPoolMutex.unlock();
    if (bufferPtr)
    {
        (*bufferPtr).resize(size);
        return bufferPtr;
    }
    return bufferPtr.make_shared(size);
}
//------------------------------------------------------------------------------
void BufferPool::Push(BufferPtr& bufferPtr)
{
    if (bufferPtr.use_count() != 1)
        return;
    bufferPoolMutex.lock();
    bufferPool.emplace_back(bufferPtr);
    bufferPoolMutex.unlock();
}
//------------------------------------------------------------------------------
size_t BufferPool::Count()
{
    return bufferPool.size();
}
//------------------------------------------------------------------------------
