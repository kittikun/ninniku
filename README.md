# Ninniku [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/kittikun/takoyaki/blob/master/LICENSE)

```ninniku (にんにく) is the Japanese word for garlic```

[![Build status](https://ci.appveyor.com/api/projects/status/9wne2qsbsihhxnxd/branch/develop?svg=true)](https://ci.appveyor.com/project/kittikun/ninniku/branch/develop)

Simple framework to run compute jobs with DX11
- Texture are supported with the SRV/UAV views
- Can load cubemaps with [cmft](https://github.com/dariomanesku/cmft).
  * Saved dds files will always be DXGI_FORMAT_R32G32B32A32_FLOAT
- Can load/save DDS with [DirectXTex](https://github.com/Microsoft/DirectXTex)
  * Saved dds files must be compressed BCx format
- Can load PNG files with [stb](https://github.com/nothings/stb)
- Can capture commands with [RenderDoc](https://renderdoc.org/).
  * Compile using Debug configuration or define _USE_RENDERDOC
  * Captures will be called ninniku_frame0.rdc

#### Usage:
Shader must be compiled for SM 5.0 and define the "HLSL" preprocessor

Plenty of samples are provided in the form of unit tests