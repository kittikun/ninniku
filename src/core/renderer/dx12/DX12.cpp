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

#include "pch.h"
#include "DX12.h"

#include "../../../utils/log.h"

namespace ninniku
{
    void DX12::CopyBufferResource(const CopyBufferSubresourceParam& params) const
    {
        throw std::exception("not implemented");
    }

    std::tuple<uint32_t, uint32_t> DX12::CopyTextureSubresource(const CopyTextureSubresourceParam& params) const
    {
        throw std::exception("not implemented");
    }

    BufferHandle DX12::CreateBuffer(const BufferParamHandle& params)
    {
        throw std::exception("not implemented");
    }

    BufferHandle DX12::CreateBuffer(const BufferHandle& src)
    {
        throw std::exception("not implemented");
    }

    CommandHandle DX12::CreateCommand() const
    {
        throw std::exception("not implemented");
    }

    DebugMarkerHandle DX12::CreateDebugMarker(const std::string& name) const
    {
        throw std::exception("not implemented");
    }

    TextureHandle DX12::CreateTexture(const TextureParamHandle& params)
    {
        throw std::exception("not implemented");
    }

    bool DX12::Dispatch(const CommandHandle& cmd) const
    {
        throw std::exception("not implemented");
    }

    bool DX12::Initialize(const std::vector<std::string>& shaderPaths, const bool isWarp)
    {
        throw std::exception("not implemented");
    }

    bool DX12::LoadShader(const std::string& name, const void* pData, const size_t size)
    {
        throw std::exception("not implemented");
    }

    MappedResourceHandle DX12::MapBuffer(const BufferHandle& bObj)
    {
        throw std::exception("not implemented");
    }

    MappedResourceHandle DX12::MapTexture(const TextureHandle& tObj, const uint32_t index)
    {
        throw std::exception("not implemented");
    }

    bool DX12::UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size)
    {
        throw std::exception("not implemented");
    }
} // namespace ninniku