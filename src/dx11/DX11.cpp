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
#include "ninniku/dx11/DX11.h"

#include "DX11_impl.h"

namespace ninniku {
    const std::tuple<uint32_t, uint32_t> DX11::CopySubresource(const CopySubresourceParam& params) const
    {
        return _impl->CopySubresource(params);
    }

    const DebugMarkerHandle DX11::CreateDebugMarker(const std::string& name) const
    {
        return _impl->CreateDebugMarker(name);
    }

    TextureHandle DX11::CreateTexture(const TextureParamHandle& param)
    {
        return _impl->CreateTexture(param);
    }

    const bool DX11::Dispatch(const Command& cmd) const
    {
        return _impl->Dispatch(cmd);
    }

    const bool DX11::UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size)
    {
        return _impl->UpdateConstantBuffer(name, data, size);
    }

    const DX11SamplerState& DX11::GetSampler(ESamplerState sampler) const
    {
        return _impl->GetSampler(sampler);
    }
}