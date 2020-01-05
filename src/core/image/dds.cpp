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

#include "pch.h"
#include "ninniku/core/image/dds.h"

#include "dds_impl.h"

namespace ninniku
{
    TextureParamHandle ddsImage::CreateTextureParam(const EResourceViews viewFlags) const
    {
        return _impl->CreateTextureParam(viewFlags);
    }

    bool ddsImage::Load(const std::string& path)
    {
        return _impl->Load(path);
    }

    bool ddsImage::LoadRaw(const void* pData, const size_t size)
    {
        return _impl->LoadRaw(pData, size);
    }

    bool ddsImage::LoadRaw(const void* pData, const size_t size, const uint32_t width, const uint32_t height, const int32_t format)
    {
        return _impl->LoadRaw(pData, size, width, height, format);
    }

    const std::tuple<uint8_t*, uint32_t> ddsImage::GetData() const
    {
        return _impl->GetData();
    }

    void ddsImage::InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureHandle& srcTex)
    {
        return _impl->InitializeFromTextureObject(dx, srcTex);
    }

    const SizeFixResult ddsImage::IsRequiringFix() const
    {
        return _impl->IsRequiringFix();
    }

    bool ddsImage::SaveImage(const std::string& path)
    {
        return _impl->SaveImage(path);
    }

    bool ddsImage::SaveCompressedImage(const std::string& path, RenderDeviceHandle& dx, DXGI_FORMAT format)
    {
        return _impl->SaveCompressedImage(path, dx, format);
    }
} // namespace ninniku