//==============================================================================
// ConcurrencyNetworkFramework : Event Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/ConcurrencyNetworkFramework
//==============================================================================
#pragma once

typedef std::shared_ptr<std::vector<char>> BufferPtr;

class BufferPool
{
public:
    static BufferPtr Get(size_t size);
    static void Push(BufferPtr& bufferPtr);
    static size_t Count();
};
