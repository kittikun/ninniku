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

#include "../../ninniku.h"
#include "types.h"

namespace ninniku
{
    class RenderDevice : NonCopyableBase
    {
        // no copy of any kind allowed
        RenderDevice(const RenderDevice&) = delete;
        RenderDevice& operator=(RenderDevice&) = delete;
        RenderDevice(RenderDevice&&) = delete;
        RenderDevice& operator=(RenderDevice&&) = delete;

    public:
        virtual ~RenderDevice() = default;

        virtual ERenderer GetType() const = 0;

        virtual void CopyBufferResource(const CopyBufferSubresourceParam& params) const = 0;
        virtual std::tuple<uint32_t, uint32_t> CopyTextureSubresource(const CopyTextureSubresourceParam& params) const = 0;
        virtual BufferHandle CreateBuffer(const BufferParamHandle& params) = 0;
        virtual BufferHandle CreateBuffer(const BufferHandle& src) = 0;
        virtual CommandHandle CreateCommand() const = 0;
        virtual DebugMarkerHandle CreateDebugMarker(const std::string& name) const = 0;
        virtual TextureHandle CreateTexture(const TextureParamHandle& params) = 0;
        virtual bool Dispatch(const CommandHandle& cmd) const = 0;
        virtual bool Initialize(const std::vector<std::string>& shaderPaths, const bool isWarp) = 0;
        virtual bool LoadShader(const std::string& name, const void* pData, const size_t size) = 0;
        virtual MappedResourceHandle MapBuffer(const BufferHandle& bObj) = 0;
        virtual MappedResourceHandle MapTexture(const TextureHandle& tObj, const uint32_t index) = 0;
        virtual bool UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size) = 0;

        virtual const SamplerState* GetSampler(ESamplerState sampler) const = 0;

    protected:
        RenderDevice() = default;
    };

    // This is just used for Renderdoc
    struct RenderDeviceDeleter
    {
        void operator()(RenderDevice* value);
    };

    using RenderDeviceHandle = std::unique_ptr<RenderDevice, RenderDeviceDeleter>;

    NINNIKU_API RenderDeviceHandle& GetRenderer();
} // namespace ninniku