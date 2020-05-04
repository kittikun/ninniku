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

#include "ninniku/core/renderer/types.h"

namespace ninniku {
    //////////////////////////////////////////////////////////////////////////
    // Track objects externally so we can free them when Terminate() is called
    //////////////////////////////////////////////////////////////////////////
    struct TrackedObject
    {
        virtual ~TrackedObject() = default;
    };

    //////////////////////////////////////////////////////////////////////////
    // ObjectTracker
    //////////////////////////////////////////////////////////////////////////
    class ObjectTracker : NonCopyable
    {
    public:
        ObjectTracker();

        static ObjectTracker& Instance() { return _instance; }

        void RegisterObject(const std::shared_ptr<TrackedObject>& obj);
        void ReleaseObjects();

    private:
        std::vector<std::shared_ptr<TrackedObject>> _objects;
        static ObjectTracker _instance;
    };
} // namespace ninniku
