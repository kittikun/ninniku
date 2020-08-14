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
    template<typename KeyType, typename ValueType>
    class VectorSet
    {
        // no copy of any kind allowed
        VectorSet(const VectorSet&) = delete;
        VectorSet& operator=(const VectorSet&) = delete;
        VectorSet(VectorSet&&) = delete;
        VectorSet& operator=(VectorSet&&) = delete;

    public:
        using KVP = std::tuple<KeyType, ValueType>;

        VectorSet() = default;

        void beginInsert() noexcept
        {
            keyValues.clear();
        }

        const ValueType* data() const noexcept { return list_.data(); }

        void endInsert()
        {
            if (keyValues.size() > 1) {
                auto lamda = [](const KVP& lhs, const KVP& rhs)
                {
                    return std::get<0>(lhs) < std::get<0>(rhs);
                };

                std::sort(keyValues.begin(), keyValues.end(), lamda);
            }

            // copy sorted item to list
            auto size = keyValues.size();

            list_.clear();
            list_.resize(size);

            for (auto i = 0u; i < size; ++i) {
                list_[i] = std::get<1>(keyValues[i]);
            }
        }

        void insert(const KeyType& key, const ValueType& value)
        {
            keyValues.push_back({ key, value });
        }

        uint32_t size() const noexcept
        {
            return static_cast<uint32_t>(list_.size());
        }

    private:
        std::vector<std::tuple<KeyType, ValueType>> keyValues;
        std::vector<ValueType> list_;
    };
} // namespace ninniku
