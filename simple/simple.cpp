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
#include <ninniku/core/image/dds.h>

#include "shaders/cbuffers.h"

int main()
{
    // corresponding test: GenerateColoredMips
    std::vector<std::string_view> shaderPaths = { "..\\simple\\shaders" };

    ninniku::Initialize(ninniku::ERenderer::RENDERER_DX12, shaderPaths, ninniku::ELogLevel::LL_FULL);

    auto& dx = ninniku::GetRenderer();

    auto param = ninniku::TextureParam::Create();
    param->format = ninniku::TF_R32G32B32A32_FLOAT;
    param->width = param->height = 512;
    param->depth = 1;
    param->numMips = ninniku::CountMips(std::min(param->width, param->height));
    param->arraySize = ninniku::CUBEMAP_NUM_FACES;
    param->viewflags = static_cast<ninniku::EResourceViews>(ninniku::RV_SRV | ninniku::RV_UAV);

    auto marker = dx->CreateDebugMarker("ColorMips");
    auto resTex = dx->CreateTexture(param);

    for (uint32_t i = 0; i < param->numMips; ++i) {
        // dispatch
        auto cmd = dx->CreateCommand();
        cmd->shader = "colorMips";
        cmd->cbufferStr = "CBGlobal";

        static_assert((COLORMIPS_NUMTHREAD_X == COLORMIPS_NUMTHREAD_Y) && (COLORMIPS_NUMTHREAD_Z == 1));
        cmd->dispatch[0] = std::max(1u, (param->width >> i) / COLORMIPS_NUMTHREAD_X);
        cmd->dispatch[1] = std::max(1u, (param->height >> i) / COLORMIPS_NUMTHREAD_Y);
        cmd->dispatch[2] = ninniku::CUBEMAP_NUM_FACES / COLORMIPS_NUMTHREAD_Z;

        cmd->uavBindings.insert(std::make_pair("dstTex", resTex->GetUAV(i)));

        // constant buffer
        CBGlobal cb = {};

        cb.targetMip = i;
        dx->UpdateConstantBuffer(cmd->cbufferStr, &cb, sizeof(CBGlobal));

        dx->Dispatch(cmd);

        auto res = std::make_unique<ninniku::ddsImage>();

        res->InitializeFromTextureObject(dx, resTex);
    }

    ninniku::Terminate();
}