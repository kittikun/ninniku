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

#include "../../utils/log.h"
#include "../../utils/misc.h"
#include "../renderer/dx11/DX11.h"

#include <comdef.h>

namespace ninniku {
    ddsImage::ddsImage()
        : _impl{ new ddsImageImpl() }
    {
    }

    ddsImage::~ddsImage() = default;

    TextureParamHandle ddsImageImpl::CreateTextureParamInternal(const EResourceViews viewFlags) const
    {
        auto res = std::make_shared<TextureParam>();

        res->arraySize = static_cast<uint32_t>(_meta.arraySize);
        res->depth = static_cast<uint32_t>(_meta.depth);
        res->format = DXGIFormatToNinnikuTF(_meta.format);
        res->width = static_cast<uint32_t>(_meta.width);
        res->height = static_cast<uint32_t>(_meta.height);
        res->imageDatas = GetInitializationData();
        res->numMips = static_cast<uint32_t>(_meta.mipLevels);
        res->viewflags = viewFlags;

        return std::move(res);
    }

    const std::tuple<uint8_t*, uint32_t> ddsImageImpl::GetData() const
    {
        return std::make_tuple(_scratch.GetPixels(), static_cast<uint32_t>(_scratch.GetPixelsSize()));
    }

    const std::vector<SubresourceParam> ddsImageImpl::GetInitializationData() const
    {
        if (_meta.IsVolumemap()) {
            LOGE << "Texture3D are not supported for now";

            return std::vector<SubresourceParam>();
        }

        // texture1D or 2D
        std::vector<SubresourceParam> res(_meta.arraySize * _meta.mipLevels);

        size_t idx = 0;

        for (size_t item = 0; item < _meta.arraySize; ++item) {
            for (size_t level = 0; level < _meta.mipLevels; ++level) {
                auto index = _meta.ComputeIndex(level, item, 0);
                auto& img = _scratch.GetImages()[index];

                res[idx].data = img.pixels;
                res[idx].rowPitch = static_cast<uint32_t>(img.rowPitch);
                res[idx].depthPitch = static_cast<uint32_t>(img.slicePitch);
                ++idx;
            }
        }

        return res;
    }

    bool ddsImageImpl::LoadInternal(const std::string& path)
    {
        auto fmt = boost::format("ddsImageImpl::Load, Path=\"%1%\"") % path;
        LOG << boost::str(fmt);

        auto wPath = strToWStr(path);

        HRESULT hr = GetMetadataFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, _meta);
        if (FAILED(hr)) {
            fmt = boost::format("Could not load metadata for DDS file %1%") % path;
            LOGE << boost::str(fmt);
            return false;
        }

        if ((_meta.dimension == DirectX::TEX_DIMENSION_TEXTURE3D) && (_meta.arraySize > 1)) {
            LOGE << "Texture3DArray cannot be loaded";
            return false;
        }

