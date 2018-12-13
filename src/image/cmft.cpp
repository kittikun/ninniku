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
#include "cmft.h"

#include "../utils/log.h"
#include "../utils/misc.h"
#include "../utils/mathUtils.h"

namespace ninniku {
    cmftImage::cmftImage(uint32_t width, uint32_t height, uint32_t numMips)
    {
        // for now we want aspect of 1:1
        assert(width == height);

        _image.m_width = width;
        _image.m_height = height;
        _image.m_format = cmft::TextureFormat::RGBA32F;
        _image.m_numFaces = CUBEMAP_NUM_FACES;
        _image.m_numMips = numMips;

        AllocateMemory();
    }

    cmftImage::~cmftImage()
    {
        if (_image.m_data != nullptr)
            imageUnload(_image);
    }

    void cmftImage::AllocateMemory()
    {
        // Alloc dst data.
        const uint32_t bytesPerPixel = 16; // RGBA32F;
        uint32_t dstDataSize = 0;
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];

        for (uint8_t face = 0; face < _image.m_numFaces; ++face) {
            for (uint8_t mip = 0; mip < _image.m_numMips; ++mip) {
                dstOffsets[face][mip] = dstDataSize;
                uint32_t dstMipSize = std::max(UINT32_C(1), _image.m_width >> mip);
                dstDataSize += dstMipSize * dstMipSize * bytesPerPixel;
            }
        }

