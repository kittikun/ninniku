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

#include "ninniku/core/renderer/renderdevice.h"

namespace ninniku
{
    class DX12 : public RenderDevice
    {
    public:
        virtual ERenderer GetType() const { return ERenderer::RENDERER_DX12; }

        virtual void CopyBufferResource(const CopyBufferSubresourceParam& params) const;
        virtual std::tuple<uint32_t, uint32_t> CopyTextureSubresource(const CopyTextureSubresourceParam& params) const;
        virtual BufferHandle CreateBuffer(const BufferParamHandle& params);
        virtual BufferHandle CreateBuffer(const BufferHandle& src);
        virtual CommandHandle CreateCommand() const;
        virtual DebugMarkerHandle CreateDebugMarker(const std::string& name) const;
        virtual TextureHandle CreateTexture(const TextureParamHandle& params);
        virtual bool Dispatch(const CommandHandle& cmd) const;
        virtual bool Initialize(const std::vector<std::string>& shaderPaths, const bool isWarp);
        virtual bool LoadShader(const std::string& name, const void* pData, const size_t size);
        virtual MappedResourceHandle MapBuffer(const BufferHandle& bObj);
        virtual MappedResourceHandle MapTexture(const TextureHandle& tObj, const uint32_t index);
        virtual bool UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size);

        virtual const SamplerState* GetSampler(ESamplerState sampler) const;
    };
} // namespace ninniku
