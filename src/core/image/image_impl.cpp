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
#include "image_impl.h"

#include "../../utils/mathUtils.h"
#include "../../utils/log.h"

namespace ninniku
{
    TextureParamHandle ImageImpl::CreateTextureParam(const uint8_t viewFlags) const
    {
        if (viewFlags == EResourceViews::RV_None) {
            LOGE << "TextureParam view flags cannot be ETextureViews::TV_None";
            return TextureParam::Create();
        }

        return CreateTextureParamInternal(viewFlags);
    }

    const SizeFixResult ImageImpl::IsRequiringFix() const
    {
        auto tx = GetWidth();
        auto ty = GetHeight();
        auto res = false;

        if (!IsPow2(tx)) {
            auto fmt = boost::format("Width %1% is not a power of 2") % tx;
            LOGW << boost::str(fmt);

            tx = NearestPow2Floor(tx);
            res = true;
        }

        if (!IsPow2(ty)) {
            auto fmt = boost::format("Height %1% is not a power of 2") % ty;
            LOGW << boost::str(fmt);

            ty = NearestPow2Floor(ty);
            res = true;
        }

        return std::make_tuple(res, tx, ty);
    }

    bool ImageImpl::Load(const std::string& path)
    {
        auto validPath = boost::filesystem::path{ path };

        if (!boost::filesystem::exists(validPath)) {
            auto fmt = boost::format("Could not find file \"%1%\"") % path;
            LOGE << boost::str(fmt);

            return false;
        }

        if (!ValidateExtension(validPath.extension().string()))
            return false;

        return LoadInternal(path);
    }

} // namespace ninniku