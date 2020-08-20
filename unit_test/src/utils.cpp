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

#include "utils.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <boost/format.hpp>
#pragma clang diagnostic pop

#include <fstream>

void ChangeDirectory(std::string_view dir)
{
    // For output files, create an platform folder in out/ if it doesn't already exist
    // then change working directory to that folder

    static std::filesystem::path currentDir;

    if (currentDir.empty())
        currentDir = std::filesystem::current_path();

    auto outDir = currentDir / "out" / dir;

    if (!std::filesystem::is_directory(outDir) || !std::filesystem::exists(outDir)) {
        std::filesystem::create_directories(outDir);
    }

    std::filesystem::current_path(outDir);
}

std::vector<uint8_t> LoadFile(const std::filesystem::path& path)
{
    std::ifstream ifs(path.c_str(), std::ios::binary | std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    std::vector<uint8_t> result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(reinterpret_cast<char*>(result.data()), pos);

    return result;
}

bool LoadShader(ninniku::RenderDeviceHandle& dx, const std::string_view& name, const std::string_view& shaderRoot)
{
    auto fmt = boost::format("%1%\\%2%%3%") % shaderRoot % name % dx->GetShaderExtension();

    return dx->LoadShader(boost::str(fmt));
}

bool IsAppVeyor()
{
    char* value = nullptr;
    size_t len;

    _dupenv_s(&value, &len, "APPVEYOR");

    if ((value != nullptr) && (len > 0) && (strcmp(value, "True") == 0))
        return true;

    return false;
}