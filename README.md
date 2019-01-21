# Ninniku [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/kittikun/takoyaki/blob/master/LICENSE)

```ninniku (にんにく) is the Japanese word for garlic```

[![Build status](https://ci.appveyor.com/api/projects/status/9wne2qsbsihhxnxd/branch/master?svg=true)](https://ci.appveyor.com/project/kittikun/ninniku/branch/master)

Simple framework to run compute jobs with DX11
- Texture are supported with the SRV/UAV views
- Can load cubemaps with [cmft](https://github.com/dariomanesku/cmft).
  * Saved dds files can be DXGI_FORMAT_R32G32B32A32_FLOAT or DXGI_FORMAT_R8G8B8A8_UNORM
- Can load/save DDS with [DirectXTex](https://github.com/Microsoft/DirectXTex)
  * Saved dds files must be compressed BCx format (but BC2 is not supported)
- Can load PNG files with [stb](https://github.com/nothings/stb)
  * Can be 1-4 channel, 8 or 16 bits
- Can load EXR files with [tinyexr](https://github.com/syoyo/tinyexr)
- Can capture commands with [RenderDoc](https://renderdoc.org/).
  * Compile using Debug configuration or define _USE_RENDERDOC
  * Captures will be called ninniku_frame0.rdc

#### Usage:
Look at project simple or there is plenty of samples provided as unit tests
