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

// 20 visually distinct colors
// https://stackoverflow.com/a/20298027/1537817
static const float3 color20[] = {
    float3(0, 1, 0),
    float3(0, 0, 1),
    float3(1, 0, 0),
    float3(0.003, 1, 0.996),
    float3(1, 0.650, 0.996),
    float3(1, 0.858, 0.4),
    float3(0, 0.392, 0.003),
    float3(0.003, 0, 0.403),
    float3(0.584, 0, 0.227),
    float3(0, 0.490, 0.709),
    float3(1, 0, 0.964),
    float3(1, 0.933, 0.909),
    float3(0.466, 0.301, 0),
    float3(0.564, 0.984, 0.572),
    float3(0, 0.462, 1),
    float3(0.835, 1, 0),
    float3(1, 0.576, 0.494),
    float3(0.415, 0.509, 0.423),
    float3(1, 0.007, 0.615),
    float3(0.996, 0.537, 0),
    float3(0.478, 0.278, 0.509)
};