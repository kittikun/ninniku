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

#include "dx11/DX11Types.h"

namespace ninniku
{
    class DX11;
    class cmftImage;

    class Processor
    {
    public:
        Processor(const std::shared_ptr<DX11>&);

        bool ProcessImage(const boost::filesystem::path&);

    private:
        std::unique_ptr<cmftImage> ImageFromTextureObject(const std::unique_ptr<TextureObject>& srcTex);

        // programs
        void ColorMips();
        void GenerateMips(const std::unique_ptr<TextureObject>& srcTex);
        void GeneratePreIntegratedCubemap(const std::unique_ptr<TextureObject>& srcTex3);
        void TestCubemapDirToTexture2DArray(const std::unique_ptr<TextureObject>& original);

    private:
        std::shared_ptr<DX11> _dx;
    };
} // namespace ninniku