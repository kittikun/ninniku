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

#include <fg/framegraph.hpp>

using TextureResource = fg::resource<std::shared_ptr<ninniku::TextureParam>, const ninniku::TextureObject>;

namespace fg
{
    template<>
    std::unique_ptr<const ninniku::TextureObject> realize(const std::shared_ptr<ninniku::TextureParam>& description)
    {
        auto& dx = ninniku::GetRenderer();

        return dx->CreateTexture(description);
    }
}

int main()
{
    if (!ninniku::Initialize(ninniku::ERenderer::RENDERER_DX12, ninniku::EInitializationFlags::IF_None, ninniku::ELogLevel::LL_FULL))
        return -1;

    fg::framegraph framegraph;

    auto param = ninniku::TextureParam::Create();

    param->width = 1024;
    param->height = 768;
    param->numMips = 1;
    param->arraySize = 1;
    param->depth = 1;
    param->format = ninniku::TF_R8G8B8A8_UNORM;
    param->viewflags = ninniku::RV_SRV;

    auto retained_resource = framegraph.add_retained_resource("Retained Resource 1", param, static_cast<const ninniku::TextureObject*>(nullptr));

    // First render task declaration.
    struct render_task_1_data
    {
        TextureResource* output1;
        TextureResource* output2;
        TextureResource* output3;
        TextureResource* output4;
    };

    auto render_task_1 = framegraph.add_render_task<render_task_1_data>(
        "Render Task 1",
        [&](render_task_1_data& data, fg::render_task_builder& builder)
    {
        auto param = ninniku::TextureParam::Create();

        param->width = 1024;
        param->height = 768;
        param->numMips = 1;
        param->arraySize = 1;
        param->depth = 1;
        param->format = ninniku::TF_R8G8B8A8_UNORM;
        param->viewflags = ninniku::RV_SRV;

        data.output1 = builder.create<TextureResource>("Resource 1", param);
        data.output2 = builder.create<TextureResource>("Resource 2", param);
        data.output3 = builder.create<TextureResource>("Resource 3", param);
        data.output4 = builder.write <TextureResource>(retained_resource);
    },
        [=](const render_task_1_data& data)
    {
        // Perform actual rendering. You may load resources from CPU by capturing them.
        auto actual1 = data.output1->actual();
        auto actual2 = data.output2->actual();
        auto actual3 = data.output3->actual();
        auto actual4 = data.output4->actual();
    });

    auto& data_1 = render_task_1->data();

    // Second render pass declaration.
    struct render_task_2_data
    {
        TextureResource* input1;
        TextureResource* input2;
        TextureResource* output1;
        TextureResource* output2;
    };

    auto render_task_2 = framegraph.add_render_task<render_task_2_data>(
        "Render Task 2",
        [&](render_task_2_data& data, fg::render_task_builder& builder)
    {
        data.input1 = builder.read(data_1.output1);
        data.input2 = builder.read(data_1.output2);
        data.output1 = builder.write(data_1.output3);

        auto param = ninniku::TextureParam::Create();

        param->width = 1024;
        param->height = 768;
        param->numMips = 1;
        param->arraySize = 1;
        param->depth = 1;
        param->format = ninniku::TF_R8G8B8A8_UNORM;
        param->viewflags = ninniku::RV_SRV;

        data.output2 = builder.create<TextureResource>("Resource 4", param);
    },
        [=](const render_task_2_data& data)
    {
        // Perform actual rendering. You may load resources from CPU by capturing them.
        auto actual1 = data.input1->actual();
        auto actual2 = data.input2->actual();
        auto actual3 = data.output1->actual();
        auto actual4 = data.output2->actual();
    });

    auto& data_2 = render_task_2->data();

    // Third pass
    struct render_task_3_data
    {
        TextureResource* input1;
        TextureResource* input2;
        TextureResource* output;
    };

    auto render_task_3 = framegraph.add_render_task<render_task_3_data>(
        "Render Task 3",
        [&](render_task_3_data& data, fg::render_task_builder& builder)
    {
        data.input1 = builder.read(data_2.output1);
        data.input2 = builder.read(data_2.output2);
        data.output = builder.write(retained_resource);
    },
        [=](const render_task_3_data& data)
    {
        // Perform actual rendering. You may load resources from CPU by capturing them.
        auto actual1 = data.input1->actual();
        auto actual2 = data.input2->actual();
        auto actual3 = data.output->actual();
    });

    framegraph.compile();
    for (auto i = 0; i < 100; i++)
        framegraph.execute();
    framegraph.export_graphviz("framegraph.gv");
    framegraph.clear();

    ninniku::Terminate();
}