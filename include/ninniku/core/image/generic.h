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

#include "../../export.h"
#include "image.h"

namespace ninniku
{
    class genericImageImpl;

    class genericImage final : public Image
    {
        // no copy of any kind allowed
        genericImage(const genericImage&) = delete;
        genericImage& operator=(genericImage&) = delete;
        genericImage(genericImage&&) = delete;
        genericImage& operator=(genericImage&&) = delete;

    public:
        NINNIKU_API genericImage();
        NINNIKU_API ~genericImage();

        NINNIKU_API TextureParamHandle CreateTextureParam(const EResourceViews viewFlags) const override;
        [[nodiscard]] NINNIKU_API bool Load(const std::string_view&) override;
        [[nodiscard]] NINNIKU_API bool LoadRaw(const void* pData, const size_t size, const uint32_t width, const uint32_t height, const int32_t format) override;
        NINNIKU_API const std::tuple<uint8_t*, uint32_t> GetData() const override;

        // Used when transferring data back from the GPU
        [[nodiscard]] NINNIKU_API bool InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureHandle& srcTex) override;

        NINNIKU_API virtual const SizeFixResult IsRequiringFix() const override;

    private:
        std::unique_ptr<genericImageImpl> impl_;
    };
} // namespace ninniku
