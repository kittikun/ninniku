# Ninniku [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/kittikun/takoyaki/blob/master/LICENSE)

```ninniku (にんにく) is the Japanese word for garlic```

### Simple framework to run compute jobs with DX11/DX12
&nbsp;
#### Build status
###### Master: &nbsp;[![Build status](https://ci.appveyor.com/api/projects/status/9wne2qsbsihhxnxd/branch/master?svg=true)](https://ci.appveyor.com/project/kittikun/ninniku/branch/master)
###### Develop: &nbsp;[![Build status](https://ci.appveyor.com/api/projects/status/9wne2qsbsihhxnxd/branch/develop?svg=true)](https://ci.appveyor.com/project/kittikun/ninniku/branch/develop)
&nbsp;
#### Features
- Can load/save cubemaps with [cmft](https://github.com/dariomanesku/cmft)
- Can load/save DDS with [DirectXTex](https://github.com/Microsoft/DirectXTex)
- Can load BMP, GIF, HDR, JPG, PNG, PIC, PNM, PSD, TGA files with [stb](https://github.com/nothings/stb)
  * Can be 1-4 channel, 8 or 16 bits
- Can load EXR files with [tinyexr](https://github.com/syoyo/tinyexr)
- Can capture commands with [RenderDoc](https://renderdoc.org/) for DX11 and [PIX](https://devblogs.microsoft.com/pix/) for DX12
  * Compile using Debug configuration or define _DO_CAPTURE
  * Captures will be called ninniku_frame0.rdc
- DX12 shaders are compiled using the [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler)

#### Usage:
Look at project simple or there is plenty of samples provided as unit tests
