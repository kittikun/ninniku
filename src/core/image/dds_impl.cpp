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
#include "dds_impl.h"

#include "ninniku/core/image/dds.h"

#include "../../globals.h"
#include "../../utils/log.h"
#include "../../utils/misc.h"
#include "../renderer/dx11/DX11.h"
#include "../renderer/dx12/DX12.h"

#include <d3dx12/d3dx12.h>
#include <comdef.h>

namespace ninniku
{
    ddsImage::ddsImage()
        : impl_{ new ddsImageImpl() }
    {
    }

    ddsImage::~ddsImage() = default;

    TextureParamHandle ddsImageImpl::CreateTextureParamInternal(const EResourceViews viewFlags) const
    {
        auto res = TextureParam::Create();

        res->arraySize = static_cast<uint32_t>(meta_.arraySize);
        res->depth = static_cast<uint32_t>(meta_.depth);
        res->format = DXGIFormatToNinnikuTF(meta_.format);
        res->width = static_cast<uint32_t>(meta_.width);
        res->height = static_cast<uint32_t>(meta_.height);
        res->imageDatas = GetInitializationData();
        res->numMips = static_cast<uint32_t>(meta_.mipLevels);
        res->viewflags = viewFlags;

        return std::move(res);
    }

    const std::tuple<uint8_t*, uint32_t> ddsImageImpl::GetData() const
    {
        return std::make_tuple(scratch_.GetPixels(), static_cast<uint32_t>(scratch_.GetPixelsSize()));
    }

    const std::vector<SubresourceParam> ddsImageImpl::GetInitializationData() const
    {
        if (meta_.IsVolumemap()) {
            LOGE << "Texture3D are not supported for now";

            return std::vector<SubresourceParam>();
        }

        // texture1D or 2D
        std::vector<SubresourceParam> res(meta_.arraySize * meta_.mipLevels);

        size_t idx = 0;

        for (size_t item = 0; item < meta_.arraySize; ++item) {
            for (size_t level = 0; level < meta_.mipLevels; ++level) {
                auto index = meta_.ComputeIndex(level, item, 0);
                auto& img = scratch_.GetImages()[index];

                res[idx].data = img.pixels;
                res[idx].rowPitch = static_cast<uint32_t>(img.rowPitch);
                res[idx].depthPitch = static_cast<uint32_t>(img.slicePitch);
                ++idx;
            }
        }

        return res;
    }

    bool ddsImageImpl::LoadInternal(const std::string_view& path)
    {
        auto fmt = boost::format("ddsImageImpl::Load, Path=\"%1%\"") % path;
        LOG << boost::str(fmt);

        auto wPath = strToWStr(path);

        HRESULT hr = GetMetadataFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, meta_);
        if (FAILED(hr)) {
            fmt = boost::format("Could not load metadata for DDS file %1%") % path;
            LOGE << boost::str(fmt);
            return false;
        }

        if ((meta_.dimension == DirectX::TEX_DIMENSION_TEXTURE3D) && (meta_.arraySize > 1)) {
            LOGE << "Texture3DArray cannot be loaded";
            return false;
        }

