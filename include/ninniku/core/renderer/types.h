// Copyright(c) 2018-2019 Kitti Vongsay
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "../../types.h"

#include <memory>

namespace ninniku
{
    //////////////////////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////////////////////
    class BufferObject
    {
    public:
        virtual ~BufferObject() = default;

    public:
        // Initial desc that was used to create the resource
        std::shared_ptr<const BufferParam> desc;
    };

    using BufferHandle = std::unique_ptr<const BufferObject>;

    //////////////////////////////////////////////////////////////////////////
    // Commands
    //////////////////////////////////////////////////////////////////////////

    struct Command
    {
        // no copy of any kind allowed
        Command(const Command&) = delete;
        Command& operator=(Command&) = delete;
        Command(Command&&) = delete;
        Command& operator=(Command&&) = delete;

    public:
        ~Command() = default;
    };

    //////////////////////////////////////////////////////////////////////////
    // Debug
    //////////////////////////////////////////////////////////////////////////

    class NINNIKU_API DebugMarker
    {
        // no copy of any kind allowed
        DebugMarker(const DebugMarker&) = delete;
        DebugMarker& operator=(DebugMarker&) = delete;
        DebugMarker(DebugMarker&&) = delete;
        DebugMarker& operator=(DebugMarker&&) = delete;

    public:
        virtual ~DebugMarker() = default;

    protected:
        DebugMarker() = default;
    };

    using DebugMarkerHandle = std::unique_ptr<const DebugMarker>;

    //////////////////////////////////////////////////////////////////////////
    // Textures
    //////////////////////////////////////////////////////////////////////////
    class TextureObject
    {
        // no copy of any kind allowed
        TextureObject(const TextureObject&) = delete;
        TextureObject& operator=(TextureObject&) = delete;
        TextureObject(TextureObject&&) = delete;
        TextureObject& operator=(TextureObject&&) = delete;

    public:
        virtual ~TextureObject() = default;

    protected:
        TextureObject() = default;

    public:
        // Initial desc that was used to create the resource
        std::shared_ptr<const TextureParam> desc;
    };

    using TextureHandle = std::unique_ptr<const TextureObject>;

    //////////////////////////////////////////////////////////////////////////
    // GPU to CPU readback
    //////////////////////////////////////////////////////////////////////////

    class MappedResource
    {
        // no copy of any kind allowed
        MappedResource(const MappedResource&) = delete;
        MappedResource& operator=(MappedResource&) = delete;
        MappedResource(MappedResource&&) = delete;
        MappedResource& operator=(MappedResource&&) = delete;

    public:
        virtual ~MappedResource();

        virtual void* GetData() const = 0;
        virtual uint32_t GetRowPitch() const = 0;

    protected:
        MappedResource() = default;
    };

    using MappedResourceHandle = std::unique_ptr<const MappedResource>;
} // namespace ninniku