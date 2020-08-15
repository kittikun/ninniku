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

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wassume"
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#pragma clang diagnostic pop

#include "ninniku/ninniku.h"

#define LOG BOOST_LOG_SEV(ninniku::Log::boost_log::get(), ninniku::Log::Log_Core) << ninniku::Log::GetIndent()
#define LOGD BOOST_LOG_SEV(ninniku::Log::boost_log::get(), ninniku::Log::Log_DX) << ninniku::Log::GetIndent()
#define LOGDF(X) LOGD << boost::str(X)
#define LOGE BOOST_LOG_SEV(ninniku::Log::boost_log::get(), ninniku::Log::Log_Error) << ninniku::Log::GetIndent()
#define LOGEF(X) LOGE << boost::str(X)
#define LOGW BOOST_LOG_SEV(ninniku::Log::boost_log::get(), ninniku::Log::Log_Warning) << ninniku::Log::GetIndent()
#define LOGWF(X) LOGW << boost::str(X)

#define LOG_INDENT_START ninniku::Log::StartIndent(); LOG
#define LOGD_INDENT_START ninniku::Log::StartIndent(); LOGD
#define LOGE_INDENT_START ninniku::Log::StartIndent(); LOGE

#define LOG_INDENT_END LOG << "..done"; ninniku::Log::EndIndent()
#define LOGD_INDENT_END LOGD << "..done"; ninniku::Log::EndIndent()
#define LOGE_INDENT_END LOGE << "..done"; ninniku::Log::EndIndent()

namespace ninniku
{
    namespace Log
    {
        enum BoostLogLevel
        {
            Log_DX,
            Log_Core,
            Log_Warning,
            Log_Error,
            Log_Level_Count
        };

        BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(boost_log, boost::log::sources::severity_logger_mt<BoostLogLevel>);

        void Initialize(const ELogLevel level);
        void StartIndent();
        void EndIndent();
        std::string GetIndent();
    } // namespace Log
} // namespace ninniku
