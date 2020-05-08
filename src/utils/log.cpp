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

#include "pch.h"
#include "log.h"

#include "ninniku/ninniku.h"
#include "ninniku/core/renderer/renderdevice.h"

#include <boost/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/debug_output_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;

namespace ninniku
{
    namespace Log
    {
        BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", BoostLogLevel);

        //////////////////////////////////////////////////////////////////////////
        /// Indentation manager
        //////////////////////////////////////////////////////////////////////////
        class LogContext
        {
            // no copy of any kind allowed
            LogContext(const LogContext&) = delete;
            LogContext& operator=(const LogContext&) = delete;
            LogContext(LogContext&&) = delete;
            LogContext& operator=(LogContext&&) = delete;

        public:
            LogContext() : count{ 0 }, isNew{ 0 }
            {
            }

            // Public because of laziness
            uint32_t count;
            uint32_t isNew;
        };

        static LogContext sLogContext;
        static bool sLogInitizalized;

        //////////////////////////////////////////////////////////////////////////
        /// Color console
        //////////////////////////////////////////////////////////////////////////
        class ColorConsoleSink : public boost::log::sinks::basic_formatted_sink_backend<char, boost::log::sinks::synchronized_feeding>
        {
            // no copy of any kind allowed
            ColorConsoleSink(const ColorConsoleSink&) = delete;
            ColorConsoleSink& operator=(const ColorConsoleSink&) = delete;
            ColorConsoleSink(ColorConsoleSink&&) = delete;
            ColorConsoleSink& operator=(ColorConsoleSink&&) = delete;

        public:
            ColorConsoleSink() = default;

            static void consume(boost::log::record_view const& rec, string_type const& formatted_string)
            {
                auto getColor = [](BoostLogLevel level)
                {
                    // default is white
                    WORD res = 7;

                    switch (level) {
                        case BoostLogLevel::Log_DX:
                            res = 11;
                            break;
                        case BoostLogLevel::Log_Warning:
                            res = 10;  // green
                            break;
                        case BoostLogLevel::Log_Error:
                            res = 12;  // red
                            break;
                    }

                    return res;
                };

                auto level = rec[severity];
                auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(hstdout, &csbi);

                SetConsoleTextAttribute(hstdout, getColor(level.get()));
                std::cout << formatted_string << std::endl;
                SetConsoleTextAttribute(hstdout, csbi.wAttributes);
            }
        };

        //////////////////////////////////////////////////////////////////////////

        std::ostream& operator<<(std::ostream& strm, BoostLogLevel level)
        {
            constexpr const char* strings[] = {
                "dx",
                "core",
                "warn",
                "ERRO"
            };

            if (static_cast<std::size_t>(level) < (sizeof(strings) / sizeof(*strings))) {
                // output version of dx
                if (level == Log_DX) {
                    auto& renderer = GetRenderer();

                    if ((renderer->GetType() & ERenderer::RENDERER_DX12) != 0)
                        strm << "dx12";
                    else
                        strm << "dx11";
                } else {
                    strm << strings[level];
                }
            } else {
                strm << static_cast<int>(level);
            }

            return strm;
        }

        void Initialize(const ELogLevel level)
        {
            if (sLogInitizalized)
                return;

            logging::add_common_attributes();

            auto colorSink = boost::make_shared<sinks::synchronous_sink<ColorConsoleSink>>();

            colorSink->set_formatter(expr::format("%1%: [%2%] %3%")
                                     % expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S")
                                     % severity
                                     % expr::message);

            // debug sink (VS output)
            auto debugSink = boost::make_shared <sinks::synchronous_sink<sinks::debug_output_backend>>();

            debugSink->set_filter(expr::is_debugger_present());
            debugSink->set_formatter(expr::format("%1%: [%2%] %3%\n")
                                     % expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S")
                                     % severity
                                     % expr::message);

            auto core = logging::core::get();

            switch (level) {
                case ELogLevel::LL_NONE:
                    colorSink->set_filter(severity > Log_Error);
                    debugSink->set_filter(severity > Log_Error);
                    break;

                case ELogLevel::LL_NORMAL:
                    colorSink->set_filter(severity > Log_DX);
                    debugSink->set_filter(severity > Log_DX);
                    break;

                case ELogLevel::LL_WARN_ERROR:
                    colorSink->set_filter(severity > Log_Core);
                    debugSink->set_filter(severity > Log_Core);
                    break;
            }

            core->add_sink(colorSink);
            core->add_sink(debugSink);

            sLogInitizalized = true;
        }

        void StartIndent()
        {
            ++sLogContext.count;
            sLogContext.isNew = 1;
        }

        void EndIndent()
        {
            --sLogContext.count;
        }

        std::string GetIndent()
        {
            std::string res;

            if (sLogContext.isNew > 0) {
                for (uint32_t i = 1; i < sLogContext.count; ++i)
                    res += " ";

                res += "- ";
                --sLogContext.isNew;
            } else {
                for (uint32_t i = 0; i < sLogContext.count; ++i)
                    res += "  ";
            }

            return res;
        }
    } // namespace Log
} // namespace ninniku