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

#include <ninniku/ninniku.h>
#include <ninniku/dx11/DX11.h>
#include <ninniku/image/generic.h>
#include <ninniku/image/dds.h>

int main()
{
    std::vector<std::string> shaderPaths = { "..\\simple\\shaders", "..\\unit_test\\shaders" };

    ninniku::Initialize(ninniku::ERenderer::RENDERER_DX11, shaderPaths, ninniku::ELogLevel::LL_FULL);

    auto& dx = ninniku::GetRenderer();
    auto params = ninniku::TextureParam::Create();

    params->width = 512;
    params->height = 512;
    params->depth = 1;
    params->arraySize = 1;
    params->format = DXGI_FORMAT_R11G11B10_FLOAT;
    params->numMips = 1;
    params->viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto srcTex = dx->CreateTexture(params);

    // dispatch
    ninniku::Command cmd = {};
    cmd.shader = "gradient";

    cmd.dispatch[0] = params->width / 32;
    cmd.dispatch[1] = params->height / 32;
    cmd.dispatch[2] = 1;

    cmd.uavBindings.insert(std::make_pair("dstTex", srcTex->uav[0]));

    dx->Dispatch(cmd);

    auto outSRC = std::make_unique<ninniku::ddsImage>();
    outSRC->InitializeFromTextureObject(dx, srcTex);
    outSRC->SaveImage("gradient.dds");

    auto lmdb = [&](const std::string & shaderName)
    {
        auto subMarker = dx->CreateDebugMarker(shaderName);

        auto desc = params->Duplicate();
        desc->viewflags = ninniku::TV_SRV | ninniku::TV_UAV;
        desc->imageDatas.clear();
        desc->format = DXGI_FORMAT_R8G8B8A8_UNORM;

        auto dst = dx->CreateTexture(desc);

        // dispatch
        ninniku::Command cmd = {};
        cmd.shader = shaderName;

        cmd.dispatch[0] = desc->width / 32;
        cmd.dispatch[1] = desc->height / 32;
        cmd.dispatch[2] = 1;

        cmd.srvBindings.insert(std::make_pair("srcTex", srcTex->srvDefault));
        cmd.uavBindings.insert(std::make_pair("dstTex", dst->uav[0]));

        dx->Dispatch(cmd);

        auto out = std::make_unique<ninniku::ddsImage>();
        out->InitializeFromTextureObject(dx, dst);
        out->SaveImage(shaderName + ".dds");
    };

    lmdb("rgb565");
    lmdb("hsv565");
    lmdb("hsl565");
    lmdb("hcy565");
    lmdb("hcl565");
    lmdb("ycocg565");

    ninniku::Terminate();
}