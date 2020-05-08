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

namespace ninniku
{
    template<typename ValueType>
    class StringMap : public std::unordered_map<std::string, ValueType>
    {
    public:

        typename std::unordered_map<std::string, ValueType>::iterator find(const std::string_view& str)
        {
            _tmp.reserve(str.size());
            _tmp.assign(str.data(), str.size());
            return std::unordered_map<std::string, ValueType>::find(_tmp);
        }

        typename std::unordered_map<std::string, ValueType>::const_iterator find(const std::string_view& str) const
        {
            _tmp.reserve(str.size());
            _tmp.assign(str.data(), str.size());
            return std::unordered_map<std::string, ValueType>::find(_tmp);
        }

        ValueType& operator[](const std::string_view& str)
        {
            _tmp.reserve(str.size());
            _tmp.assign(str.data(), str.size());

            return std::unordered_map<std::string, ValueType>::operator [](_tmp);
        }

        const ValueType& operator[](const std::string_view& str) const
        {
            _tmp.reserve(str.size());
            _tmp.assign(str.data(), str.size());

            return std::unordered_map<std::string, ValueType>::operator [](_tmp);
        }

        void emplace(const std::string_view& str, const ValueType& value)
        {
            // discard return value since it is not needed
            _tmp.reserve(str.size());
            _tmp.assign(str.data(), str.size());

            std::unordered_map<std::string, ValueType>::emplace(_tmp, value);
        }

        void emplace(const std::string_view& str, ValueType&& value)
        {
            // discard return value since it is not needed
            _tmp.reserve(str.size());
            _tmp.assign(str.data(), str.size());

            std::unordered_map<std::string, ValueType>::emplace(_tmp, std::move(value));
        }

    private:
        thread_local static std::string _tmp;
    };

    template<typename ValueType>
    thread_local std::string StringMap<ValueType>::_tmp;
} // namespace ninniku
