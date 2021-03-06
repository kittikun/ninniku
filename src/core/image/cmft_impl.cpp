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
#include "cmft_impl.h"

#include "ninniku/core/renderer/renderdevice.h"
#include "ninniku/core/renderer/types.h"
#include "ninniku/core/image/cmft.h"

#include "../renderer/dx11/dx11_types.h"
#include "../renderer/dx12/DX12.h"
#include "../../utils/log.h"
#include "../../utils/misc.h"

#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h>

#include <array>
#include <filesystem>

namespace ninniku
{
    cmftImage::cmftImage()
        : impl_{ new cmftImageImpl() }
    {
    }

    cmftImage::~cmftImage() = default;

    cmftImageImpl::~cmftImageImpl()
    {
        if (image_.m_data != nullptr)
            imageUnload(image_);
    }

    void cmftImageImpl::AllocateMemory()
    {
        // Alloc dst data.
        const uint32_t bytesPerPixel = GetBPPFromFormat(image_.m_format);
        uint32_t dstDataSize = 0;
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];

        for (uint8_t face = 0; face < image_.m_numFaces; ++face) {
            for (uint8_t mip = 0; mip < image_.m_numMips; ++mip) {
                dstOffsets[face][mip] = dstDataSize;
                uint32_t dstMipSize = std::max(UINT32_C(1), image_.m_width >> mip);
                dstDataSize += dstMipSize * dstMipSize * bytesPerPixel;
            }
        }

        if (image_.m_data != nullptr)
            imageUnload(image_);

