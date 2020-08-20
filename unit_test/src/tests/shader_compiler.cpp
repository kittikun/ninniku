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

#include "../fixture.h"
#include "../utils.h"

#include <rapidxml/rapidxml.hpp>
#include <filesystem>
#include <fstream>
#include <vector>

struct RootSignature
{
    std::string_view name_;
    std::string_view path_;
};

struct PipelineState
{
    std::string_view name_;
    std::unique_ptr<RootSignature> rs_;
};

BOOST_AUTO_TEST_SUITE(ShaderCompiler)

BOOST_FIXTURE_TEST_CASE(shader_compiler_check_exist, SetupFixtureNull)
{
    std::vector<std::unique_ptr<PipelineState>> pipelineStates;

    ChangeToDataDirectory("shader_compiler");

    rapidxml::xml_document<> doc;

    // read TOC and fill a list of compiled shaders to check
    std::ifstream xmlFile("shaders.xml");
    std::vector<char> buffer((std::istreambuf_iterator<char>(xmlFile)), std::istreambuf_iterator<char>());

    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);

    // Parse the buffer using the xml file parsing library into doc
    for (auto iter = doc.first_node("PipelineState"); iter; iter = iter->next_sibling()) {
        auto ps = new PipelineState();

        ps->name_ = iter->first_attribute("name")->value();

        auto rsXml = iter->first_node("RootSignature");

        if (rsXml != nullptr) {
            auto rs = new RootSignature();

            rs->name_ = rsXml->first_attribute("name")->value();
            rs->path_ = rsXml->first_attribute("path")->value();

            ps->rs_.reset(rs);
        }

        pipelineStates.emplace_back(ps);
    }

    ChangeToOutDirectory("shader_compiler");

    for (auto& ps : pipelineStates) {
        auto filename = std::string(ps->rs_->name_) + ".rso";

        BOOST_REQUIRE(std::filesystem::exists(filename));
    }
}

BOOST_AUTO_TEST_SUITE_END()