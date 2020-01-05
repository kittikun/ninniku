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

#ifndef CBUFFER_H
#define CBUFFER_H

#ifdef HLSL
#define CBUFFER cbuffer
#else
#include <DirectXMath.h>

#define CBUFFER struct alignas(16)
#define float4x4 DirectX::XMMATRIX
#endif

CBUFFER CBGlobal{
    int targetMip;
};

// While we're here, also defines numthreads here
#define COLORFACES_NUMTHREAD_X 16
#define COLORFACES_NUMTHREAD_Y COLORFACES_NUMTHREAD_X
#define COLORFACES_NUMTHREAD_Z 1

#define COLORMIPS_NUMTHREAD_X 16
#define COLORMIPS_NUMTHREAD_Y COLORMIPS_NUMTHREAD_X
#define COLORMIPS_NUMTHREAD_Z 1

#define DIRTOFACE_NUMTHREAD_X 32
#define DIRTOFACE_NUMTHREAD_Y DIRTOFACE_NUMTHREAD_X
#define DIRTOFACE_NUMTHREAD_Z 1

#define DOWNSAMPLE_NUMTHREAD_X 16
#define DOWNSAMPLE_NUMTHREAD_Y DOWNSAMPLE_NUMTHREAD_X
#define DOWNSAMPLE_NUMTHREAD_Z 1

#define PACKNORMALS_NUMTHREAD_X 32
#define PACKNORMALS_NUMTHREAD_Y PACKNORMALS_NUMTHREAD_X
#define PACKNORMALS_NUMTHREAD_Z 1

#define RESIZE_NUMTHREAD_X 32
#define RESIZE_NUMTHREAD_Y RESIZE_NUMTHREAD_X
#define RESIZE_NUMTHREAD_Z 1

#endif // CBUFFER_H