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

#include "pch.h"

#include "utils/log.h"
#include "utils/misc.h"
#include "dx11/DX11.h"
#include "process.h"

#include <boost/program_options.hpp>
#include <iostream>
#include <windows.h>

#ifdef _USE_RENDERDOC
#include <renderdoc_app.h>
#endif

namespace po = boost::program_options;

enum CLIFlags
{
    CLI_HELP = 0,           // only print help
    CLI_RUN = 1 << 1,       // run process per file
    CLI_VERBOSE = 1 << 2,   // verbose
    CLI_RENDERDOC = 1 << 3  // renderdoc capture the processing
};

#ifdef _USE_RENDERDOC
RENDERDOC_API_1_1_2* renderDocApi = nullptr;
#endif

int ParseCommandLine(int ac, char* av[], std::vector<std::string>& toProcess, std::string& renderDocCapturePath)
{
    int res = CLI_HELP;

    try {
        po::options_description generic("Generic options");
        generic.add_options()
        ("help", "produce help message")
        ("capture,c", po::value<std::string>(), "capture the rendering state with renderdoc")
        ("verbose,v", "output all log messages")
        ("version", "print build information")
        ;

        po::options_description visible("Allowed options");
        visible.add(generic);

        po::options_description hidden("Hidden options");
        hidden.add_options()
        ("input,i", po::value<std::vector<std::string>>(), "input file")
        ;

        po::options_description cmdline_options;
        cmdline_options.add(generic).add(hidden);

        po::positional_options_description p;
        p.add("input", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(ac, av).options(cmdline_options).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help") || (ac == 1)) {
            std::cout << "ninniku HLSL compute shader framework" << std::endl;
            std::cout << std::endl;
            std::cout << "Usage: cubemap [OPTIONS] [FILES]" << std::endl;
            std::cout << visible << std::endl;
            return res;
        }

        if (vm.count("version")) {
            auto fmt = boost::format("Build date: %1% %2%") % __DATE__ % __TIME__;

            std::cout << boost::str(fmt) << std::endl;
            return res;
        }

        if (vm.count("verbose")) {
            res |= CLI_VERBOSE;
        }

        if (vm.count("capture")) {
            res |= CLI_RENDERDOC;

            renderDocCapturePath = vm["capture"].as<std::string>();
        }

        if (vm.count("input")) {
            auto fileList = vm["input"].as<std::vector<std::string>>();

            toProcess.swap(fileList);
        }
    } catch (std::exception& e) {
        std::cout << e.what();
        return 1;
    }

    res |= CLI_RUN;

    return res;
}

#ifdef _USE_RENDERDOC
void LoadRenderDoc()
{
    LOG << "Loading RenderDoc..";

    std::string path = "external/renderdoc/renderdoc.dll";
    auto hInst = LoadLibrary(ninniku::strToWStr(path).c_str());

    if (hInst == nullptr) {
        auto fmt = boost::format("Failed to load %1%") % path;
        LOGE << boost::str(fmt);

        return;
    } else {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(hInst, "RENDERDOC_GetAPI");
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&renderDocApi);

        if (ret != 1) {
            LOGE << "Failed to get function pointer to RenderDoc API";
        }
    }
}
#endif

int main(int ac, char* av[])
{
    std::vector<std::string> toProcess;
    std::string renderDocCapturePath;

    auto parsed = ParseCommandLine(ac, av, toProcess, renderDocCapturePath);

    if ((parsed & CLI_RUN) == 0)
        return parsed;

    ninniku::Log::Initialize((parsed & CLI_VERBOSE) != 0);

    LOG << "ninniku HLSL compute shader framework";

#ifdef _USE_RENDERDOC
    if ((parsed & CLI_RENDERDOC) != 0)
        LoadRenderDoc();
#endif

    auto basePath(boost::filesystem::current_path());
    auto fileCount = toProcess.size();

    std::shared_ptr<ninniku::DX11> dxApp;

    if (fileCount > 0) {
        dxApp.reset(new ninniku::DX11());

        if (!dxApp->Initialize()) {
            LOGE << "DX11App::Initialize failed";
            return 1;
        }

#ifdef _USE_RENDERDOC
        if (renderDocApi != nullptr) {
            renderDocApi->SetCaptureFilePathTemplate(renderDocCapturePath.c_str());
            renderDocApi->StartFrameCapture(NULL, NULL);
        }
#endif
    }

    LOG << boost::str(boost::format{ "Processing %1% files..." } % fileCount);

    for (int i = 0; i < fileCount; ++i) {
        auto path = basePath / toProcess[i];
        auto filePosition = boost::format{ "%1%/%2%" } % i % fileCount;

        bool failed = false;

        if (boost::filesystem::exists(path)) {
            auto fmt = boost::format{ "(%1%) Processing: %2%" } % filePosition % path;

            LOG_INDENT_START << boost::str(fmt);

            ninniku::Processor processor{ dxApp };

            failed = !processor.ProcessImage(toProcess[i]);
        } else {
            auto fmt = boost::format("(%1%) File doesn't exist: %2%") % filePosition % path;
            LOGE_INDENT_START << boost::str(fmt);
            failed = true;
        }

        if (failed) {
            LOGE_INDENT_END;
        } else {
            LOG_INDENT_END;
        }
    }

#ifdef _USE_RENDERDOC
    if (renderDocApi != nullptr)
        renderDocApi->EndFrameCapture(NULL, NULL);
#endif

    return 0;
}