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
#include "cmft_impl.h"

#include "ninniku/dx11/DX11.h"
#include "ninniku/dx11/DX11Types.h"
#include "ninniku/image/cmft.h"

#include "../dx11/DX11_impl.h"
#include "../utils/log.h"

namespace ninniku {
    cmftImage::cmftImage()
        : _impl{ new cmftImageImpl() }
    {
    }

    cmftImage::~cmftImage() = default;

    cmftImageImpl::~cmftImageImpl()
    {
        if (_image.m_data != nullptr)
            imageUnload(_image);
    }

    void cmftImageImpl::AllocateMemory()
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

    TextureParamHandle cmftImageImpl::CreateTextureParam(const ETextureViews viewFlags) const
    {
        auto res = std::make_shared<TextureParam>();

        if (viewFlags == ETextureViews::TV_None) {
            LOGE << "TextureParam view flags cannot be ETextureViews::TV_None";
        } else {
            res->arraySize = CUBEMAP_NUM_FACES;
            res->depth = 1;
            res->format = 2; // DXGI_FORMAT_R32G32B32A32_FLOAT
            res->height = res->width = imageGetCubemapFaceSize(_image);
            res->imageDatas = GetInitializationData();
            res->numMips = 1;
            res->viewflags = viewFlags;
        }

        return res;
    }

    const bool cmftImageImpl::Load(const std::string& path)
    {
        auto fmt = boost::format("cmftImageImpl::Load, Path=\"%1%\"") % path;
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

    void cmftImageImpl::InitializeFromTextureObject(DX11Handle& dx, const TextureHandle& srcTex)
    {
        // we want to enforce 1:1 for now
        assert(srcTex->desc->width == srcTex->desc->height);

        // allocate memory
        _image.m_width = srcTex->desc->width;
        _image.m_height = srcTex->desc->height;
        _image.m_format = cmft::TextureFormat::RGBA32F;
        _image.m_numFaces = CUBEMAP_NUM_FACES;
        _image.m_numMips = srcTex->desc->numMips;

        auto fmt = boost::format("cmftImageImpl::InitializeFromTextureObject with Width=%1%, Height=%2%, Array=%3%, Mips=%4%") % _image.m_width % _image.m_height % (int)_image.m_numFaces % (int)_image.m_numMips;
        LOG << boost::str(fmt);

        AllocateMemory();

        auto marker = dx->CreateDebugMarker("ImageFromTextureObject");

        // we have to copy each mip with a read back texture or the same size for each face
        for (uint32_t mip = 0; mip < srcTex->desc->numMips; ++mip) {
            auto param = std::make_shared<TextureParam>();

            param->width = srcTex->desc->width >> mip;
            param->height = srcTex->desc->height >> mip;
            param->format = srcTex->desc->format;
            param->numMips = 1;
            param->arraySize = 1;
            param->viewflags = ETextureViews::TV_CPU_READ;

            auto readBack = dx->CreateTexture(param);

            CopySubresourceParam params = {};
            params.src = srcTex.get();
            params.srcMip = mip;
            params.dst = readBack.get();

            for (uint32_t face = 0; face < CUBEMAP_NUM_FACES; ++face) {
                params.srcFace = face;

                auto indexes = dx->CopySubresource(params);
                auto mapped = dx->GetImpl()->MapTexture(readBack, std::get<1>(indexes));

                UpdateSubImage(face, mip, (uint8_t*)mapped->GetData(), mapped->GetRowPitch());
            }
        }
    }

    const std::vector<SubresourceParam> cmftImageImpl::GetInitializationData() const
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

    const std::tuple<uint8_t*, uint32_t> cmftImageImpl::GetData() const
    {
        return std::make_tuple(static_cast<uint8_t*>(_image.m_data), _image.m_dataSize);
    }

    bool cmftImageImpl::SaveImage(const std::string& path)
    {
        auto fmt = boost::format("Saving DDS with cmftImageImpl file \"%1%\"") % path;
        LOG << boost::str(fmt);

        return cmft::imageSave(_image, path.c_str(), cmft::ImageFileType::Enum::DDS, cmft::OutputType::Enum::Cubemap, cmft::TextureFormat::Enum::RGBA32F, true);
    }

    bool cmftImageImpl::SaveImageFaceList(const std::string& path)
    {
        auto fmt = boost::format("Saving DDS face list with cmftImageImpl files \"%1%\"") % path;
        LOG << boost::str(fmt);

        return cmft::imageSave(_image, path.c_str(), cmft::ImageFileType::Enum::DDS, cmft::OutputType::Enum::FaceList, cmft::TextureFormat::Enum::RGBA32F, true);
    }

    void cmftImageImpl::UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch)
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