        image_.m_data = CMFT_ALLOC(cmft::g_allocator, dstDataSize);
        image_.m_dataSize = dstDataSize;
    }

    TextureParamHandle cmftImageImpl::CreateTextureParamInternal(const EResourceViews viewFlags) const
    {
        auto res = TextureParam::Create();

        res->arraySize = CUBEMAP_NUM_FACES;
        res->depth = 1;
        res->format = TF_R32G32B32A32_FLOAT;
        res->height = res->width = imageGetCubemapFaceSize(image_);
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

    cmft::ImageFileType::Enum cmftImageImpl::GetFiletypeFromFilename(const std::filesystem::path& path)
    {
        auto res = cmft::ImageFileType::Count;

        if (!path.has_extension())
            LOGE << "Path passed to SaveImage must contain an extension";

        auto ext = path.extension();

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

    cmft::TextureFormat::Enum cmftImageImpl::GetFormatFromNinnikuFormat(uint32_t format) const
    {
        auto res = cmft::TextureFormat::Enum::Null;

        switch (format) {
            case TF_R16G16B16A16_FLOAT:
                res = cmft::TextureFormat::Enum::RGBA16F;
                break;

            case TF_R32G32B32A32_FLOAT:
                res = cmft::TextureFormat::Enum::RGBA32F;
                break;

            case TF_R8G8B8A8_UNORM:
                res = cmft::TextureFormat::Enum::BGRA8;
                break;

            default:
                LOGE << "Unsupported format was passed to GetFormatFromNinnikuFormat";
        }

        return res;
    }

    bool cmftImageImpl::LoadEXR(const std::filesystem::path& path)
    {
        int width, height;
        float* rgba;
        const char* err;

        int ret = ::LoadEXR(&rgba, &width, &height, path.string().c_str(), &err);
        if (ret != TINYEXR_SUCCESS) {
            auto fmt = boost::format("cmftImageImpl::LoadEXR failed with: %1%") % err;
            LOG << boost::str(fmt);

            return false;
        }

        image_.m_width = width;
        image_.m_height = height;
        image_.m_dataSize = width * height * sizeof(float);
        image_.m_format = cmft::TextureFormat::RGBA32F;
        image_.m_numMips = 1;
        image_.m_numFaces = 1;
        image_.m_data = rgba;

        return true;
    }

    bool cmftImageImpl::AssembleCubemap()
    {
        if (!imageIsCubemap(image_)) {
            if (imageIsCubeCross(image_)) {
                LOG << "Converting cube cross to cubemap.";
                imageCubemapFromCross(image_);
            } else if (imageIsLatLong(image_)) {
                LOG << "Converting latlong image to cubemap.";
                imageCubemapFromLatLong(image_);
            } else if (imageIsHStrip(image_)) {
                LOG << "Converting hstrip image to cubemap.";
                imageCubemapFromStrip(image_);
            } else if (imageIsVStrip(image_)) {
                LOG << "Converting vstrip image to cubemap.";
                imageCubemapFromStrip(image_);
            } else if (imageIsOctant(image_)) {
                LOG << "Converting octant image to cubemap.";
                imageCubemapFromOctant(image_);
            } else {
                LOGE << "Image is not cubemap(6 faces), cubecross(ratio 3:4 or 4:3), latlong(ratio 2:1), hstrip(ratio 6:1), vstrip(ration 1:6)";

                return false;
            }
        }

        if (!imageIsCubemap(image_))
            return false;

        return true;
    }

    bool cmftImageImpl::LoadInternal(const std::string_view& path)
    {
        auto fmt = boost::format("cmftImageImpl::Load, Path=\"%1%\"") % path;
        LOG << boost::str(fmt);

        bool imageLoaded = false;

        if (std::filesystem::path{ path }.extension() == ".exr")
            imageLoaded = LoadEXR(path);
        else
            imageLoaded = imageLoad(image_, path.data(), cmft::TextureFormat::RGBA32F) || imageLoadStb(image_, path.data(), cmft::TextureFormat::RGBA32F);

        if (!imageLoaded) {
            LOGE << "Failed to load file";

            return false;
        }

        if (!AssembleCubemap()) {
            LOGE << "Conversion failed.";

            return false;
        }

        return true;
    }

    bool cmftImageImpl::LoadRaw(const void* pData, const size_t size, const uint32_t width, const uint32_t height, const int32_t format)
    {
        auto cmftFormat = cmft::TextureFormat::RGBA32F;

        if (format == DXGI_FORMAT_R32G32B32A32_FLOAT) {
            cmftFormat = cmft::TextureFormat::Enum::RGBA32F;
        } else if (format == DXGI_FORMAT_R8G8B8A8_UNORM) {
            cmftFormat = cmft::TextureFormat::Enum::BGRA8;
        } else {
            LOGE << "SaveImage* format only supports DXGI_FORMAT_R32G32B32A32_FLOAT or DXGI_FORMAT_R8G8B8A8_UNORM";
            return false;
        }

        image_.m_width = width;
        image_.m_height = height;
        image_.m_dataSize = static_cast<uint32_t>(size);
        image_.m_format = cmftFormat;
        image_.m_numMips = 1;
        image_.m_numFaces = 1;
        //_image.m_data = (float*)pData;

        image_.m_data = CMFT_ALLOC(cmft::g_allocator, size);
        memcpy_s(image_.m_data, size, pData, size);

        if (!AssembleCubemap()) {
            LOGE << "Conversion failed.";

            return false;
        }

        return true;
    }

    bool cmftImageImpl::InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureHandle& srcTex)
    {
        return InitializeFromTextureObject(dx, srcTex, 0);
    }

    bool cmftImageImpl::InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureHandle& srcTex, const uint32_t cubeIndex)
    {
        // we want to enforce 1:1 for now
        if (srcTex->GetDesc()->width != srcTex->GetDesc()->height) {
            LOGE << "CFMT requires textures 1:1 for width and height";
            return false;
        }

        if (srcTex->GetDesc()->arraySize % CUBEMAP_NUM_FACES != 0) {
            LOGE << "InitializeFromTextureObject with cubeIndex specified implies source is a cubemap array";
            return false;
        }

        if ((cubeIndex != 0) && (srcTex->GetDesc()->arraySize / CUBEMAP_NUM_FACES == 1)) {
            LOGEF(boost::format("Source texture doesn't seems to have enough cubemaps in array to extract %1%") % cubeIndex);
            return false;
        }

        // allocate memory
        image_.m_width = srcTex->GetDesc()->width;
        image_.m_height = srcTex->GetDesc()->height;
        image_.m_format = GetFormatFromNinnikuFormat(srcTex->GetDesc()->format);
        image_.m_numFaces = CUBEMAP_NUM_FACES;
        image_.m_numMips = (uint8_t)srcTex->GetDesc()->numMips;

        auto fmt = boost::format("cmftImageImpl::InitializeFromTextureObject with Width=%1%, Height=%2%, Array=%3%, Mips=%4%") % image_.m_width % image_.m_height % (int)image_.m_numFaces % (int)image_.m_numMips;
        LOG << boost::str(fmt);

        AllocateMemory();

        auto marker = dx->CreateDebugMarker("ImageFromTextureObject");

        // we have to copy each mip with a read back texture or the same size for each face
        auto srcDesc = srcTex->GetDesc();

        if ((dx->GetType() & ERenderer::RENDERER_DX11) != 0) {
            // dx11 can read back from a texture
            for (uint32_t mip = 0; mip < srcDesc->numMips; ++mip) {
                auto param = TextureParam::Create();

                param->width = srcDesc->width >> mip;
                param->height = srcDesc->height >> mip;
                param->format = srcDesc->format;
                param->numMips = 1;
                param->arraySize = 1;
                param->viewflags = EResourceViews::RV_CPU_READ;

                auto readBack = dx->CreateTexture(param);

                CopyTextureSubresourceParam params = {};
                params.src = srcTex.get();
                params.srcMip = mip;
                params.dst = readBack.get();

                for (uint32_t face = 0; face < CUBEMAP_NUM_FACES; ++face) {
                    params.srcFace = (cubeIndex * CUBEMAP_NUM_FACES) + face;

                    auto indexes = dx->CopyTextureSubresource(params);
                    auto mapped = dx->Map(readBack, std::get<1>(indexes));
                    auto dx11Mapped = static_cast<const DX11MappedResource*>(mapped.get());

                    UpdateSubImage(face, mip, static_cast<uint8_t*>(mapped->GetData()), dx11Mapped->GetRowPitch());
                }
            }
        } else {
            // dx12 needs to use an intermediate buffer
            auto dx12 = static_cast<DX12*>(dx.get());

            for (uint32_t mip = 0; mip < srcDesc->numMips; ++mip) {
                auto param = TextureParam::Create();

                param->width = srcDesc->width >> mip;
                param->height = srcDesc->height >> mip;
                param->format = srcDesc->format;

                auto readback = dx12->CreateBuffer(param);

                CopyTextureSubresourceToBufferParam cpyParams = {};

                cpyParams.tex = srcTex.get();
                cpyParams.texMip = mip;
                cpyParams.buffer = readback.get();

                for (uint32_t face = 0; face < CUBEMAP_NUM_FACES; ++face) {
                    cpyParams.texFace = (cubeIndex * CUBEMAP_NUM_FACES) + face;

                    auto cpyRes = dx12->CopyTextureSubresourceToBuffer(cpyParams);

                    auto mapped = dx->Map(readback);

                    UpdateSubImage(face, mip, static_cast<uint8_t*>(mapped->GetData()), std::get<0>(cpyRes));
                }
            }
        }

        return true;
    }

    const std::vector<SubresourceParam> cmftImageImpl::GetInitializationData() const
    {
        std::array<uint32_t, CUBEMAP_NUM_FACES> offsets;

        cmft::imageGetFaceOffsets(offsets.data(), image_);

        const uint32_t bytesPerPixel = getImageDataInfo(image_.m_format).m_bytesPerPixel;
        std::vector<SubresourceParam> res(CUBEMAP_NUM_FACES);

        for (auto i = 0; i < CUBEMAP_NUM_FACES; ++i) {
            res[i].data = static_cast<void*>(static_cast<uint8_t*>(image_.m_data) + offsets[i]);
            res[i].rowPitch = image_.m_width * bytesPerPixel;
            res[i].depthPitch = 0;
        }

        return res;
    }

    const std::tuple<uint8_t*, uint32_t> cmftImageImpl::GetData() const
    {
        return { static_cast<uint8_t*>(image_.m_data), image_.m_dataSize };
    }

    bool cmftImageImpl::SaveImage(const std::filesystem::path& path, cmftImage::SaveType type)
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
            default:
                LOGE << "Unsupported cmft file type";
                return false;
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
            default:
                LOGE << "Unsupported cmft type";
                return false;
        }

        // because path is const
        auto pathCopy = path;

        return cmft::imageSave(image_, pathCopy.replace_extension().string().c_str(), cmftFileType, cmftType, cmftFormat, true);
    }

    void cmftImageImpl::UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch)
    {
        // get the right offset
        const uint32_t bytesPerPixel = GetBPPFromFormat(image_.m_format);
        uint32_t size = image_.m_width;
        uint32_t dstDataSize = 0;
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        uint32_t dstMipSize[CUBE_FACE_NUM][MAX_MIP_NUM];

        for (uint8_t face = 0; face < image_.m_numFaces; ++face) {
            for (uint8_t mip = 0; mip < image_.m_numMips; ++mip) {
                uint32_t mipSize = std::max(UINT32_C(1), size >> mip);

                dstMipSize[face][mip] = mipSize * mipSize * bytesPerPixel;
                dstOffsets[face][mip] = dstDataSize;
                dstDataSize += dstMipSize[face][mip];
            }
        }

        auto offset = (uint8_t*)image_.m_data + dstOffsets[dstFace][dstMip];

        // row pitch from dx11 can be larger than for the image so we have to do each row manually
        uint32_t mipSize = size >> dstMip;
        auto imgPitch = mipSize * bytesPerPixel;

        if (newRowPitch != imgPitch) {
            for (uint32_t row = 0; row < mipSize; ++row) {
                auto imgOffset = imgPitch * row;
                auto newOffset = newRowPitch * row;

                memcpy_s(offset + imgOffset, imgPitch, newData + newOffset, std::min(newRowPitch, imgPitch));
            }
        } else {
            memcpy_s(offset, imgPitch * mipSize, newData, imgPitch * mipSize);
        }
    }

    bool cmftImageImpl::ValidateExtension(const std::string_view& ext) const
    {
        const std::array<std::string_view, 5> valid = { ".dds", ".exr", ".ktx", ".hdr", ".tga" };

        for (auto& validExt : valid)
            if (ext == validExt)
                return true;

        auto fmt = boost::format("cmftImage does not support extension: \"%1%\"") % ext;
        LOGE << boost::str(fmt);

        return false;
    }
} // namespace ninniku