        _image.m_data = CMFT_ALLOC(cmft::g_allocator, dstDataSize);
        _image.m_dataSize = dstDataSize;
    }

    TextureParam cmftImage::CreateTextureParam(uint8_t viewFlags) const
    {
        TextureParam res = {};

        res.arraySize = CUBEMAP_NUM_FACES;
        res.depth = 0;
        res.format = 2; // DXGI_FORMAT_R32G32B32A32_FLOAT
        res.height = res.width = imageGetCubemapFaceSize(_image);
        res.imageDatas = GetInitializationData();
        res.numMips = 1;
        res.viewflags = viewFlags;

        return res;
    }

    bool cmftImage::Load(const std::string& path)
    {
        bool imageLoaded = imageLoad(_image, path.c_str(), cmft::TextureFormat::RGBA32F)
                           || imageLoadStb(_image, path.c_str(), cmft::TextureFormat::RGBA32F);

        if (!imageLoaded) {
            LOGE << "Failed to load file";

            return false;
        }

        // Assemble cubemap.
        if (!imageIsCubemap(_image)) {
            if (imageIsCubeCross(_image)) {
                LOG << "Converting cube cross to cubemap.";
                imageCubemapFromCross(_image);
            } else if (imageIsLatLong(_image)) {
                LOG << "Converting latlong image to cubemap.";
                imageCubemapFromLatLong(_image);
            } else if (imageIsHStrip(_image)) {
                LOG << "Converting hstrip image to cubemap.";
                imageCubemapFromStrip(_image);
            } else if (imageIsVStrip(_image)) {
                LOG << "Converting vstrip image to cubemap.";
                imageCubemapFromStrip(_image);
            } else if (imageIsOctant(_image)) {
                LOG << "Converting octant image to cubemap.";
                imageCubemapFromOctant(_image);
            } else {
                LOGE << "Image is not cubemap(6 faces), cubecross(ratio 3:4 or 4:3), latlong(ratio 2:1), hstrip(ratio 6:1), vstrip(ration 1:6)";

                return false;
            }
        }

        if (!imageIsCubemap(_image)) {
            LOGE << "Conversion failed.";

            return false;
        }

        return true;
    }

    /// <summary>
    /// Check if a cubemap is a power of 2 and return a 2 item tuple
    /// 0 bool: need fix
    /// 1 uint32: new face size
    /// </summary>
    std::tuple<bool, uint32_t> cmftImage::IsRequiringFix()
    {
        auto faceSize = imageGetCubemapFaceSize(_image);
        auto tx = _image.m_width;
        auto ty = _image.m_height;
        auto res = false;

        if (tx != faceSize) {
            auto fmt = boost::format("Width of %1% does not match face size of %2%, will resize accordingly") % tx % faceSize;
            LOGW << boost::str(fmt);
            res = true;
        }

        if (ty != faceSize) {
            auto fmt = boost::format("Height of %1% does not match face size of %2%, will resize accordingly") % ty % faceSize;
            LOGW << boost::str(fmt);
            res = true;
        }

        auto isPow2 = IsPow2(faceSize);

        if (!isPow2) {
            auto fmt = boost::format("Face size %1%x%1% is not a power of 2") % faceSize;
            LOGW << boost::str(fmt);

            faceSize = NearestPow2Floor(faceSize);
            res = true;
        }

        return std::make_tuple(res, faceSize);
    }

    std::vector<SubresourceParam> cmftImage::GetInitializationData() const
    {
        std::array<uint32_t, CUBEMAP_NUM_FACES> offsets;

        cmft::imageGetFaceOffsets(offsets.data(), _image);

        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;
        std::vector<SubresourceParam> res(CUBEMAP_NUM_FACES);

        for (auto i = 0; i < CUBEMAP_NUM_FACES; ++i) {
            res[i].data = static_cast<void*>(static_cast<uint8_t*>(_image.m_data) + offsets[i]);
            res[i].rowPitch = _image.m_width * bytesPerPixel;
            res[i].depthPitch = 0;
        }

        return res;
    }

    /// <summary>
    /// Note that this just allocate memory, data needs to be filled with UpdateSubImage afterwards
    /// </summary>
    void cmftImage::ResizeImage(uint32_t size)
    {
        // Note that this just allocate memory, data needs to be filled afterwards
        LOG << "Resizing image...";

        _image.m_width = size;
        _image.m_height = size;

        CMFT_FREE(cmft::g_allocator, _image.m_data);
        _image.m_data = nullptr;

        AllocateMemory();
    }

    void cmftImage::SaveImage(const std::string& path)
    {
        cmft::imageSave(_image, path.c_str(), cmft::ImageFileType::Enum::DDS, cmft::OutputType::Enum::Cubemap, cmft::TextureFormat::Enum::RGBA32F, true);
    }

    void cmftImage::SaveImageFaceList(const std::string& path)
    {
        cmft::imageSave(_image, path.c_str(), cmft::ImageFileType::Enum::DDS, cmft::OutputType::Enum::FaceList, cmft::TextureFormat::Enum::RGBA32F, true);
    }

    void cmftImage::UpdateSubImage(uint32_t dstFace, uint32_t dstMip, uint8_t* newData, uint32_t newRowPitch)
    {
        // get the right offset
        const uint32_t bytesPerPixel = 16; // RGBA32F;
        uint32_t size = _image.m_width;
        uint32_t dstDataSize = 0;
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        uint32_t dstMipSize[CUBE_FACE_NUM][MAX_MIP_NUM];

        for (uint8_t face = 0; face < _image.m_numFaces; ++face) {
            for (uint8_t mip = 0; mip < _image.m_numMips; ++mip) {
                uint32_t mipSize = std::max(UINT32_C(1), size >> mip);

                dstMipSize[face][mip] = mipSize * mipSize * bytesPerPixel;
                dstOffsets[face][mip] = dstDataSize;
                dstDataSize += dstMipSize[face][mip];
            }
        }

        auto offset = (uint8_t*)_image.m_data + dstOffsets[dstFace][dstMip];

        // row pitch from dx11 can be larger than for the image so we have to do each row manually
        uint32_t mipSize = size >> dstMip;

        for (uint32_t row = 0; row < mipSize; ++row) {
            auto imgPitch = mipSize * bytesPerPixel;
            auto imgOffset = imgPitch * row;
            auto newOffset = newRowPitch * row;

            memcpy_s(offset + imgOffset, imgPitch, newData + newOffset, std::min(newRowPitch, imgPitch));
        }
    }
} // namespace ninniku