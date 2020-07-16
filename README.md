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
  * Renderdoc captures will be called ninniku_frame0.rdc
- DX12 shaders are compiled using the [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler)

#### Prerequisites:
- You must install the [Windows 10 SDK (10.0.19041.0)](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/)

#### Usage:
Look at project simple or there is plenty of samples provided as unit tests

#### Compiling Shaders:
- **[DX11]** Shaders must be compiled with [FXC](https://docs.microsoft.com/en-us/windows/win32/direct3dtools/fxc) as .cso and is straight forward using Visual Studio
- **[DX12]** Shaders must use [DXC](https://github.com/microsoft/DirectXShaderCompiler) as .dxco
- **[DX12]** [RootSignatures must be defined within the shader](https://docs.microsoft.com/en-us/windows/win32/direct3d12/specifying-root-signatures-in-hlsl)
- **[DX12]** At the moment, you must define one root signature per main.
- **[DX12]** WARP only support up to SM 6.2 so unit tests are restricted to 6.2 but feel free to use a higher version
- **[DX12]** Shader compilation can be set as a **Custom Build Tool** using the following syntax:

###### DXC Debug:
> "$(WindowsSdkDir)bin\$(TargetPlatformVersion)\$(Platform)\dxc.exe" -T cs_6_2 -Od -Zi -Qembed_debug -D HLSL -rootsig-define RS %(FullPath) -Fo $(ProjectDir)bin\$(Configuration)\dx12\%(Filename).dxco

###### DXC Release:
> "$(WindowsSdkDir)bin\$(TargetPlatformVersion)\$(Platform)\dxc.exe" -T cs_6_2 -D HLSL -rootsig-define RS %(FullPath) -Fo $(ProjectDir)bin\$(Configuration)\dx12\%(Filename).dxco

#### Rules:
- **[DX12]** Samplers must be declared last in the HLSL root signature definition.
