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
#include "ninniku/image/dds.h"

#include "ninniku/dx11/DX11.h"
#include "ninniku/dx11/DX11Types.h"

#include "../utils/log.h"
#include "../utils/misc.h"

#include <comdef.h>

namespace ninniku
{
    TextureParam ddsImage::CreateTextureParam(uint8_t viewFlags) const
    {
        TextureParam res = {};

        res.arraySize = static_cast<uint32_t>(_meta.arraySize);
        res.depth = static_cast<uint32_t>(_meta.depth);
        res.format = static_cast<uint32_t>(_meta.format);
        res.width = static_cast<uint32_t>(_meta.width);
        res.height = static_cast<uint32_t>(_meta.height);
        res.imageDatas = GetInitializationData();
        res.numMips = static_cast<uint32_t>(_meta.mipLevels);
        res.viewflags = viewFlags;

        return res;
    }

    std::tuple<uint8_t*, uint32_t> ddsImage::GetData() const
    {
        return std::make_tuple(_scratch.GetPixels(), static_cast<uint32_t>(_scratch.GetPixelsSize()));
    }

    std::vector<SubresourceParam> ddsImage::GetInitializationData() const
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

    bool ddsImage::Load(const std::string& path)
    {
        auto wPath = strToWStr(path);

        HRESULT hr = GetMetadataFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, _meta);
        if (FAILED(hr)) {
            auto fmt = boost::format("Could not load metadata for DDS file %1%") % path;
            LOGE << boost::str(fmt);
            return false;
        }

        if (_meta.miscFlags & DirectX::TEX_MISC_TEXTURECUBE) {
            LOGW << "Loading cubemap from DDS, if you want to process it, please use cmft";
        }

        if ((_meta.dimension == DirectX::TEX_DIMENSION_TEXTURE3D) && (_meta.arraySize > 1)) {
            LOGE << "Texture3DArray cannot be loaded";
            return false;
        }

        hr = LoadFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, &_meta, _scratch);
        if (FAILED(hr)) {
            auto fmt = boost::format("Failed to load DDS file %1%") % path;
            LOGE << boost::str(fmt);
            return false;
        }

        return true;
    }

    void ddsImage::InitializeFromTextureObject(std::unique_ptr<DX11>& dx, const std::unique_ptr<TextureObject>& srcTex)
    {
        // DirectXTex
        _meta.width = srcTex->desc.width;
        _meta.height = srcTex->desc.height;
        _meta.depth = srcTex->desc.depth;
        _meta.arraySize = srcTex->desc.arraySize;
        _meta.mipLevels = srcTex->desc.numMips;

        _meta.format = static_cast<DXGI_FORMAT>(srcTex->desc.format);

        // assume that an arraysize of 6 == cubemap
        if (_meta.depth > 1) {
            _meta.dimension = DirectX::TEX_DIMENSION_TEXTURE3D;
        } else if (_meta.height > 1) {
            _meta.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;
        } else {
            _meta.dimension = DirectX::TEX_DIMENSION_TEXTURE1D;
        }

        auto hr = _scratch.Initialize(_meta);

        if (FAILED(hr)) {
            auto fmt = boost::format("Failed to create DDS with Width=%1%, Height=%2%, Depth=%3%, Array=%4%, Mips=%5% with:") % _meta.width % _meta.height % _meta.depth % _meta.arraySize % _meta.mipLevels;
            LOGE << boost::str(fmt);
            _com_error err(hr);
            LOGE << err.ErrorMessage();

            return;
        }

        auto marker = dx->CreateDebugMarker("ddsFromTextureObject");

        // we have to copy each mip with a read back texture or the same size for each face
        for (uint32_t mip = 0; mip < srcTex->desc.numMips; ++mip) {
            ninniku::TextureParam param = {};

            param.width = srcTex->desc.width >> mip;
            param.height = srcTex->desc.height >> mip;
            param.format = srcTex->desc.format;
            param.numMips = 1;
            param.arraySize = 1;
            param.viewflags = ninniku::TV_CPU_READ;

            auto readBack = dx->CreateTexture(param);

            ninniku::CopySubresourceParam params = {};
            params.src = srcTex.get();
            params.srcMip = mip;
            params.dst = readBack.get();

            for (uint32_t face = 0; face < ninniku::CUBEMAP_NUM_FACES; ++face) {
                params.srcFace = face;

                auto indexes = dx->CopySubresource(params);
                auto mapped = dx->MapTexture(readBack, std::get<1>(indexes));

                UpdateSubImage(face, mip, (uint8_t*)mapped->GetData(), mapped->GetRowPitch());
            }
        }
    }

    bool ddsImage::SaveImage(const std::string& path, std::unique_ptr<DX11>& dx, DXGI_FORMAT format)
    {
        auto fmt = boost::format("Saving DDS with ddsImage file \"%1%\"") % path;
        LOG << boost::str(fmt);

        auto img = _scratch.GetImage(0, 0, 0);
        assert(img);
        size_t nimg = _scratch.GetImageCount();

        std::unique_ptr<DirectX::ScratchImage> resImage(new (std::nothrow) DirectX::ScratchImage);

        if (!resImage) {
            LOGE << "\nERROR: Memory allocation failed";
            return false;
        }

        bool bc6hbc7 = false;

        switch (format) {
            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                bc6hbc7 = true;
        }

        auto info = DirectX::TexMetadata(_meta);

        {
            LOGD_INDENT_START << "DirectXTex GPU Compression";

            auto subMarker = dx->CreateDebugMarker("DirectXTex Compress");
            auto hr = DirectX::Compress(dx->_device.Get(), img, nimg, info, format, 0, 1.f, *resImage);

            if (FAILED(hr)) {
                LOGE << "Failed to compress DDS";
                return false;
            }

            LOGE_INDENT_END;
        }

        auto hr = DirectX::SaveToDDSFile(*resImage->GetImage(0, 0, 0), 0, ninniku::strToWStr(path).c_str());

        if (FAILED(hr)) {
            LOGE << "Failed to save compressed DDS";
            return false;
        }

        return true;
    }

    void ddsImage::UpdateSubImage(uint32_t dstFace, uint32_t dstMip, uint8_t* newData, uint32_t newRowPitch)
    {
        auto index = _meta.ComputeIndex(dstMip, dstFace, 0);
        auto& img = _scratch.GetImages()[index];

        assert(img.rowPitch == newRowPitch);

        memcpy_s(img.pixels, img.height * img.rowPitch, newData, newRowPitch * img.height);
    }
} // namespace ninniku