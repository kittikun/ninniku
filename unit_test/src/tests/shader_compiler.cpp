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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <boost/test/unit_test.hpp>
#pragma clang diagnostic pop

#define DO_SHADER_COMPILER_TESTS 0

#if DO_SHADER_COMPILER_TESTS

#include <ninniku/ninniku.h>
#include <ninniku/core/renderer/renderdevice.h>

#include "../fixture.h"
#include "../utils.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <boost/format.hpp>
#pragma clang diagnostic pop

#include <rapidxml/rapidxml.hpp>
#include <filesystem>
#include <fstream>
#include <vector>

struct Component
{
    std::string entry_;
    std::string path_;
    ninniku::EShaderType type;
};

struct PipelineState
{
    std::string name;
    std::vector<std::unique_ptr<Component>> components_;
};

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
                component->path_ = rsXml->first_attribute("path")->value();

                ps->components_.emplace_back(component);
            }
        }

        auto lmbd = [&](const std::string_view& typeName, ninniku::EShaderType type)
        {
            auto xml = iter->first_node(typeName.data());

            if (xml != nullptr) {
                auto component = new Component();

                component->type = type;
                component->path_ = xml->first_attribute("path")->value();
                component->entry_ = xml->first_attribute("entry")->value();

                ps->components_.emplace_back(component);
            }
        };

        lmbd("VertexShader", ninniku::EShaderType::ST_Vertex);
        lmbd("PixelShader", ninniku::EShaderType::ST_Pixel);

        res.emplace_back(ps);
    }

    return res;
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

BOOST_AUTO_TEST_SUITE(ShaderCompiler)

BOOST_FIXTURE_TEST_CASE(shader_compiler_check_exist, SetupFixtureNull)
{
    auto pipelineStates = ParseTOC();

    ChangeToOutDirectory("shader_compiler");

    for (auto& ps : pipelineStates) {
        for (auto& component : ps->components_) {
            auto filename = GetFilename(ps->name, component->type, ".dxco");

            BOOST_REQUIRE(std::filesystem::exists(filename));
        }
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(shader_compiler_load, T, FixturesDX12All, T)
{
    // Disable HW GPU support when running on CI
    if (T::isNull)
        return;

    auto pipelineStates = ParseTOC();

    ChangeToOutDirectory("shader_compiler");

    auto& dx = ninniku::GetRenderer();

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

    for (auto i = 0u; i < pipelineStates.size(); ++i) {
        auto& ps = pipelineStates[i];
        ninniku::GraphicPipelineStateParam param;

        param.name = ps->name;
        param.inputLayout = "simple";
        param.rtFormat = ninniku::EFormat::F_R8G8B8A8_UNORM;

        for (auto& component : ps->components_) {
            auto filename = GetFilename(ps->name, component->type, dx->GetShaderExtension());

            param.shaders[component->type] = filename.stem().string();

            BOOST_REQUIRE(dx->LoadShader(component->type, ps->name, filename));
        }

        BOOST_REQUIRE(dx->CreatePipelineState(param));
    }
}

BOOST_AUTO_TEST_SUITE_END()
#endif // DO_SHADER_COMPILER_TESTS