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

namespace ninniku {
#ifdef NINNIKU_EXPORT
    class DX11Impl;
#endif

    class DX11
    {
        // no copy of any kind allowed
        DX11(const DX11&) = delete;
        DX11& operator=(DX11&) = delete;
        DX11(DX11&&) = delete;
        DX11& operator=(DX11&&) = delete;

    public:
        NINNIKU_API DX11();

        NINNIKU_API std::tuple<uint32_t, uint32_t> CopySubresource(const CopySubresourceParam& params) const;
        NINNIKU_API std::unique_ptr<DebugMarker> CreateDebugMarker(const std::string& name) const;
        NINNIKU_API std::unique_ptr<TextureObject> CreateTexture(const TextureParam& param);
        NINNIKU_API bool Dispatch(const Command& cmd) const;
        NINNIKU_API bool Initialize(const std::string& shaderPath, const bool isWarp);
        NINNIKU_API std::unique_ptr<MappedResource> MapTexture(const std::unique_ptr<TextureObject>& tObj, const uint32_t index);
        NINNIKU_API bool UpdateConstantBuffer(const std::string& name, void* data, const uint32_t size);

        NINNIKU_API const DX11SamplerState& GetSampler(ESamplerState sampler) const;

#ifdef NINNIKU_EXPORT
        DX11Impl* GetImpl() { return _impl.get(); }

    private:
        std::unique_ptr<DX11Impl> _impl;
#endif
    };
} // namespace ninniku
