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

#include <boost/mpl/vector.hpp>
#include <string_view>

struct SetupFixtureDX11
{
    SetupFixtureDX11();
    ~SetupFixtureDX11();

    std::string_view shaderRoot;
    bool isNull = false;
};

struct SetupFixtureDX11Warp
{
    SetupFixtureDX11Warp();
    ~SetupFixtureDX11Warp();

    std::string_view shaderRoot;
    bool isNull = false;
};

struct SetupFixtureDX12
{
    SetupFixtureDX12();
    ~SetupFixtureDX12();

    std::string_view shaderRoot;
    bool isNull = false;
};

struct SetupFixtureDX12Warp
{
    SetupFixtureDX12Warp();
    ~SetupFixtureDX12Warp();

    std::string_view shaderRoot;
    bool isNull = false;
};

typedef boost::mpl::vector<SetupFixtureDX11, SetupFixtureDX12, SetupFixtureDX11Warp, SetupFixtureDX12Warp> FixturesAll;
typedef boost::mpl::vector<SetupFixtureDX11Warp, SetupFixtureDX12Warp> FixturesWarpAll;
typedef boost::mpl::vector<SetupFixtureDX11, SetupFixtureDX12> FixturesHWAll;
typedef boost::mpl::vector<SetupFixtureDX11> FixturesDX11;
typedef boost::mpl::vector<SetupFixtureDX12> FixturesDX12;