        hr = LoadFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, &meta_, scratch_);
        if (FAILED(hr)) {
            fmt = boost::format("Failed to load DDS file %1%") % path;
            LOGE << boost::str(fmt);
            return false;
        }

        return true;
    }

    bool ddsImageImpl::LoadRaw(const void* pData, const size_t size)
    {
        const auto hr = LoadFromDDSMemory(pData, size, DirectX::DDS_FLAGS_NONE, &meta_, scratch_);
        if (FAILED(hr)) {
            LOGE << "Failed to load DDS file";
            return false;
        }

        return true;
    }

    bool ddsImageImpl::LoadRaw([[maybe_unused]] const void* pData, [[maybe_unused]] const size_t size, [[maybe_unused]] const uint32_t width, [[maybe_unused]] const uint32_t height, [[maybe_unused]] const int32_t format)
    {
        throw std::exception("not implemented");
    }

    bool ddsImageImpl::InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureHandle& srcTex)
    {
        // DirectXTex
        meta_ = DirectX::TexMetadata{};
        meta_.width = srcTex->GetDesc()->width;
        meta_.height = srcTex->GetDesc()->height;
        meta_.depth = srcTex->GetDesc()->depth;
        meta_.arraySize = srcTex->GetDesc()->arraySize;
        meta_.mipLevels = srcTex->GetDesc()->numMips;
        meta_.format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(srcTex->GetDesc()->format));

        if (meta_.depth > 1) {
            meta_.dimension = DirectX::TEX_DIMENSION_TEXTURE3D;
        } else if (meta_.height > 1) {
            meta_.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;
        } else {
            meta_.dimension = DirectX::TEX_DIMENSION_TEXTURE1D;
        }

        // assume that an arraysize of 6 == cubemap
        if ((meta_.arraySize % CUBEMAP_NUM_FACES == 0) && (meta_.dimension == DirectX::TEX_DIMENSION_TEXTURE2D))
            meta_.miscFlags |= DirectX::TEX_MISC_TEXTURECUBE;

        auto fmt = boost::format("ddsImageImpl::InitializeFromTextureObject with Width=%1%, Height=%2%, Depth=%3%, Array=%4%, Mips=%5%, IsCubemap=%6%") % meta_.width % meta_.height % meta_.depth % meta_.arraySize % meta_.mipLevels % ((meta_.miscFlags & DirectX::TEX_MISC_TEXTURECUBE) != 0);
        LOG << boost::str(fmt);

        auto hr = scratch_.Initialize(meta_);

        if (CheckAPIFailed(hr, "DirectX::ScratchImage::Initialize"))
            return false;

        auto marker = dx->CreateDebugMarker("ddsFromTextureObject");

        // we have to copy each mip with a read back texture of the same size for each face
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
                param->depth = 1;

                auto readBack = dx->CreateTexture(param);

                ninniku::CopyTextureSubresourceParam params = {};
                params.src = srcTex.get();
                params.srcMip = mip;
                params.dst = readBack.get();

                for (uint32_t face = 0; face < srcDesc->arraySize; ++face) {
                    params.srcFace = face;

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

                for (uint32_t face = 0; face < srcDesc->arraySize; ++face) {
                    cpyParams.texFace = face;

                    auto cpyRes = dx12->CopyTextureSubresourceToBuffer(cpyParams);

                    auto mapped = dx->Map(readback);

                    UpdateSubImage(face, mip, static_cast<uint8_t*>(mapped->GetData()), std::get<0>(cpyRes));
                }
            }
        }

        return true;
    }

    bool ddsImageImpl::SaveImage(const std::string_view& path)
    {
        auto hr = DirectX::SaveToDDSFile(scratch_.GetImage(0, 0, 0), scratch_.GetImageCount(), meta_, DirectX::DDS_FLAGS_FORCE_DX10_EXT, ninniku::strToWStr(path).c_str());

        if (FAILED(hr)) {
            LOGE << "Failed to save compressed DDS";
            return false;
        }

        return true;
    }

    bool ddsImageImpl::SaveCompressedImage(const std::string_view& path, RenderDeviceHandle& dx, DXGI_FORMAT format)
    {
        auto fmt = boost::format("Saving DDS with ddsImageImpl file \"%1%\"") % path;
        LOG << boost::str(fmt);

        if (!DirectX::IsCompressed(format)) {
            LOGE << "Only compressed format are supported for now";
            return false;
        }

        auto img = scratch_.GetImage(0, 0, 0);
        assert(img);
        size_t nimg = scratch_.GetImageCount();

        std::unique_ptr<DirectX::ScratchImage> resImageImpl(new (std::nothrow) DirectX::ScratchImage);

        if (!resImageImpl) {
            LOGE << "\nERROR: Memory allocation failed";
            return false;
        }

        DWORD flags = DirectX::TEX_COMPRESS_PARALLEL;
        auto bc6hbc7 = false;

        // use best compression for BC7
        switch (format) {
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                if (Globals::Instance().bc7Quick_)
                    flags |= DirectX::TEX_COMPRESS_BC7_QUICK;
                else
                    flags |= DirectX::TEX_COMPRESS_BC7_USE_3SUBSETS;
                bc6hbc7 = true;
                break;

            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
                bc6hbc7 = true;
                break;
        };

        if (bc6hbc7) {
            HRESULT hr;

            // DirectXTex only support DX11
            if (dx->GetType() & ERenderer::RENDERER_DX11) {
                LOGD_INDENT_START << "DirectXTex GPU Compression";
                auto subMarker = dx->CreateDebugMarker("DirectXTex Compress");
                auto dx11 = static_cast<DX11*>(dx.get());

                hr = DirectX::Compress(dx11->GetDevice(), img, nimg, meta_, format, static_cast<DirectX::TEX_COMPRESS_FLAGS>(flags), 1.f, *resImageImpl);
            } else {
                LOGD_INDENT_START << "DirectXTex CPU Compression";

                hr = DirectX::Compress(img, nimg, meta_, format, static_cast<DirectX::TEX_COMPRESS_FLAGS>(flags), 1.f, *resImageImpl);
            }

            if (FAILED(hr)) {
                LOGE << "Failed to compress DDS";
                LOGD_INDENT_END;
                return false;
            }

            LOGD_INDENT_END;
        } else {
            LOGD_INDENT_START << "DirectXTex CPU Compression";

            auto hr = DirectX::Compress(img, nimg, meta_, format, static_cast<DirectX::TEX_COMPRESS_FLAGS>(flags), DirectX::TEX_THRESHOLD_DEFAULT, *resImageImpl);

            if (FAILED(hr)) {
                LOGE << "Failed to compress DDS";
                LOGD_INDENT_END;
                return false;
            }

            LOGD_INDENT_END;
        }

        auto resMeta = DirectX::TexMetadata(meta_);

        resMeta.format = format;

        auto hr = DirectX::SaveToDDSFile(resImageImpl->GetImage(0, 0, 0), nimg, resMeta, DirectX::DDS_FLAGS_FORCE_DX10_EXT, ninniku::strToWStr(path).c_str());

        if (FAILED(hr)) {
            LOGE << "Failed to save compressed DDS";
            return false;
        }

        return true;
    }

    void ddsImageImpl::UpdateSubImage(const uint32_t dstFace, const uint32_t dstMip, const uint8_t* newData, const uint32_t newRowPitch)
    {
        auto index = meta_.ComputeIndex(dstMip, dstFace, 0);
        auto& img = scratch_.GetImages()[index];

        if (newRowPitch > img.rowPitch) {
            // row pitch from dx11 can be larger than for the image so we have to do each row manually
            std::vector<uint8_t> temp(img.height * img.rowPitch);

            for (size_t y = 0; y < img.height; ++y) {
                memcpy_s(&temp[y * img.rowPitch], img.rowPitch, &newData[y * newRowPitch], img.rowPitch);
            }

            memcpy_s(img.pixels, img.height * img.rowPitch, temp.data(), temp.size());
        } else {
            memcpy_s(img.pixels, img.height * img.rowPitch, newData, img.height * newRowPitch);
        }
    }

    bool ddsImageImpl::ValidateExtension(const std::string_view& ext) const
    {
        if (ext == ".dds")
            return true;

        auto fmt = boost::format("ddsImage does not support extension: \"%1%\"") % ext;
        LOGE << boost::str(fmt);

        return false;
    }
} // namespace ninniku