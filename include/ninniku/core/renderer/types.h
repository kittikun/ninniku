// Copyright(c) 2018-2020 Kitti Vongsay
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
#include "../../utils.h"

#include <array>
#include <memory>
#include <unordered_map>

namespace ninniku
{
    //////////////////////////////////////////////////////////////////////////
    // Shader Resources
    //////////////////////////////////////////////////////////////////////////
    class ShaderResourceView : private NonCopyableBase
    {
    };

    using SRVHandle = std::unique_ptr<ShaderResourceView>;

    class UnorderedAccessView : private NonCopyableBase
    {
    };

    using UAVHandle = std::unique_ptr<UnorderedAccessView>;

    class SamplerState : private NonCopyableBase
    {
    };

    using SSHandle = std::unique_ptr<SamplerState>;

    //////////////////////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////////////////////
    class BufferObject : private NonCopyableBase
    {
    public:
        // This will only be filled when copied from another buffer (they are mapped)
        // Add support for initial data later
        virtual const std::vector<uint32_t>& GetData() const = 0;

        virtual const ShaderResourceView* GetSRV() const = 0;
        virtual const UnorderedAccessView* GetUAV() const = 0;

    public:
        // Initial desc that was used to create the resource
        std::shared_ptr<const BufferParam> desc;
    };

    using BufferHandle = std::unique_ptr<const BufferObject>;

    //////////////////////////////////////////////////////////////////////////
    // Debug
    //////////////////////////////////////////////////////////////////////////

    class NINNIKU_API DebugMarker : private NonCopyableBase
    {
    };

    using DebugMarkerHandle = std::unique_ptr<const DebugMarker>;

    //////////////////////////////////////////////////////////////////////////
    // Commands
    //////////////////////////////////////////////////////////////////////////

    class Command : private NonCopyableBase
    {
    public:
        std::string shader;
        std::string cbufferStr;
        std::array<uint32_t, 3> dispatch;
        std::unordered_map<std::string, const ShaderResourceView*> srvBindings;
        std::unordered_map<std::string, const UnorderedAccessView*> uavBindings;
        std::unordered_map<std::string, const SamplerState*> ssBindings;
    };

    using CommandHandle = std::unique_ptr<Command>;

    //////////////////////////////////////////////////////////////////////////
    // Textures
    //////////////////////////////////////////////////////////////////////////
    class TextureObject : private NonCopyableBase
    {
    public:
        virtual const ShaderResourceView* GetSRVDefault() const = 0;
        virtual const ShaderResourceView* GetSRVCube() const = 0;
        virtual const ShaderResourceView* GetSRVCubeArray() const = 0;
        virtual const ShaderResourceView* GetSRVArray(uint32_t index) const = 0;
        virtual const UnorderedAccessView* GetUAV(uint32_t index) const = 0;

    public:
        // Initial desc that was used to create the resource
        std::shared_ptr<const TextureParam> desc;
    };

    using TextureHandle = std::unique_ptr<const TextureObject>;

    //////////////////////////////////////////////////////////////////////////
    // GPU to CPU readback
    //////////////////////////////////////////////////////////////////////////

    class MappedResource : private NonCopyableBase
    {
    public:
        virtual void* GetData() const = 0;
        virtual uint32_t GetRowPitch() const = 0;
    };

    using MappedResourceHandle = std::unique_ptr<const MappedResource>;
} // namespace ninniku