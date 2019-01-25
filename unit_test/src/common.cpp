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
#include <ninniku/utils.h>

ninniku::TextureHandle GenerateColoredMips(ninniku::DX11Handle& dx)
{
    auto param = ninniku::CreateEmptyTextureParam();
    param->format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    param->width = param->height = 512;
    param->depth = 1;
    param->numMips = ninniku::CountMips(std::min(param->width, param->height));
    param->arraySize = ninniku::CUBEMAP_NUM_FACES;
    param->viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto marker = dx->CreateDebugMarker("ColorMips");
    auto resTex = dx->CreateTexture(param);

    for (uint32_t i = 0; i < param->numMips; ++i) {
        // dispatch
        ninniku::Command cmd = {};
        cmd.shader = "colorMips";
        cmd.cbufferStr = "CBGlobal";

        static_assert((COLORMIPS_NUMTHREAD_X == COLORMIPS_NUMTHREAD_Y) && (COLORMIPS_NUMTHREAD_Z == 1));
        cmd.dispatch[0] = std::max(1u, (param->width >> i) / COLORMIPS_NUMTHREAD_X);
        cmd.dispatch[1] = std::max(1u, (param->height >> i) / COLORMIPS_NUMTHREAD_Y);
        cmd.dispatch[2] = ninniku::CUBEMAP_NUM_FACES / COLORMIPS_NUMTHREAD_Z;

        cmd.uavBindings.insert(std::make_pair("dstTex", resTex->uav[i]));

        // constant buffer
        CBGlobal cb = {};

        cb.targetMip = i;
        dx->UpdateConstantBuffer(cmd.cbufferStr, &cb, sizeof(CBGlobal));

        dx->Dispatch(cmd);
    }

    return resTex;
}

ninniku::TextureHandle Generate2DTexWithMips(ninniku::DX11Handle& dx, const ninniku::Image* image)
{
    auto marker = dx->CreateDebugMarker("CommonGenerateMips");
    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto srcTex = dx->CreateTexture(srcParam);

    auto param = ninniku::CreateEmptyTextureParam();
    param->format = srcParam->format;
    param->width = srcParam->width;
    param->height = srcParam->height;
    param->depth = 1;
    param->numMips = ninniku::CountMips(std::min(param->width, param->height));
    param->arraySize = ninniku::CUBEMAP_NUM_FACES;
    param->viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto resTex = dx->CreateTexture(param);

    // copy srcTex mip 0 to resTex
    {
        auto subMarker = dx->CreateDebugMarker("Copy mip 0");

        ninniku::CopySubresourceParam copyParams = {};

        copyParams.src = srcTex.get();
        copyParams.dst = resTex.get();

        for (uint16_t i = 0; i < ninniku::CUBEMAP_NUM_FACES; ++i) {
            copyParams.srcFace = i;
            copyParams.dstFace = i;

            dx->CopySubresource(copyParams);
        }
    }

    // mip generation
    {
        auto subMarker = dx->CreateDebugMarker("Generate remaining mips");

        for (uint32_t srcMip = 0; srcMip < param->numMips - 1; ++srcMip) {
            // dispatch
            ninniku::Command cmd = {};
            cmd.shader = "downsample";
            cmd.cbufferStr = "CBGlobal";

            static_assert((DOWNSAMPLE_NUMTHREAD_X == DOWNSAMPLE_NUMTHREAD_Y) && (DOWNSAMPLE_NUMTHREAD_Z == 1));
            cmd.dispatch[0] = std::max(1u, (param->width >> srcMip) / DOWNSAMPLE_NUMTHREAD_X);
            cmd.dispatch[1] = std::max(1u, (param->height >> srcMip) / DOWNSAMPLE_NUMTHREAD_Y);
            cmd.dispatch[2] = ninniku::CUBEMAP_NUM_FACES / DOWNSAMPLE_NUMTHREAD_Z;

            cmd.srvBindings.insert(std::make_pair("srcMip", resTex->srvArray[srcMip]));
            cmd.uavBindings.insert(std::make_pair("dstMipSlice", resTex->uav[srcMip + 1]));
            cmd.ssBindings.insert(std::make_pair("ssPoint", dx->GetSampler(ninniku::ESamplerState::SS_Point)));

            dx->Dispatch(cmd);
        }
    }

    return std::move(resTex);
}

ninniku::TextureHandle ResizeImage(ninniku::DX11Handle& dx, const ninniku::TextureHandle& srcTex, const ninniku::SizeFixResult fixRes)
{
    auto subMarker = dx->CreateDebugMarker("CommonResizeImageImpl");

    auto dstParam = ninniku::CreateEmptyTextureParam();
    dstParam->width = std::get<1>(fixRes);
    dstParam->height = std::get<2>(fixRes);
    dstParam->depth = srcTex->desc->depth;
    dstParam->format = srcTex->desc->format;
    dstParam->numMips = srcTex->desc->numMips;
    dstParam->arraySize = srcTex->desc->arraySize;
    dstParam->viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto dst = dx->CreateTexture(dstParam);

    for (uint32_t mip = 0; mip < dstParam->numMips; ++mip) {
        // dispatch
        ninniku::Command cmd = {};
        cmd.shader = "resize";
        cmd.ssBindings.insert(std::make_pair("ssLinear", dx->GetSampler(ninniku::ESamplerState::SS_Linear)));

        if (dstParam->arraySize > 1)
            cmd.srvBindings.insert(std::make_pair("srcTex", srcTex->srvArray[mip]));
        else
            cmd.srvBindings.insert(std::make_pair("srcTex", srcTex->srvDefault));

        cmd.uavBindings.insert(std::make_pair("dstTex", dst->uav[mip]));

        static_assert((RESIZE_NUMTHREAD_X == RESIZE_NUMTHREAD_Y) && (RESIZE_NUMTHREAD_Z == 1));
        cmd.dispatch[0] = dstParam->width / RESIZE_NUMTHREAD_X;
        cmd.dispatch[1] = dstParam->height / RESIZE_NUMTHREAD_Y;
        cmd.dispatch[2] = dstParam->arraySize / RESIZE_NUMTHREAD_Z;

        dx->Dispatch(cmd);
    }

    return std::move(dst);
}