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
#include "ninniku/image/cmft.h"

#include "cmft_impl.h"

namespace ninniku {
    TextureParamHandle cmftImage::CreateTextureParam(const ETextureViews viewFlags) const
    {
        return _impl->CreateTextureParam(viewFlags);
    }

    const bool cmftImage::Load(const std::string& path)
    {
        return _impl->Load(path);
    }

    const std::tuple<uint8_t*, uint32_t> cmftImage::GetData() const
    {
        return _impl->GetData();
    }

    void cmftImage::InitializeFromTextureObject(DX11Handle& dx, const TextureHandle& srcTex)
    {
        return _impl->InitializeFromTextureObject(dx, srcTex);
    }

    const SizeFixResult cmftImage::IsRequiringFix() const
    {
        return _impl->IsRequiringFix();
    }

    bool cmftImage::SaveImageCubemap(const std::string& path, uint32_t format)
    {
        return _impl->SaveImageCubemap(path, format);
    }

    bool cmftImage::SaveImageFaceList(const std::string& path, uint32_t format)
    {
        return _impl->SaveImageFaceList(path, format);
    }
}