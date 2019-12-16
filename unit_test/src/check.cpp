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

#include "check.h"

#include <boost/test/unit_test.hpp>
#include <openssl/md5.h>
#include <array>
#include <fstream>

unsigned char* GetMD5(uint8_t* data, uint32_t size)
{
    return MD5(data, size, nullptr);
}

void CheckMD5(uint8_t* data, uint32_t size, uint64_t a, uint64_t b)
{
    unsigned char* hash = GetMD5(data, size);

    std::array<uint64_t, 2> wanted = { a, b };

    BOOST_TEST(memcmp(hash, wanted.data(), wanted.size()) == 0);
}

void CheckFileMD5(std::filesystem::path path, uint64_t a, uint64_t b)
{
    std::ifstream ifs(path.c_str(), std::ios::binary | std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    std::vector<uint8_t> result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(reinterpret_cast<char*>(result.data()), pos);

    CheckMD5(result.data(), static_cast<uint32_t>(result.size()), a, b);
}