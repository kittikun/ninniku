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
#include "ninniku/core/image/cmft.h"

#include "../../dx11/DX11_impl.h"
#include "../../utils/log.h"
#include "../../utils/misc.h"

#include <boost/filesystem.hpp>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h>

namespace ninniku
{
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
        const uint32_t bytesPerPixel = GetBPPFromFormat(_image.m_format);
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

    TextureParamHandle cmftImageImpl::CreateTextureParamInternal(const uint8_t viewFlags) const
    {
        auto res = TextureParam::Create();

        res->arraySize = CUBEMAP_NUM_FACES;
        res->depth = 1;
        res->format = 2; // DXGI_FORMAT_R32G32B32A32_FLOAT
        res->height = res->width = imageGetCubemapFaceSize(_image);
        res->imageDatas = GetInitializationData();
        res->numMips = 1;
        res->viewflags = viewFlags;

        return std::move(res);
    }

    uint32_t cmftImageImpl::GetBPPFromFormat(cmft::TextureFormat::Enum format) const
    {
        // Assume that we are always using 4 channels
        // RGBA32F
        auto res = 16;

        if (format == cmft::TextureFormat::RGBA16F)
            res = 8;
        else if (format == cmft::TextureFormat::BGRA8)
            res = 4;

        return res;
    }

    cmft::ImageFileType::Enum cmftImageImpl::GetFiletypeFromFilename(const std::string& path)
    {
        auto res = cmft::ImageFileType::Count;

        boost::filesystem::path p{ path };

        if (!p.has_extension())
            LOGE << "Path passed to SaveImage must contain an extension";

        auto ext = p.extension();

        if (ext == ".dds")
            res = cmft::ImageFileType::Enum::DDS;
        else if (ext == ".hdr")
            res = cmft::ImageFileType::Enum::HDR;
        else if (ext == ".tga")
            res = cmft::ImageFileType::Enum::TGA;
        else
            LOGE << "Requested file extension is not supported by cmft";

        return res;
    }

    cmft::TextureFormat::Enum cmftImageImpl::GetFormatFromDXGIFormat(uint32_t format) const
    {
        auto res = cmft::TextureFormat::Enum::Null;

        switch (format) {
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                res = cmft::TextureFormat::Enum::RGBA16F;
                break;

            case DXGI_FORMAT_R32G32B32A32_FLOAT:
                res = cmft::TextureFormat::Enum::RGBA32F;
                break;

            case DXGI_FORMAT_R8G8B8A8_UNORM:
                res = cmft::TextureFormat::Enum::BGRA8;
                break;

            default:
                LOGE << "Unsupported format was passed to GetFormatFromDXGIFormat";
        }

        return res;
    }

    bool cmftImageImpl::LoadEXR(const std::string& path)
    {
        int width, height;
        float* rgba;
        const char* err;

        int ret = ::LoadEXR(&rgba, &width, &height, path.c_str(), &err);
        if (ret != TINYEXR_SUCCESS) {
            auto fmt = boost::format("cmftImageImpl::LoadEXR failed with: %1%") % err;
            LOG << boost::str(fmt);

            return false;
        }

        _image.m_width = width;
        _image.m_height = height;
        _image.m_dataSize = width * height * sizeof(float);
        _image.m_format = cmft::TextureFormat::RGBA32F;
        _image.m_numMips = 1;
        _image.m_numFaces = 1;
        _image.m_data = rgba;

        return true;
    }

    bool cmftImageImpl::LoadInternal(const std::string& path)
    {
        auto fmt = boost::format("cmftImageImpl::Load, Path=\"%1%\"") % path;
        LOG << boost::str(fmt);

        bool imageLoaded = false;

        if (boost::filesystem::path{ path }.extension() == ".exr")
            imageLoaded = LoadEXR(path);
        else
            imageLoaded = imageLoad(_image, path.c_str(), cmft::TextureFormat::RGBA32F) || imageLoadStb(_image, path.c_str(), cmft::TextureFormat::RGBA32F);

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
        _image.m_format = GetFormatFromDXGIFormat(srcTex->desc->format);
        _image.m_numFaces = srcTex->desc->arraySize;
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
            param->viewflags = EResourceViews::RV_CPU_READ;

            auto readBack = dx->CreateTexture(param);

            CopyTextureSubresourceParam params = {};
            params.src = srcTex.get();
            params.srcMip = mip;
            params.dst = readBack.get();

            for (uint32_t face = 0; face < CUBEMAP_NUM_FACES; ++face) {
                params.srcFace = face;

                auto indexes = dx->CopyTextureSubresource(params);
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

    bool cmftImageImpl::SaveImage(const std::string& path, cmftImage::SaveType type)
    {
        auto cmftFileType = GetFiletypeFromFilename(path);

        if (cmftFileType == cmft::ImageFileType::Count)
            return false;

        cmft::TextureFormat::Enum cmftFormat;

        switch (cmftFileType) {
            case cmft::ImageFileType::DDS:
            case cmft::ImageFileType::TGA:
                cmftFormat = cmft::TextureFormat::Enum::RGBA32F;
                break;

            case cmft::ImageFileType::HDR:
                cmftFormat = cmft::TextureFormat::Enum::RGBE;
                break;
        }

        cmft::OutputType::Enum cmftType;

        switch (type) {
            case ninniku::cmftImage::SaveType::Cubemap:
                cmftType = cmft::OutputType::Cubemap;
                break;
            case ninniku::cmftImage::SaveType::Facelist:
                cmftType = cmft::OutputType::FaceList;
                break;
            case ninniku::cmftImage::SaveType::LatLong:
                cmftType = cmft::OutputType::LatLong;
                break;
            case ninniku::cmftImage::SaveType::VCross:
                cmftType = cmft::OutputType::VCross;
                break;
        }

        return cmft::imageSave(_image, removeFileExtension(path).c_str(), cmftFileType, cmftType, cmftFormat, true);
    }

    void cmftImageImpl::UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch)
    {
        // get the right offset
        const uint32_t bytesPerPixel = GetBPPFromFormat(_image.m_format);
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

    bool cmftImageImpl::ValidateExtension(const std::string& ext) const
    {
        const std::array<std::string, 5> valid = { ".dds", ".exr", ".ktx", ".hdr", ".tga" };

        for (auto& validExt : valid)
            if (ext == validExt)
                return true;

        auto fmt = boost::format("cmftImage does not support extension: \"%1%\"") % ext;
        LOGE << boost::str(fmt);

        return false;
    }
} // namespace ninniku