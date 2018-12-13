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
#include "dds.h"

#include "../utils/log.h"
#include "../utils/misc.h"

namespace ninniku {
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

    std::vector<SubresourceParam> ddsImage::GetInitializationData() const
    {
        if (_meta.IsVolumemap()) {
            LOGE << "Texture3D are now supported for now";

            return std::vector<SubresourceParam>();
        }

        // texture1D or 2D
        std::vector<SubresourceParam> res(_meta.arraySize * _meta.mipLevels);

        size_t idx = 0;

        for (size_t item = 0; item < _meta.arraySize; ++item) {
            for (size_t level = 0; level < _meta.mipLevels; ++level) {
                size_t index = _meta.ComputeIndex(level, item, 0);
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
} // namespace ninniku