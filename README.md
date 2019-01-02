# Ninniku [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/kittikun/takoyaki/blob/master/LICENSE)

```ninniku (にんにく) is the Japanese word for garlic```

[![Build status](https://ci.appveyor.com/api/projects/status/9wne2qsbsihhxnxd/branch/develop?svg=true)](https://ci.appveyor.com/project/kittikun/ninniku/branch/develop)

Simple framework to run compute jobs with DX11
- Can load cubemaps with [cmft](https://github.com/dariomanesku/cmft).
- Can load DDS with [DirectXTex](https://github.com/Microsoft/DirectXTex)
- Can capture commands with [RenderDoc](https://renderdoc.org/).
  * Compile using Debug configuration
  * Captures will be called ninniku_frame0.rdc

#### Usage:
Shader must be compiled for SM 5.0 and define the "HLSL" preprocessor