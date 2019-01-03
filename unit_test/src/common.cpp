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

#include "common.h"

#include "shaders/cbuffers.h"

#include <ninniku/dx11/DX11.h>
#include <ninniku/types.h>

std::unique_ptr<ninniku::TextureObject>&& ResizeImageImpl(std::unique_ptr<ninniku::DX11>& dx, const std::unique_ptr<ninniku::TextureObject>& srcTex, const uint32_t width, const uint32_t height)
{
    auto subMarker = dx->CreateDebugMarker("CommonResizeImageImpl");

    ninniku::TextureParam dstParam = {};
    dstParam.width = width;
    dstParam.height = height;
    dstParam.format = srcTex->desc.format;
    dstParam.numMips = srcTex->desc.numMips;
    dstParam.arraySize = srcTex->desc.arraySize;
    dstParam.viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto dst = dx->CreateTexture(dstParam);

    for (uint32_t mip = 0; mip < dstParam.numMips; ++mip) {
        // dispatch
        ninniku::Command cmd = {};
        cmd.shader = "resize";
        cmd.ssBindings.insert(std::make_pair("ssLinear", dx->GetSampler(ninniku::ESamplerState::SS_Linear)));
        cmd.srvBindings.insert(std::make_pair("srcTex", srcTex->srvArray[mip]));
        cmd.uavBindings.insert(std::make_pair("dstTex", dst->uav[mip]));

        static_assert((RESIZE_NUMTHREAD_X == RESIZE_NUMTHREAD_Y) && (RESIZE_NUMTHREAD_Z == 1));
        cmd.dispatch[0] = width / RESIZE_NUMTHREAD_X;
        cmd.dispatch[1] = height / RESIZE_NUMTHREAD_Y;
        cmd.dispatch[2] = dstParam.arraySize / RESIZE_NUMTHREAD_Z;

        dx->Dispatch(cmd);
    }

    return std::move(dst);
}