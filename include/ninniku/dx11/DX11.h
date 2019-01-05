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

#include "../export.h"
#include "DX11Types.h"

#include <d3d11shader.h>

namespace ninniku
{
    class DX11Impl;

    class DX11
    {
        // no copy of any kind allowed
        DX11(const DX11&) = delete;
        DX11& operator=(DX11&) = delete;
        DX11(DX11&&) = delete;
        DX11& operator=(DX11&&) = delete;

    public:
        NINNIKU_API DX11();
        NINNIKU_API ~DX11();

        NINNIKU_API const std::tuple<uint32_t, uint32_t> CopySubresource(const CopySubresourceParam& params) const;
        NINNIKU_API const DebugMarkerHandle CreateDebugMarker(const std::string& name) const;
        NINNIKU_API const TextureHandle CreateTexture(const TextureParam& param);
        NINNIKU_API const bool Dispatch(const Command& cmd) const;
        NINNIKU_API const bool UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size);

        NINNIKU_API const DX11SamplerState& GetSampler(ESamplerState sampler) const;

#ifdef NINNIKU_EXPORT
        DX11Impl* GetImpl() { return _impl.get(); }
#endif

    private:
        std::unique_ptr<DX11Impl> _impl;
    };

    NINNIKU_API DX11Handle& GetRenderer();
} // namespace ninniku
