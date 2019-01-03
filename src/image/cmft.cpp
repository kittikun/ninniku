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

#include "ninniku/dx11/DX11.h"
#include "ninniku/dx11/DX11Types.h"

#include "../utils/log.h"
#include "../utils/misc.h"
#include "../utils/mathUtils.h"

namespace ninniku {
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

        if (_image.m_data != nullptr)
            imageUnload(_image);

        _image.m_data = CMFT_ALLOC(cmft::g_allocator, dstDataSize);
        _image.m_dataSize = dstDataSize;
    }

    TextureParam cmftImage::CreateTextureParam(uint8_t viewFlags) const
    {
        TextureParam res = {};

        if (viewFlags == TV_None) {
            LOGE << "TextureParam view flags cannot be 0";
        } else {
            res.arraySize = CUBEMAP_NUM_FACES;
            res.depth = 1;
            res.format = 2; // DXGI_FORMAT_R32G32B32A32_FLOAT
            res.height = res.width = imageGetCubemapFaceSize(_image);
            res.imageDatas = GetInitializationData();
            res.numMips = 1;
            res.viewflags = viewFlags;
        }

        return res;
    }

    bool cmftImage::Load(const std::string& path)
    {
        auto fmt = boost::format("cmftImage::Load, Path=\"%1%\"") % path;
        LOG << boost::str(fmt);

        bool imageLoaded = imageLoad(_image, path.c_str(), cmft::TextureFormat::RGBA32F) || imageLoadStb(_image, path.c_str(), cmft::TextureFormat::RGBA32F);

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

    void cmftImage::InitializeFromTextureObject(std::unique_ptr<DX11>& dx, const std::unique_ptr<TextureObject>& srcTex)
    {
        // we want to enforce 1:1 for now
        assert(srcTex->desc.width == srcTex->desc.height);

        // allocate memory
        _image.m_width = srcTex->desc.width;
        _image.m_height = srcTex->desc.height;
        _image.m_format = cmft::TextureFormat::RGBA32F;
        _image.m_numFaces = CUBEMAP_NUM_FACES;
        _image.m_numMips = srcTex->desc.numMips;

        auto fmt = boost::format("cmftImage::InitializeFromTextureObject with Width=%1%, Height=%2%, Array=%3%, Mips=%4%") % _image.m_width % _image.m_height % (int)_image.m_numFaces % (int)_image.m_numMips;
        LOG << boost::str(fmt);

        AllocateMemory();

        auto marker = dx->CreateDebugMarker("ImageFromTextureObject");

        // we have to copy each mip with a read back texture or the same size for each face
        for (uint32_t mip = 0; mip < srcTex->desc.numMips; ++mip) {
            TextureParam param = {};

            param.width = srcTex->desc.width >> mip;
            param.height = srcTex->desc.height >> mip;
            param.format = srcTex->desc.format;
            param.numMips = 1;
            param.arraySize = 1;
            param.viewflags = TV_CPU_READ;

            auto readBack = dx->CreateTexture(param);

            CopySubresourceParam params = {};
            params.src = srcTex.get();
            params.srcMip = mip;
            params.dst = readBack.get();

            for (uint32_t face = 0; face < CUBEMAP_NUM_FACES; ++face) {
                params.srcFace = face;

                auto indexes = dx->CopySubresource(params);
                auto mapped = dx->MapTexture(readBack, std::get<1>(indexes));

                UpdateSubImage(face, mip, (uint8_t*)mapped->GetData(), mapped->GetRowPitch());
            }
        }
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

    std::tuple<uint8_t*, uint32_t> cmftImage::GetData() const
    {
        return std::make_tuple(static_cast<uint8_t*>(_image.m_data), _image.m_dataSize);
    }

    bool cmftImage::SaveImage(const std::string& path)
    {
        auto fmt = boost::format("Saving DDS with cmftImage file \"%1%\"") % path;
        LOG << boost::str(fmt);

        return cmft::imageSave(_image, path.c_str(), cmft::ImageFileType::Enum::DDS, cmft::OutputType::Enum::Cubemap, cmft::TextureFormat::Enum::RGBA32F, true);
    }

    bool cmftImage::SaveImageFaceList(const std::string& path)
    {
        auto fmt = boost::format("Saving DDS face list with cmftImage files \"%1%\"") % path;
        LOG << boost::str(fmt);

        return cmft::imageSave(_image, path.c_str(), cmft::ImageFileType::Enum::DDS, cmft::OutputType::Enum::FaceList, cmft::TextureFormat::Enum::RGBA32F, true);
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