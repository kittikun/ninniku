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
#include <ninniku/image/cmft.h>

ninniku::TextureHandle ResizeImage(ninniku::DX11Handle& dx, const ninniku::TextureHandle& srcTex, const ninniku::SizeFixResult fixRes)
{
    auto subMarker = dx->CreateDebugMarker("CommonResizeImageImpl");

    auto dstParam = ninniku::TextureParam::Create();
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

        cmd.dispatch[0] = dstParam->width / 32;
        cmd.dispatch[1] = dstParam->height / 32;
        cmd.dispatch[2] = dstParam->arraySize / 1;

        dx->Dispatch(cmd);
    }

    return std::move(dst);
}

int main()
{
    ninniku::Initialize(ninniku::ERenderer::RENDERER_DX11, "E:\\ninniku\\unit_test\\shaders", ninniku::ELogLevel::LL_FULL);

    auto image = std::make_unique<ninniku::cmftImage>();

    image->Load("E:\\ninniku\\unit_test\\data\\park02.exr");

    auto needFix = image->IsRequiringFix();
    auto newSize = std::get<1>(needFix);

    auto srcParam = image->CreateTextureParam(ninniku::TV_SRV);
    auto& dx = ninniku::GetRenderer();
    auto marker = dx->CreateDebugMarker("Resize");
    auto srcTex = dx->CreateTexture(srcParam);

    auto dstParam = ninniku::TextureParam::Create();
    dstParam->width = newSize;
    dstParam->height = newSize;
    dstParam->format = srcTex->desc->format;
    dstParam->numMips = 1;
    dstParam->arraySize = 6;
    dstParam->viewflags = ninniku::TV_SRV | ninniku::TV_UAV;

    auto dst = ResizeImage(dx, srcTex, needFix);

    auto res = std::make_unique<ninniku::cmftImage>();

    res->InitializeFromTextureObject(dx, dst);

    image.reset();

    res->SaveImage("test", DXGI_FORMAT_R32G32B32A32_FLOAT, ninniku::cmftImage::SaveType::Cubemap);

    ninniku::Terminate();
}