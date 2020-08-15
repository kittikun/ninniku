//// Copyright(c) 2018-2020 Kitti Vongsay
////
//// Permission is hereby granted, free of charge, to any person obtaining a copy
//// of this software and associated documentation files(the "Software"), to deal
//// in the Software without restriction, including without limitation the rights
//// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//// copies of the Software, and to permit persons to whom the Software is
//// furnished to do so, subject to the following conditions :
////
//// The above copyright notice and this permission notice shall be included in all
//// copies or substantial portions of the Software.
////
//// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//// SOFTWARE.
//
//#include "../shaders/cbuffers.h"
//#include "../shaders/dispatch.h"
//#include "../check.h"
//#include "../common.h"
//#include "../fixture.h"
//#include "../utils.h"
//
//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wunused-variable"
//#pragma clang diagnostic ignored "-Wdeprecated-declarations"
//#include <boost/test/unit_test.hpp>
//#pragma clang diagnostic pop
//
//#include <ninniku/core/renderer/renderdevice.h>
//#include <ninniku/core/renderer/rendergraph.h>
//#include <ninniku/core/renderer/types.h>
//#include <ninniku/core/image/cmft.h>
//#include <ninniku/core/image/dds.h>
//#include <ninniku/ninniku.h>
//#include <ninniku/types.h>
//#include <ninniku/utils.h>
//
//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wdeprecated-declarations"
//#include <boost/format.hpp>
//#pragma clang diagnostic pop
//
//BOOST_AUTO_TEST_SUITE(Shader)
//
//BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_SRV_UAV_same_resource, T, FixturesAll, T)
//{
//    // Disable HW GPU support when running on CI
//    if (T::isNull)
//        return;
//
//    auto& dx = ninniku::GetRenderer();
//
//    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
//    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
//        return;
//    }
//
//    BOOST_REQUIRE(LoadShader(dx, "sameResource", T::shaderRoot));
//
//    auto param = ninniku::TextureParam::Create();
//
//    param->width = param->height = 512;
//
//    auto numMips = ninniku::CountMips(param->width);
//
//    param->numMips = numMips;
//    param->arraySize = ninniku::CUBEMAP_NUM_FACES;
//    param->depth = 1;
//    param->format = ninniku::TF_R8G8B8A8_UNORM;
//    param->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;
//
//    auto res = dx->CreateTexture(param);
//    auto cmd = dx->CreateCommand();
//    CBGlobal cb = {};
//
//    cmd->shader = "sameResource";
//    cmd->cbufferStr = "CBGlobal";
//
//    // first dispatch is to initialize mip 0 to black
//    {
//        auto marker = dx->CreateDebugMarker("Initialize mip 0");
//
//        cmd->srvBindings.insert(std::make_pair("srcTex", nullptr));
//        cmd->uavBindings.insert(std::make_pair("dstTex", res->GetUAV(0)));
//        cmd->dispatch[0] = param->width / SAME_RESOURCE_X;
//        cmd->dispatch[1] = param->height / SAME_RESOURCE_Y;
//        cmd->dispatch[2] = param->arraySize / SAME_RESOURCE_Z;
//
//        // constant buffer
//        cb.targetMip = 0;
//
//        BOOST_REQUIRE(dx->UpdateConstantBuffer(cmd->cbufferStr, &cb, sizeof(CBGlobal)));
//        BOOST_REQUIRE(dx->Dispatch(cmd));
//    }
//
//    auto marker = dx->CreateDebugMarker("Other mips");
//
//    for (uint32_t i = 1; i < numMips; ++i) {
//        cmd->srvBindings["srcTex"] = res->GetSRVArray(i - 1);
//        cmd->uavBindings["dstTex"] = res->GetUAV(i);
//
//        cmd->dispatch[0] = std::max(1u, (param->width >> i) / SAME_RESOURCE_X);
//        cmd->dispatch[1] = std::max(1u, (param->height >> i) / SAME_RESOURCE_Y);
//
//        cb.targetMip = i;
//
//        BOOST_REQUIRE(dx->UpdateConstantBuffer(cmd->cbufferStr, &cb, sizeof(CBGlobal)));
//        BOOST_REQUIRE(dx->Dispatch(cmd));
//    }
//
//    auto image = std::make_unique<ninniku::ddsImage>();
//
//    BOOST_REQUIRE(image->InitializeFromTextureObject(dx, res.get()));
//
//    std::string filename = "shader_SRV_UAV_same_resource.dds";
//
//    BOOST_REQUIRE(image->SaveImage(filename));
//    BOOST_REQUIRE(std::filesystem::exists(filename));
//
//    switch (dx->GetType()) {
//        case ninniku::ERenderer::RENDERER_DX11:
//        case ninniku::ERenderer::RENDERER_DX12:
//            CheckFileCRC(filename, 1517223776);
//            break;
//
//        case ninniku::ERenderer::RENDERER_WARP_DX11:
//            CheckFileCRC(filename, 1737166122);
//            break;
//
//        case ninniku::ERenderer::RENDERER_WARP_DX12:
//            throw new std::exception("Invalid test, shouldn't happen");
//            break;
//
//        default:
//            throw std::exception("case should not happen");
//            break;
//    }
//}
//
//BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_SRV_UAV_same_resource_rendergraph, T, FixturesAll, T)
//{
//    // Disable HW GPU support when running on CI
//    if (T::isNull)
//        return;
//
//    auto& dx = ninniku::GetRenderer();
//
//    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
//    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
//        return;
//    }
//
//    BOOST_REQUIRE(LoadShader(dx, "sameResource", T::shaderRoot));
//
//    fg::framegraph framegraph;
//
//    // Final result texture
//    auto param = ninniku::TextureParam::Create();
//
//    param->width = param->height = 512;
//
//    auto numMips = ninniku::CountMips(param->width);
//
//    param->numMips = numMips;
//    param->arraySize = ninniku::CUBEMAP_NUM_FACES;
//    param->depth = 1;
//    param->format = ninniku::TF_R8G8B8A8_UNORM;
//    param->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;
//
//    auto finalOut = framegraph.add_retained_resource("Final Output", param, static_cast<const ninniku::TextureObject*>(nullptr));
//
//    CBGlobal cb = {};
//
//    // Pass 0 : Initialize mip 0
//    struct PassData_0
//    {
//        ninniku::TextureResource* output;
//    };
//
//    auto pass0 = framegraph.add_render_task<PassData_0>(
//        "Initialize Mip 0",
//        [&](PassData_0& data, fg::render_task_builder& builder)
//    {
//        data.output = builder.write <ninniku::TextureResource>(finalOut);
//    },
//        [&dx, &cb](const PassData_0& data)
//    {
//        auto cmd = dx->CreateCommand();
//
//        cmd->shader = "sameResource";
//        cmd->cbufferStr = "CBGlobal";
//        cmd->srvBindings.insert(std::make_pair("srcTex", nullptr));
//
//        auto output = data.output->actual();
//
//        cmd->uavBindings.insert(std::make_pair("dstTex", output->GetUAV(0)));
//
//        auto& param = data.output->description();
//
//        cmd->dispatch[0] = param->width / SAME_RESOURCE_X;
//        cmd->dispatch[1] = param->height / SAME_RESOURCE_Y;
//        cmd->dispatch[2] = param->arraySize / SAME_RESOURCE_Z;
//
//        // constant buffer
//        cb.targetMip = 0;
//
//        BOOST_REQUIRE(dx->UpdateConstantBuffer(cmd->cbufferStr, &cb, sizeof(CBGlobal)));
//        BOOST_REQUIRE(dx->Dispatch(cmd));
//    });
//
//    auto& pass0Data = pass0->data();
//
//    // Pass 1 : Other mips
//    struct PassData_1
//    {
//        ninniku::TextureResource* input;
//        ninniku::TextureResource* output;
//    };
//
//    auto pass1 = framegraph.add_render_task<PassData_1>(
//        "Other Mips",
//        [&pass0Data, &finalOut](PassData_1& data, fg::render_task_builder& builder)
//    {
//        data.input = builder.read(pass0Data.output);
//        data.output = builder.write<ninniku::TextureResource>(finalOut);
//    },
//        [&](const PassData_1& data)
//    {
//        for (uint32_t i = 1; i < numMips; ++i) {
//            auto cmd = dx->CreateCommand();
//
//            cmd->shader = "sameResource";
//            cmd->cbufferStr = "CBGlobal";
//
//            auto input = data.input->actual();
//            auto output = data.output->actual();
//
//            cmd->srvBindings["srcTex"] = input->GetSRVArray(i - 1);
//            cmd->uavBindings["dstTex"] = output->GetUAV(i);
//
//            auto& param = data.input->description();
//
//            cmd->dispatch[0] = std::max(1u, (param->width >> i) / SAME_RESOURCE_X);
//            cmd->dispatch[1] = std::max(1u, (param->height >> i) / SAME_RESOURCE_Y);
//            cmd->dispatch[2] = param->arraySize / SAME_RESOURCE_Z;
//
//            cb.targetMip = i;
//
//            BOOST_REQUIRE(dx->UpdateConstantBuffer(cmd->cbufferStr, &cb, sizeof(CBGlobal)));
//            BOOST_REQUIRE(dx->Dispatch(cmd));
//        }
//    });
//
//    auto& pass1Data = pass1->data();
//
//    framegraph.compile();
//    framegraph.execute();
//
//    // Save to disk
//    auto image = std::make_unique<ninniku::ddsImage>();
//
//    BOOST_REQUIRE(image->InitializeFromTextureObject(dx, pass1Data.output->actual()));
//
//    std::string filename = "shader_SRV_UAV_same_resource.dds";
//
//    BOOST_REQUIRE(image->SaveImage(filename));
//
//    framegraph.clear();
//
//    BOOST_REQUIRE(std::filesystem::exists(filename));
//
//    switch (dx->GetType()) {
//        case ninniku::ERenderer::RENDERER_DX11:
//        case ninniku::ERenderer::RENDERER_DX12:
//            CheckFileCRC(filename, 1517223776);
//            break;
//
//        case ninniku::ERenderer::RENDERER_WARP_DX11:
//            CheckFileCRC(filename, 1737166122);
//            break;
//
//        case ninniku::ERenderer::RENDERER_WARP_DX12:
//            throw new std::exception("Invalid test, shouldn't happen");
//            break;
//
//        default:
//            throw std::exception("case should not happen");
//            break;
//    }
//}
//
//BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_LoadMemory, T, FixturesAll, T)
//{
//    // Disable HW GPU support when running on CI
//    if (T::isNull)
//        return;
//
//    auto& dx = ninniku::GetRenderer();
//
//    auto name = "colorMips";
//    auto fmt = boost::format("%1%\\%2%%3%") % T::shaderRoot % name % dx->GetShaderExtension();
//
//    auto shader = LoadFile(boost::str(fmt));
//
//    BOOST_REQUIRE(dx->LoadShader(name, shader.data(), static_cast<uint32_t>(shader.size())));
//}
//
//BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_colorMips, T, FixturesAll, T)
//{
//    // Disable HW GPU support when running on CI
//    if (T::isNull)
//        return;
//
//    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
//    auto& dx = ninniku::GetRenderer();
//
//    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
//        return;
//    }
//
//    auto resTex = GenerateColoredMips(dx, T::shaderRoot);
//    auto res = std::make_unique<ninniku::cmftImage>();
//
//    BOOST_REQUIRE(res->InitializeFromTextureObject(dx, resTex.get()));
//
//    auto& data = res->GetData();
//
//    switch (dx->GetType()) {
//        case ninniku::ERenderer::RENDERER_DX11:
//        case ninniku::ERenderer::RENDERER_DX12:
//        case ninniku::ERenderer::RENDERER_WARP_DX11:
//            CheckCRC(std::get<0>(data), std::get<1>(data), 3775864256);
//            break;
//
//        case ninniku::ERenderer::RENDERER_WARP_DX12:
//            throw new std::exception("Invalid test, shouldn't happen");
//            break;
//
//        default:
//            throw std::exception("case should not happen");
//            break;
//    }
//}
//
//BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_cubemapDirToArray, T, FixturesAll, T)
//{
//    // Disable HW GPU support when running on CI
//    if (T::isNull)
//        return;
//
//    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
//    auto& dx = ninniku::GetRenderer();
//
//    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
//        return;
//    }
//
//    BOOST_REQUIRE(LoadShader(dx, "colorFaces", T::shaderRoot));
//    BOOST_REQUIRE(LoadShader(dx, "dirToFaces", T::shaderRoot));
//
//    auto marker = dx->CreateDebugMarker("CubemapDirToArray");
//
//    auto param = ninniku::TextureParam::Create();
//    param->width = param->height = 512;
//    param->format = ninniku::DXGIFormatToNinnikuTF(DXGI_FORMAT_R32G32B32A32_FLOAT);
//    param->depth = 1;
//    param->numMips = 1;
//    param->arraySize = ninniku::CUBEMAP_NUM_FACES;
//    param->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;
//
//    auto srcTex = dx->CreateTexture(param);
//    auto dstTex = dx->CreateTexture(param);
//
//    // generate source texture
//    {
//        auto subMarker = dx->CreateDebugMarker("Source Texture");
//
//        // dispatch
//        auto cmd = dx->CreateCommand();
//        cmd->shader = "colorFaces";
//
//        static_assert((COLORFACES_NUMTHREAD_X == COLORFACES_NUMTHREAD_Y) && (COLORFACES_NUMTHREAD_Z == 1));
//        cmd->dispatch[0] = param->width / COLORFACES_NUMTHREAD_X;
//        cmd->dispatch[1] = param->height / COLORFACES_NUMTHREAD_Y;
//        cmd->dispatch[2] = ninniku::CUBEMAP_NUM_FACES / COLORFACES_NUMTHREAD_Z;
//
//        cmd->uavBindings.insert(std::make_pair("dstTex", srcTex->GetUAV(0)));
//
//        BOOST_REQUIRE(dx->Dispatch(cmd));
//    }
//
//    // generate destination texture by sampling source using direction vectors
//    {
//        auto subMarker = dx->CreateDebugMarker("Destination Texture");
//
//        // dispatch
//        auto cmd = dx->CreateCommand();
//        cmd->shader = "dirToFaces";
//
//        static_assert((DIRTOFACE_NUMTHREAD_X == DIRTOFACE_NUMTHREAD_Y) && (DIRTOFACE_NUMTHREAD_Z == 1));
//        cmd->dispatch[0] = param->width / DIRTOFACE_NUMTHREAD_X;
//        cmd->dispatch[1] = param->height / DIRTOFACE_NUMTHREAD_Y;
//        cmd->dispatch[2] = ninniku::CUBEMAP_NUM_FACES / DIRTOFACE_NUMTHREAD_Z;
//
//        cmd->ssBindings.insert(std::make_pair("ssPoint", dx->GetSampler(ninniku::ESamplerState::SS_Point)));
//        cmd->srvBindings.insert(std::make_pair("srcTex", srcTex->GetSRVCube()));
//        cmd->uavBindings.insert(std::make_pair("dstTex", dstTex->GetUAV(0)));
//
//        BOOST_REQUIRE(dx->Dispatch(cmd));
//    }
//
//    auto srcImg = std::make_unique<ninniku::ddsImage>();
//
//    BOOST_REQUIRE(srcImg->InitializeFromTextureObject(dx, srcTex.get()));
//    BOOST_REQUIRE(srcImg->SaveImage("shader_cubemapDirToArray_src.dds"));
//
//    auto srcData = srcImg->GetData();
//    auto srcHash = GetCRC(std::get<0>(srcData), std::get<1>(srcData));
//    auto dstImg = std::make_unique<ninniku::ddsImage>();
//
//    BOOST_REQUIRE(dstImg->InitializeFromTextureObject(dx, srcTex.get()));
//    BOOST_REQUIRE(srcImg->SaveImage("shader_cubemapDirToArray_dst.dds"));
//    auto dstData = dstImg->GetData();
//    auto dstHash = GetCRC(std::get<0>(dstData), std::get<1>(dstData));
//
//    BOOST_REQUIRE(srcHash == dstHash);
//}
//
//BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_genMips, T, FixturesAll, T)
//{
//    // Disable HW GPU support when running on CI
//    if (T::isNull)
//        return;
//
//    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
//    auto& dx = ninniku::GetRenderer();
//
//    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
//        return;
//    }
//
//    auto image = std::make_unique<ninniku::ddsImage>();
//
//    BOOST_REQUIRE(image->Load("data/Cathedral01.dds"));
//
//    auto resTex = Generate2DTexWithMips(dx, image.get(), T::shaderRoot);
//    auto res = std::make_unique<ninniku::cmftImage>();
//
//    BOOST_REQUIRE(res->InitializeFromTextureObject(dx, resTex.get()));
//
//    auto& data = res->GetData();
//
//    switch (dx->GetType()) {
//        case ninniku::ERenderer::RENDERER_DX11:
//        case ninniku::ERenderer::RENDERER_DX12:
//        case ninniku::ERenderer::RENDERER_WARP_DX11:
//            CheckCRC(std::get<0>(data), std::get<1>(data), 946385041);
//            break;
//
//        case ninniku::ERenderer::RENDERER_WARP_DX12:
//            throw new std::exception("Invalid test, shouldn't happen");
//            break;
//
//        default:
//            throw std::exception("case should not happen");
//            break;
//    }
//}
//
//BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_resize, T, FixturesAll, T)
//{
//    // Disable HW GPU support when running on CI
//    if (T::isNull)
//        return;
//
//    auto& dx = ninniku::GetRenderer();
//
//    // There is something wrong with WARP but it's working fine for DX12 HW so disable it
//    if (dx->GetType() == ninniku::ERenderer::RENDERER_WARP_DX12) {
//        return;
//    }
//
//    auto image = std::make_unique<ninniku::cmftImage>();
//
//    BOOST_REQUIRE(image->Load("data/Cathedral01.hdr"));
//
//    auto needFix = image->IsRequiringFix();
//    auto newSize = std::get<1>(needFix);
//
//    auto srcParam = image->CreateTextureParam(ninniku::RV_SRV);
//    auto marker = dx->CreateDebugMarker("Resize");
//    auto srcTex = dx->CreateTexture(srcParam);
//
//    auto dstParam = ninniku::TextureParam::Create();
//    dstParam->width = newSize;
//    dstParam->height = newSize;
//    dstParam->format = srcTex->GetDesc()->format;
//    dstParam->numMips = 1;
//    dstParam->arraySize = 6;
//    dstParam->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;
//
//    auto dst = ResizeImage(dx, srcTex, needFix, T::shaderRoot);
//
//    auto res = std::make_unique<ninniku::cmftImage>();
//
//    BOOST_REQUIRE(res->InitializeFromTextureObject(dx, dst.get()));
//
//    auto& data = res->GetData();
//
//    switch (dx->GetType()) {
//        case ninniku::ERenderer::RENDERER_DX12:
//        case ninniku::ERenderer::RENDERER_DX11:
//            CheckCRC(std::get<0>(data), std::get<1>(data), 1396798068);
//            break;
//
//        case ninniku::ERenderer::RENDERER_WARP_DX11:
//            CheckCRC(std::get<0>(data), std::get<1>(data), 457450649);
//            break;
//
//        case ninniku::ERenderer::RENDERER_WARP_DX12:
//            throw new std::exception("Invalid test, shouldn't happen");
//            break;
//
//        default:
//            throw std::exception("case should not happen");
//            break;
//    }
//}
//
//BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_structuredBuffer, T, FixturesAll, T)
//{
//    // Disable HW GPU support when running on CI
//    if (T::isNull)
//        return;
//
//    auto& dx = ninniku::GetRenderer();
//    BOOST_REQUIRE(LoadShader(dx, "fillBuffer", T::shaderRoot));
//
//    auto params = ninniku::BufferParam::Create();
//
//    params->numElements = 16;
//    params->elementSize = sizeof(uint32_t);
//    params->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;
//
//    auto srcBuffer = dx->CreateBuffer(params);
//
//    // fill structured buffer
//    {
//        auto subMarker = dx->CreateDebugMarker("Fill StructuredBuffer");
//
//        // dispatch
//        auto cmd = dx->CreateCommand();
//        cmd->shader = "fillBuffer";
//
//        cmd->dispatch[0] = FILLBUFFER_NUMTHREAD_X;
//        cmd->dispatch[1] = FILLBUFFER_NUMTHREAD_Y;
//        cmd->dispatch[2] = FILLBUFFER_NUMTHREAD_Z;
//
//        cmd->uavBindings.insert(std::make_pair("dstBuffer", srcBuffer->GetUAV()));
//
//        BOOST_REQUIRE(dx->Dispatch(cmd));
//    }
//
//    auto dstBuffer = dx->CreateBuffer(srcBuffer);
//
//    auto& data = dstBuffer->GetData();
//
//    switch (dx->GetType()) {
//        case ninniku::ERenderer::RENDERER_DX11:
//        case ninniku::ERenderer::RENDERER_DX12:
//        case ninniku::ERenderer::RENDERER_WARP_DX11:
//        case ninniku::ERenderer::RENDERER_WARP_DX12:
//            CheckCRC(std::get<0>(data), std::get<1>(data), 3783883977);
//            break;
//
//        default:
//            throw std::exception("case should not happen");
//            break;
//    }
//}
//
//BOOST_AUTO_TEST_SUITE_END()