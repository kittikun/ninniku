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

#include <ninniku/ninniku.h>
#include <ninniku/core/renderer/renderdevice.h>
#include <ninniku/core/image/cmft.h>
#include <ninniku/core/image/dds.h>

#include "shaders/cbuffers.h"

int main()
{
    // corresponding test: GenerateColoredMips
    std::vector<std::string_view> shaderPaths = { "..\\simple\\shaders" };

    ninniku::Initialize(ninniku::ERenderer::RENDERER_DX12, shaderPaths, ninniku::ELogLevel::LL_FULL);

    auto& dx = ninniku::GetRenderer();

    auto image = std::make_unique<ninniku::ddsImage>();
    image->Load("Cathedral01.dds");

    auto marker = dx->CreateDebugMarker("CommonGenerateMips");
    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
    auto srcTex = dx->CreateTexture(srcParam);

    auto param = ninniku::TextureParam::Create();
    param->format = srcParam->format;
    param->width = srcParam->width;
    param->height = srcParam->height;
    param->depth = 1;
    param->numMips = ninniku::CountMips(std::min(param->width, param->height));
    param->arraySize = ninniku::CUBEMAP_NUM_FACES;
    param->viewflags = static_cast<ninniku::EResourceViews>(ninniku::RV_SRV | ninniku::RV_UAV);

    auto resTex = dx->CreateTexture(param);

    // copy srcTex mip 0 to resTex
    {
        auto subMarker = dx->CreateDebugMarker("Copy mip 0");

        ninniku::CopyTextureSubresourceParam copyParams = {};

        copyParams.src = srcTex.get();
        copyParams.dst = resTex.get();

        for (uint16_t i = 0; i < ninniku::CUBEMAP_NUM_FACES; ++i) {
            copyParams.srcFace = i;
            copyParams.dstFace = i;

            dx->CopyTextureSubresource(copyParams);
        }
    }

    // mip generation
    {
        auto subMarker = dx->CreateDebugMarker("Generate remaining mips");

        for (uint32_t srcMip = 0; srcMip < param->numMips - 1; ++srcMip) {
            // dispatch
            auto cmd = dx->CreateCommand();
            cmd->shader = "downsample";
            cmd->cbufferStr = "CBGlobal";

            static_assert((DOWNSAMPLE_NUMTHREAD_X == DOWNSAMPLE_NUMTHREAD_Y) && (DOWNSAMPLE_NUMTHREAD_Z == 1));
            cmd->dispatch[0] = std::max(1u, (param->width >> srcMip) / DOWNSAMPLE_NUMTHREAD_X);
            cmd->dispatch[1] = std::max(1u, (param->height >> srcMip) / DOWNSAMPLE_NUMTHREAD_Y);
            cmd->dispatch[2] = ninniku::CUBEMAP_NUM_FACES / DOWNSAMPLE_NUMTHREAD_Z;

            cmd->srvBindings.insert(std::make_pair("srcMip", resTex->GetSRVArray(srcMip)));
            cmd->uavBindings.insert(std::make_pair("dstMipSlice", resTex->GetUAV(srcMip + 1)));
            cmd->ssBindings.insert(std::make_pair("ssPoint", dx->GetSampler(ninniku::ESamplerState::SS_Point)));

            dx->Dispatch(cmd);
        }
    }

    ninniku::Terminate();
}