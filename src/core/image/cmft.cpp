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

#include "ninniku/core/image/cmft.h"

#include "cmft_impl.h"

namespace ninniku
{
    TextureParamHandle cmftImage::CreateTextureParam(const EResourceViews viewFlags) const
    {
        return impl_->CreateTextureParam(viewFlags);
    }

    bool cmftImage::Load(const std::string_view& path)
    {
        return impl_->Load(path);
    }

    bool cmftImage::LoadRaw(const void* pData, const size_t size, const uint32_t width, const uint32_t height, const int32_t format)
    {
        return impl_->LoadRaw(pData, size, width, height, format);
    }

    const std::tuple<uint8_t*, uint32_t> cmftImage::GetData() const
    {
        return impl_->GetData();
    }

    bool cmftImage::InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureObject* srcTex)
    {
        return impl_->InitializeFromTextureObject(dx, srcTex);
    }

    bool cmftImage::InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureObject* srcTex, const uint32_t cubeIndex)
    {
        return impl_->InitializeFromTextureObject(dx, srcTex, cubeIndex);
    }

    const SizeFixResult cmftImage::IsRequiringFix() const
    {
        return impl_->IsRequiringFix();
    }

    bool cmftImage::SaveImage(const std::string_view& path, SaveType type)
    {
        return impl_->SaveImage(path, type);
    }
} // namespace ninniku