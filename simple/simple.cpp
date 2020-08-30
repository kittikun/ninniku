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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <boost/format.hpp>
#pragma clang diagnostic pop

#include <rapidxml/rapidxml.hpp>
#include <DirectXColors.h>

// we want to do like unit_test data
static std::filesystem::path CurrentDir;

struct Component
{
    std::string entry;
    std::string path;
    ninniku::EShaderType type;
};

struct PipelineState
{
    std::string name;
    std::vector<std::unique_ptr<Component>> components;
};

void ChangeToDataDirectory(std::string_view dir)
{
    if (CurrentDir.empty())
        CurrentDir = std::filesystem::current_path().parent_path() / "unit_test";

    auto dataDir = CurrentDir / "data" / dir;

    std::filesystem::current_path(dataDir);
}

void ChangeToOutDirectory(std::string_view dir)
{
    // For output files, create an platform folder in out/ if it doesn't already exist
    // then change working directory to that folder

    if (CurrentDir.empty())
        CurrentDir = std::filesystem::current_path().parent_path() / "unit_test";

    auto outDir = CurrentDir / "out" / dir;

    if (!std::filesystem::is_directory(outDir) || !std::filesystem::exists(outDir)) {
        std::filesystem::create_directories(outDir);
    }

    std::filesystem::current_path(outDir);
}

std::filesystem::path GetFilename(const std::string& psName, ninniku::EShaderType type, const std::string_view& ext)
{
    std::string res;
    boost::format fmt;

    switch (type) {
        case ninniku::EShaderType::ST_Root_Signature:
        {
            fmt = boost::format("%1%_rs%2%") % psName % ext;
        }
        break;

        case ninniku::EShaderType::ST_Vertex:
        {
            fmt = boost::format("%1%_vs%2%") % psName % ext;
        }
        break;

        case ninniku::EShaderType::ST_Pixel:
        {
            fmt = boost::format("%1%_ps%2%") % psName % ext;
        }
        break;

        default:
            break;
    }

    return boost::str(fmt);
}

std::vector<std::unique_ptr<PipelineState>> ParseTOC()
{
    std::vector<std::unique_ptr<PipelineState>> res;

    ChangeToDataDirectory("shader_compiler");

    rapidxml::xml_document<> doc;

    // read TOC and fill a list of compiled shaders to check
    std::ifstream xmlFile("shaders.xml");
    std::vector<char> buffer((std::istreambuf_iterator<char>(xmlFile)), std::istreambuf_iterator<char>());

    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);

    auto root = doc.first_node("PipelineStates");

    // Parse the buffer using the xml file parsing library into doc
    for (auto iter = root->first_node("PipelineState"); iter; iter = iter->next_sibling()) {
        auto ps = new PipelineState();

        ps->name = iter->first_attribute("name")->value();

        // RootSignature
        {
            auto rsXml = iter->first_node("RootSignature");

            if (rsXml != nullptr) {
                auto component = new Component();

                component->type = ninniku::EShaderType::ST_Root_Signature;
                component->path = rsXml->first_attribute("path")->value();

                ps->components.emplace_back(component);
            }
        }

        auto lmbd = [&](const std::string_view& typeName, ninniku::EShaderType type)
        {
            auto xml = iter->first_node(typeName.data());

            if (xml != nullptr) {
                auto component = new Component();

                component->type = type;
                component->path = xml->first_attribute("path")->value();
                component->entry = xml->first_attribute("entry")->value();

                ps->components.emplace_back(component);
            }
        };

        lmbd("VertexShader", ninniku::EShaderType::ST_Vertex);
        lmbd("PixelShader", ninniku::EShaderType::ST_Pixel);

        res.emplace_back(ps);
    }

    return res;
}

int main()
{
    if (!ninniku::Initialize(ninniku::ERenderer::RENDERER_DX12, ninniku::EInitializationFlags::IF_None, ninniku::ELogLevel::LL_FULL))
        return -1;

    auto& dx = ninniku::GetRenderer();

    // Swap chains
    auto scDesc = ninniku::SwapchainParam::Create();

    scDesc->bufferCount = 2;
    scDesc->format = ninniku::EFormat::F_R8G8B8A8_UNORM;
    scDesc->height = 768;
    scDesc->width = 1024;
    scDesc->hwnd = ninniku::MakeWindow(scDesc->width, scDesc->height, true);
    scDesc->vsync = false;

    auto swapChain = dx->CreateSwapChain(scDesc);

    // input layout
    {
        ninniku::InputLayoutDesc desc;

        desc.name = "simple";
        desc.elements.resize(2);
        desc.elements[0].name = "POSITION";
        desc.elements[0].format = ninniku::F_R32G32B32_FLOAT;
        desc.elements[1].name = "COLOR";
        desc.elements[1].format = ninniku::F_R32G32B32A32_FLOAT;

        dx->RegisterInputLayout(desc);
    }

    // load shaders
    {
        auto pipelineStates = ParseTOC();

        ChangeToOutDirectory("shader_compiler");

        for (auto i = 0u; i < pipelineStates.size(); ++i) {
            auto& ps = pipelineStates[i];
            ninniku::GraphicPipelineStateParam param;

            param.name = ps->name;
            param.inputLayout = "simple";
            param.rtFormat = scDesc->format;

            for (auto& component : ps->components) {
                auto filename = GetFilename(ps->name, component->type, dx->GetShaderExtension());

                param.shaders[component->type] = filename.stem().string();

                if (!dx->LoadShader(component->type, ps->name, filename))
                    throw new std::exception("LoadShader failed");
            }

            if (!dx->CreatePipelineState(param))
                throw new std::exception("CreatePipelineState failed");
        }
    }

    // vertex buffer
    {
        //auto params = ninniku::BufferParam::Create();
    }

    MSG msg;
    bool running = true;

    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if ((msg.message == WM_QUIT) || (msg.message == WM_CLOSE)) {
            running = false;
        } else {
            auto bufferIndex = swapChain->GetCurrentBackBufferIndex();
            auto frameRT = swapChain->GetRT(bufferIndex);

            ninniku::ClearRenderTargetParam clearParam;

            clearParam.color = DirectX::Colors::Cyan;
            clearParam.dstRT = frameRT;
            clearParam.index = bufferIndex;

            if (!dx->ClearRenderTarget(clearParam))
                throw std::exception("failed to clear");

            if (!dx->Present(swapChain))
                throw std::exception("failed to present");
        }
    };

    auto image = std::make_unique<ninniku::ddsImage>();

    if (!image->InitializeFromSwapChain(dx, swapChain))
        throw new std::exception("InitializeFromSwapChain failed");

    if (!image->SaveImage("test.dds"))
        throw new std::exception("SaveImage failed");

    ninniku::Terminate();
}