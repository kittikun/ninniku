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
#include <ninniku/image/dds.h>
#include <ninniku/image/png.h>

int main()
{
    ninniku::Initialize(ninniku::ERenderer::RENDERER_DX11, "shaders", ninniku::ELogLevel::LL_FULL);

    auto image = std::make_unique<ninniku::pngImage>();

    image->Load("data/CC0-compressed-rock-NRM.png");

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto srcTex = dx->CreateTexture(srcParam);

    // normal to derivative
    auto dstParam = std::make_shared<ninniku::TextureParam>();
    dstParam->width = srcParam->width;
    dstParam->height = srcParam->height;
    dstParam->depth = srcParam->depth;
    dstParam->format = DXGI_FORMAT_R8G8_SINT;
    dstParam->numMips = srcParam->numMips;
    dstParam->arraySize = srcParam->arraySize;
    dstParam->viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto dst = dx->CreateTexture(dstParam);

    // dispatch
    ninniku::Command cmd = {};
    cmd.shader = "normal2Derivative";
    cmd.srvBindings.insert(std::make_pair("srcTex", srcTex->srvDefault));
    cmd.uavBindings.insert(std::make_pair("dstTex", dst->uav[0]));

    cmd.dispatch[0] = dstParam->width / 32;
    cmd.dispatch[1] = dstParam->height / 32;
    cmd.dispatch[2] = 1;

    dx->Dispatch(cmd);

    // derivative to normal
    auto dst2Param = std::make_shared<ninniku::TextureParam>();
    dst2Param->width = srcParam->width;
    dst2Param->height = srcParam->height;
    dst2Param->depth = srcParam->depth;
    dst2Param->format = DXGI_FORMAT_R11G11B10_FLOAT;
    dst2Param->numMips = srcParam->numMips;
    dst2Param->arraySize = srcParam->arraySize;
    dst2Param->viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto dst2 = dx->CreateTexture(dst2Param);

    // dispatch
    ninniku::Command cmd2 = {};
    cmd2.shader = "derivative2Normal";
    cmd2.srvBindings.insert(std::make_pair("srcTex", dst->srvDefault));
    cmd2.uavBindings.insert(std::make_pair("dstTex", dst2->uav[0]));

    cmd2.dispatch[0] = dst2Param->width / 32;
    cmd2.dispatch[1] = dst2Param->height / 32;
    cmd2.dispatch[2] = 1;

    dx->Dispatch(cmd2);

    // save as BC5
    auto resImg = std::make_unique<ninniku::ddsImage>();

    resImg->InitializeFromTextureObject(dx, dst);
    resImg->SaveImage("dds_saveImage_bc5.dds", dx, DXGI_FORMAT_BC5_SNORM);

    ninniku::Terminate();
}