        hr = LoadFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, &_meta, _scratch);
        if (FAILED(hr)) {
            fmt = boost::format("Failed to load DDS file %1%") % path;
            LOGE << boost::str(fmt);
            return false;
        }

        return true;
    }

    bool ddsImageImpl::LoadRaw(const void* pData, const size_t size)
    {
        const auto hr = LoadFromDDSMemory(pData, size, DirectX::DDS_FLAGS_NONE, &_meta, _scratch);
        if (FAILED(hr)) {
            LOGE << "Failed to load DDS file";
            return false;
        }

        return true;
    }

    bool ddsImageImpl::LoadRaw([[maybe_unused]]const void* pData, [[maybe_unused]]const size_t size, [[maybe_unused]]const uint32_t width, [[maybe_unused]]const uint32_t height, [[maybe_unused]]const int32_t format)
    {
        throw std::exception("not implemented");
    }

    void ddsImageImpl::InitializeFromTextureObject(RenderDeviceHandle& dx, const TextureHandle& srcTex)
    {
        // DirectXTex
        _meta = DirectX::TexMetadata{};
        _meta.width = srcTex->desc->width;
        _meta.height = srcTex->desc->height;
        _meta.depth = srcTex->desc->depth;
        _meta.arraySize = srcTex->desc->arraySize;
        _meta.mipLevels = srcTex->desc->numMips;
        _meta.format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(srcTex->desc->format));

        if (_meta.depth > 1) {
            _meta.dimension = DirectX::TEX_DIMENSION_TEXTURE3D;
        } else if (_meta.height > 1) {
            _meta.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;
        } else {
            _meta.dimension = DirectX::TEX_DIMENSION_TEXTURE1D;
        }

        // assume that an arraysize of 6 == cubemap
        if ((_meta.arraySize % CUBEMAP_NUM_FACES == 0) && (_meta.dimension == DirectX::TEX_DIMENSION_TEXTURE2D))
            _meta.miscFlags |= DirectX::TEX_MISC_TEXTURECUBE;

        auto fmt = boost::format("ddsImageImpl::InitializeFromTextureObject with Width=%1%, Height=%2%, Depth=%3%, Array=%4%, Mips=%5%, IsCubemap=%6%") % _meta.width % _meta.height % _meta.depth % _meta.arraySize % _meta.mipLevels % ((_meta.miscFlags & DirectX::TEX_MISC_TEXTURECUBE) != 0);
        LOG << boost::str(fmt);

        auto hr = _scratch.Initialize(_meta);

        if (FAILED(hr)) {
            fmt = boost::format("Failed to create DDS with:");
            LOGE << boost::str(fmt);
            _com_error err(hr);
            LOGE << err.ErrorMessage();

            return;
        }

        auto marker = dx->CreateDebugMarker("ddsFromTextureObject");

        // we have to copy each mip with a read back texture or the same size for each face
        for (uint32_t mip = 0; mip < _meta.mipLevels; ++mip) {
            auto param = std::make_shared<ninniku::TextureParam>();

            param->width = srcTex->desc->width >> mip;
            param->height = srcTex->desc->height >> mip;
            param->format = srcTex->desc->format;
            param->numMips = 1;
            param->arraySize = 1;
            param->viewflags = ninniku::RV_CPU_READ;
            param->depth = 1;

            auto readBack = dx->CreateTexture(param);

            ninniku::CopyTextureSubresourceParam params = {};
            params.src = srcTex.get();
            params.srcMip = mip;
            params.dst = readBack.get();

            for (uint32_t face = 0; face < _meta.arraySize; ++face) {
                params.srcFace = face;

                auto indexes = dx->CopyTextureSubresource(params);
                auto mapped = dx->MapTexture(readBack, std::get<1>(indexes));

                UpdateSubImage(face, mip, (uint8_t*)mapped->GetData(), mapped->GetRowPitch());
            }
        }
    }

    bool ddsImageImpl::SaveImage(const std::string& path)
    {
        auto hr = DirectX::SaveToDDSFile(_scratch.GetImage(0, 0, 0), _scratch.GetImageCount(), _meta, DirectX::DDS_FLAGS_FORCE_DX10_EXT, ninniku::strToWStr(path).c_str());

        if (FAILED(hr)) {
            LOGE << "Failed to save compressed DDS";
            return false;
        }

        return true;
    }

    bool ddsImageImpl::SaveCompressedImage(const std::string& path, RenderDeviceHandle& dx, DXGI_FORMAT format)
    {
        auto fmt = boost::format("Saving DDS with ddsImageImpl file \"%1%\"") % path;
        LOG << boost::str(fmt);

        if (!DirectX::IsCompressed(format)) {
            LOGE << "Only compressed format are supported for now";
            return false;
        }

        auto img = _scratch.GetImage(0, 0, 0);
        assert(img);
        size_t nimg = _scratch.GetImageCount();

        std::unique_ptr<DirectX::ScratchImage> resImageImpl(new (std::nothrow) DirectX::ScratchImage);

        if (!resImageImpl) {
            LOGE << "\nERROR: Memory allocation failed";
            return false;
        }

        DWORD flags = DirectX::TEX_COMPRESS_DEFAULT;
        auto bc6hbc7 = false;

        // use best compression for BC7
        switch (format) {
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
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

            if ((dx->GetType() & ERenderer::RENDERER_WARP) == 0) {
                LOGD_INDENT_START << "DirectXTex GPU Compression";
                auto subMarker = dx->CreateDebugMarker("DirectXTex Compress");
                auto dx11 = static_cast<DX11*>(dx.get());

                hr = DirectX::Compress(dx11->_device.Get(), img, nimg, _meta, format, flags, 1.f, *resImageImpl);
            } else {
                LOGD_INDENT_START << "DirectXTex CPU Compression";

                hr = DirectX::Compress(img, nimg, _meta, format, flags, 1.f, *resImageImpl);
            }

            if (FAILED(hr)) {
                LOGE << "Failed to compress DDS";
                return false;
            }

            LOGD_INDENT_END;
        } else {
            LOGD_INDENT_START << "DirectXTex CPU Compression";

            auto hr = DirectX::Compress(img, nimg, _meta, format, flags, DirectX::TEX_THRESHOLD_DEFAULT, *resImageImpl);

            if (FAILED(hr)) {
                LOGE << "Failed to compress DDS";
                return false;
            }

            LOGD_INDENT_END;
        }

        auto resMeta = DirectX::TexMetadata(_meta);

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
        auto index = _meta.ComputeIndex(dstMip, dstFace, 0);
        auto& img = _scratch.GetImages()[index];

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

    bool ddsImageImpl::ValidateExtension(const std::string& ext) const
    {
        if (ext == ".dds")
            return true;

        auto fmt = boost::format("ddsImage does not support extension: \"%1%\"") % ext;
        LOGE << boost::str(fmt);

        return false;
    }
} // namespace ninniku