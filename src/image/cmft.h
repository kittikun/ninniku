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

#include "image.h"

#include <cmft/image.h>

namespace ninniku {
    class cmftImage : public Image
    {
    public:
        cmftImage() = default;
        cmftImage(uint32_t width, uint32_t height, uint32_t numMips);

        ~cmftImage();

        TextureParam CreateTextureParam(uint8_t viewFlags) const override;
        bool Load(const std::string&) override;

        std::tuple<bool, uint32_t> IsRequiringFix();

        void ResizeImage(uint32_t size);
        void UpdateSubImage(uint32_t dstFace, uint32_t dstMip, uint8_t* newData, uint32_t newRowPitch);

        void SaveImage(const std::string&);
        void SaveImageFaceList(const std::string&);

    protected:
        std::vector<SubresourceParam> GetInitializationData() const override;

    private:
        void AllocateMemory();

    private:
        cmft::Image _image;
    };
} // namespace ninniku
