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

#include <ninniku/ninniku.h>

#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/vector.hpp>
#include <string_view>

struct SetupFixtureNull
{
    SetupFixtureNull();
    ~SetupFixtureNull();
};

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

struct SetupFixtureDX12Slow
{
    SetupFixtureDX12Slow();
    ~SetupFixtureDX12Slow();

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

struct SetupFixtureDX12WarpSlow
{
    SetupFixtureDX12WarpSlow();
    ~SetupFixtureDX12WarpSlow();

    std::string_view shaderRoot;
    bool isNull = false;
};

typedef boost::mpl::vector<SetupFixtureDX11Warp, SetupFixtureDX12WarpSlow, SetupFixtureDX12Warp> FixturesWarpAll;
typedef boost::mpl::vector<SetupFixtureDX11, SetupFixtureDX12Slow, SetupFixtureDX12> FixturesHWAll;
typedef boost::mpl::joint_view<FixturesHWAll, FixturesWarpAll> FixturesAll;

typedef boost::mpl::vector<SetupFixtureDX11> FixtureDX11;
typedef boost::mpl::vector<SetupFixtureDX11Warp> FixtureDX11Warp;

typedef boost::mpl::vector<SetupFixtureDX12Slow, SetupFixtureDX12> FixturesDX12;
typedef boost::mpl::vector<SetupFixtureDX12WarpSlow, SetupFixtureDX12Warp> FixturesDX12Warp;
typedef boost::mpl::joint_view<FixturesDX12, FixturesDX12Warp> FixturesDX12All;

typedef boost::mpl::vector<SetupFixtureDX12Slow> FixtureDX12Slow;
typedef boost::mpl::vector<SetupFixtureDX12> FixtureDX12;
