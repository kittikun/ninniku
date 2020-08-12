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
#include <ninniku/core/renderer/rendergraph.h>
#include <ninniku/core/image/cmft.h>
#include <ninniku/core/image/dds.h>

#include <filesystem>

#include "../unit_test/src/shaders/cbuffers.h"
#include "../unit_test/src/shaders/dispatch.h"

#ifdef _DEBUG
static constexpr std::string_view DX12ShadersRoot = "..\\unit_test\\bin\\Debug\\dx12\\";
#else
static constexpr std::string_view DX12ShadersRoot = "..\\unit_test\\bin\\Release\\dx12\\";
#endif

bool LoadShader(ninniku::RenderDeviceHandle& dx, const std::string& name)
{
    auto absolutePath = std::filesystem::absolute(DX12ShadersRoot);
    auto path = absolutePath.string() + name + std::string(dx->GetShaderExtension());

    return dx->LoadShader(path);
}

int main()
{
    if (!ninniku::Initialize(ninniku::ERenderer::RENDERER_DX12, ninniku::EInitializationFlags::IF_None, ninniku::ELogLevel::LL_FULL))
        return -1;

    auto& dx = ninniku::GetRenderer();

    LoadShader(dx, "sameResource");

    fg::framegraph framegraph;

    // Final result texture
    auto param = ninniku::TextureParam::Create();

    param->width = param->height = 512;

    auto numMips = ninniku::CountMips(param->width);

    param->numMips = numMips;
    param->arraySize = ninniku::CUBEMAP_NUM_FACES;
    param->depth = 1;
    param->format = ninniku::TF_R8G8B8A8_UNORM;
    param->viewflags = ninniku::RV_SRV | ninniku::RV_UAV;

    auto finalOut = framegraph.add_retained_resource("Final Output", param, static_cast<const ninniku::TextureObject*>(nullptr));

    CBGlobal cb = {};

    // Pass 0 : Initialize mip 0
    struct PassData_0
    {
        ninniku::TextureResource* output;
    };

    auto pass0 = framegraph.add_render_task<PassData_0>(
        "Initialize Mip 0",
        [&finalOut](PassData_0& data, fg::render_task_builder& builder)
    {
        data.output = builder.write <ninniku::TextureResource>(finalOut);
    },
        [&dx, &cb](const PassData_0& data)
    {
        auto cmd = dx->CreateCommand();

        cmd->shader = "sameResource";
        cmd->cbufferStr = "CBGlobal";
        cmd->srvBindings.insert(std::make_pair("srcTex", nullptr));

        auto output = data.output->actual();

        cmd->uavBindings.insert(std::make_pair("dstTex", output->GetUAV(0)));

        auto& param = data.output->description();

        cmd->dispatch[0] = param->width / SAME_RESOURCE_X;
        cmd->dispatch[1] = param->height / SAME_RESOURCE_Y;
        cmd->dispatch[2] = param->arraySize / SAME_RESOURCE_Z;

        // constant buffer
        cb.targetMip = 0;

        auto res = dx->UpdateConstantBuffer(cmd->cbufferStr, &cb, sizeof(CBGlobal));
        res = dx->Dispatch(cmd);
    });

    auto& pass0Data = pass0->data();

    // Pass 1 : Other mips
    struct PassData_1
    {
        ninniku::TextureResource* input;
        ninniku::TextureResource* output;
    };

    auto pass1 = framegraph.add_render_task<PassData_1>(
        "Other Mips",
        [&pass0Data, &finalOut](PassData_1& data, fg::render_task_builder& builder)
    {
        data.input = builder.read(pass0Data.output);
        data.output = builder.write(pass0Data.output);
    },
        [&dx, &cb, &numMips](const PassData_1& data)
    {
        for (uint32_t i = 1; i < numMips; ++i) {
            auto cmd = dx->CreateCommand();

            cmd->shader = "sameResource";
            cmd->cbufferStr = "CBGlobal";

            auto input = data.input->actual();
            auto output = data.output->actual();

            cmd->srvBindings["srcTex"] = input->GetSRVArray(i - 1);
            cmd->uavBindings["dstTex"] = output->GetUAV(i);

            auto& param = data.input->description();

            cmd->dispatch[0] = std::max(1u, (param->width >> i) / SAME_RESOURCE_X);
            cmd->dispatch[1] = std::max(1u, (param->height >> i) / SAME_RESOURCE_Y);

            cb.targetMip = i;

            auto res = dx->UpdateConstantBuffer(cmd->cbufferStr, &cb, sizeof(CBGlobal));
            res = dx->Dispatch(cmd);
        }
    });

    auto& pass1Data = pass1->data();

    framegraph.compile();
    framegraph.execute();

    // Save to disk
    auto image = std::make_unique<ninniku::ddsImage>();

    auto res = image->InitializeFromTextureObject(dx, pass1Data.output->actual());

    std::string filename = "shader_SRV_UAV_same_resource.dds";

    res = image->SaveImage(filename);

    framegraph.export_graphviz("framegraph.gv");
    framegraph.clear();

    ninniku::Terminate();
}