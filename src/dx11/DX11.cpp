// Copyright(c) 2018-2019 Kitti Vongsay
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

#include "pch.h"
#include "ninniku/dx11/DX11.h"

#pragma comment(lib, "D3DCompiler.lib")

#include "ninniku/image/image.h"
#include "../utils/log.h"
#include "../utils/misc.h"
#include "../utils/VectorSet.h"

#include <comdef.h>
#include <d3dcompiler.h>

namespace ninniku
{
    //////////////////////////////////////////////////////////////////////////
    // DebugMarker
    //////////////////////////////////////////////////////////////////////////
    DX11::DebugMarker::DebugMarker(const DX11Marker& marker, const std::string& name)
        : _marker{ marker }
    {
#ifdef _DEBUG
        _marker->BeginEvent(strToWStr(name).c_str());
#endif
    }

    DX11::DebugMarker::~DebugMarker()
    {
#ifdef _DEBUG
        _marker->EndEvent();
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    // MappedResource
    //////////////////////////////////////////////////////////////////////////
    DX11::MappedResource::MappedResource(const DX11Context& context, const std::unique_ptr<TextureObject>& texObj, uint32_t index)
        : _context{ context }
        , _texObj{ texObj }
        , _index{ index }
        , _mapped{}
    {}

    DX11::MappedResource::~MappedResource()
    {
        _context->Unmap(_texObj->texture.Get(), _index);
    }

    uint32_t DX11::MappedResource::GetRowPitch() const
    {
        return _mapped.RowPitch;
    }

    //////////////////////////////////////////////////////////////////////////
    std::tuple<uint32_t, uint32_t> DX11::CopySubresource(const CopySubresourceParam& params) const
    {
        uint32_t dstSub = D3D11CalcSubresource(params.dstMip, params.dstFace, params.dst->desc.numMips);
        uint32_t srcSub = D3D11CalcSubresource(params.srcMip, params.srcFace, params.src->desc.numMips);

        _context->CopySubresourceRegion(params.dst->texture.Get(), dstSub, 0, 0, 0, params.src->texture.Get(), srcSub, nullptr);

        return std::make_tuple(srcSub, dstSub);
    }

    std::unique_ptr<DX11::DebugMarker> DX11::CreateDebugMarker(const std::string& name) const
    {
        DX11Marker marker;
#ifdef _DEBUG
        _context->QueryInterface(IID_PPV_ARGS(marker.GetAddressOf()));

#endif

        return std::make_unique<DX11::DebugMarker>(marker, name);
    }

    bool DX11::CreateDevice(int adapter, _Outptr_ ID3D11Device** pDevice)
    {
        LOGD << "Creating ID3D11Device..";

        if (!pDevice)
            return false;

        *pDevice = nullptr;

        static PFN_D3D11_CREATE_DEVICE s_DynamicD3D11CreateDevice = nullptr;

        if (!s_DynamicD3D11CreateDevice) {
            HMODULE hModD3D11 = LoadLibrary(L"d3d11.dll");
            if (!hModD3D11)
                return false;

            s_DynamicD3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(hModD3D11, "D3D11CreateDevice")));
            if (!s_DynamicD3D11CreateDevice)
                return false;
        }

        UINT createDeviceFlags = 0;

#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
        D3D_DRIVER_TYPE driverType;

        if (adapter >= 0) {
            Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;

            if (GetDXGIFactory(dxgiFactory.GetAddressOf())) {
                if (FAILED(dxgiFactory->EnumAdapters(adapter, pAdapter.GetAddressOf()))) {
                    auto fmt = boost::format("Invalid GPU adapter index (%1%)!") % adapter;
                    LOGE << boost::str(fmt);
                    return false;
                }
            }

            driverType = (pAdapter) ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
        } else {
            driverType = D3D_DRIVER_TYPE_WARP;
        }

        std::array<D3D_FEATURE_LEVEL, 1> featureLevels = { D3D_FEATURE_LEVEL_11_1 };

        D3D_FEATURE_LEVEL fl;

        auto hr = s_DynamicD3D11CreateDevice(pAdapter.Get(),
                                             driverType,
                                             nullptr, createDeviceFlags, featureLevels.data(), (uint32_t)featureLevels.size(),
                                             D3D11_SDK_VERSION, pDevice, &fl, nullptr);

        if (SUCCEEDED(hr)) {
            Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;

            hr = (*pDevice)->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()));

            if (SUCCEEDED(hr)) {
                hr = dxgiDevice->GetAdapter(pAdapter.ReleaseAndGetAddressOf());

                if (SUCCEEDED(hr)) {
                    DXGI_ADAPTER_DESC desc;

                    hr = pAdapter->GetDesc(&desc);

                    if (SUCCEEDED(hr)) {
                        auto fmt = boost::wformat(L"Using DirectCompute on %1%") % desc.Description;
                        LOGD << boost::str(fmt);
                    }
                }
            }

            return true;
        } else
            return false;
    }

    std::unique_ptr<TextureObject> DX11::CreateTexture(const TextureParam& params)
    {
        auto isSRV = (params.viewflags & TV_SRV) != 0;
        auto isUAV = (params.viewflags & TV_UAV) != 0;
        auto isCPURead = (params.viewflags & TV_CPU_READ) != 0;

        D3D11_TEXTURE2D_DESC desc = {};

        desc.Width = params.width;
        desc.Height = params.height;
        desc.MipLevels = params.numMips;
        desc.ArraySize = params.arraySize;
        desc.Format = static_cast<DXGI_FORMAT>(params.format);
        desc.SampleDesc.Count = 1;

        std::string usageStr;

        if ((params.imageDatas.size() > 0) && (!isUAV)) {
            // this for original data
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            usageStr = "D3D11_USAGE_IMMUTABLE";
        } else if ((params.imageDatas.size() == 0) && (isCPURead)) {
            // only to read back data to the CPU
            desc.Usage = D3D11_USAGE_STAGING;
            usageStr = "D3D11_USAGE_STAGING";
        } else {
            desc.Usage = D3D11_USAGE_DEFAULT;
            usageStr = "D3D11_USAGE_DEFAULT";
        }

        auto fmt = boost::format("Creating Texture2D: Size=%1%x%2%, Mips=%3%, Usage=%4%") % params.width % params.height % params.numMips % usageStr;

        LOGD << boost::str(fmt);

        if (params.arraySize == CUBEMAP_NUM_FACES)
            desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        if (isCPURead)
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        else {
            uint32_t flags = D3D11_BIND_SHADER_RESOURCE;

            if (isUAV)
                flags |= D3D11_BIND_UNORDERED_ACCESS;

            desc.BindFlags = flags;
        }

        auto numImages = params.imageDatas.size();
        std::vector<D3D11_SUBRESOURCE_DATA> initialData(numImages);

        for (auto i = 0; i < numImages; ++i) {
            auto& subParam = params.imageDatas[i];

            initialData[i].pSysMem = subParam.data;
            initialData[i].SysMemPitch = subParam.rowPitch;
            initialData[i].SysMemSlicePitch = subParam.depthPitch;
        }

        auto res = std::make_unique<TextureObject>();

        res->desc = params;

        auto hr = _device->CreateTexture2D(&desc, initialData.data(), res->texture.GetAddressOf());
        if (FAILED(hr)) {
            LOGE << "Failed to CreateTexture2D cube map with:";
            _com_error err(hr);
            LOGE << err.ErrorMessage();
            return std::unique_ptr<TextureObject>();
        }

        if (isSRV) {
            if (params.arraySize == CUBEMAP_NUM_FACES) {
                // To sample texture as cubemap
                D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

                srvDesc.Format = static_cast<DXGI_FORMAT>(params.format);
                srvDesc.TextureCube.MipLevels = params.numMips;
                srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;

                hr = _device->CreateShaderResourceView(res->texture.Get(), &srvDesc, res->srvCube.GetAddressOf());
                if (FAILED(hr)) {
                    LOGE << "Failed to CreateShaderResourceView with D3D_SRV_DIMENSION_TEXTURECUBE with:";
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return std::unique_ptr<TextureObject>();
                }

                // To sample texture as array, one for each miplevel
                res->srvArray.resize(params.numMips);

                for (uint32_t i = 0; i < params.numMips; ++i) {
                    srvDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
                    srvDesc.Format = static_cast<DXGI_FORMAT>(params.format);
                    srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
                    srvDesc.Texture2DArray.ArraySize = CUBEMAP_NUM_FACES;
                    srvDesc.Texture2DArray.MostDetailedMip = i;
                    srvDesc.Texture2DArray.MipLevels = 1;

                    hr = _device->CreateShaderResourceView(res->texture.Get(), &srvDesc, res->srvArray[i].GetAddressOf());
                    if (FAILED(hr)) {
                        auto fmt = boost::format("Failed to CreateShaderResourceView with D3D_SRV_DIMENSION_TEXTURE2DARRAY for mip %1% with:") % i;
                        LOGE << boost::str(fmt);
                        _com_error err(hr);
                        LOGE << err.ErrorMessage();

                        return std::unique_ptr<TextureObject>();
                    }
                }

                // one for array with all mips
                srvDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
                srvDesc.Format = static_cast<DXGI_FORMAT>(params.format);
                srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.ArraySize = CUBEMAP_NUM_FACES;
                srvDesc.Texture2DArray.MostDetailedMip = 0;
                srvDesc.Texture2DArray.MipLevels = params.numMips;

                hr = _device->CreateShaderResourceView(res->texture.Get(), &srvDesc, res->srvArrayWithMips.GetAddressOf());
                if (FAILED(hr)) {
                    auto fmt = boost::format("Failed to CreateShaderResourceView with D3D_SRV_DIMENSION_TEXTURE2DARRAY with all mips with:");
                    LOGE << boost::str(fmt);
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return std::unique_ptr<TextureObject>();
                }
            } else {
                D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

                srvDesc.Format = static_cast<DXGI_FORMAT>(params.format);

                if (params.arraySize > 1) {
                    // one for each miplevel
                    res->srvArray.resize(params.numMips);

                    for (uint32_t i = 0; i < params.numMips; ++i) {
                        if (params.height == 1) {
                            srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1DARRAY;
                            srvDesc.Texture1DArray.ArraySize = params.arraySize;
                            srvDesc.Texture1DArray.MostDetailedMip = i;
                            srvDesc.Texture1DArray.MipLevels = 1;
                        } else {
                            srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
                            srvDesc.Texture2DArray.ArraySize = params.arraySize;
                            srvDesc.Texture2DArray.MostDetailedMip = i;
                            srvDesc.Texture2DArray.MipLevels = 1;
                        }

                        hr = _device->CreateShaderResourceView(res->texture.Get(), &srvDesc, res->srvArray[i].GetAddressOf());
                        if (FAILED(hr)) {
                            auto fmt = boost::format("Failed to CreateShaderResourceView with D3D_SRV_DIMENSION for Mip=%1% with Height=%2% with:") % i % params.height;
                            LOGE << boost::str(fmt);
                            _com_error err(hr);
                            LOGE << err.ErrorMessage();
                            return std::unique_ptr<TextureObject>();
                        }
                    }
                } else {
                    if (params.depth > 1) {
                        srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE3D;
                        srvDesc.Texture3D.MipLevels = params.numMips;
                    } else if (params.height == 1) {
                        srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1D;
                        srvDesc.Texture1D.MipLevels = params.numMips;
                    } else {
                        srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
                        srvDesc.Texture2D.MipLevels = params.numMips;
                    }

                    hr = _device->CreateShaderResourceView(res->texture.Get(), &srvDesc, res->srvDefault.GetAddressOf());
                    if (FAILED(hr)) {
                        auto fmt = boost::format("Failed to CreateShaderResourceView with: Height=%1%, Depth=%2% with:") % params.height % params.depth;
                        LOGE << boost::str(fmt);
                        _com_error err(hr);
                        LOGE << err.ErrorMessage();
                        return std::unique_ptr<TextureObject>();
                    }
                }
            }
        }

        if (isUAV) {
            res->uav.resize(params.numMips);

            // we have to create an UAV for each miplevel
            for (uint32_t i = 0; i < params.numMips; ++i) {
                D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = static_cast<DXGI_FORMAT>(params.format);

                if (params.arraySize > 1) {
                    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = i;
                    uavDesc.Texture2DArray.ArraySize = CUBEMAP_NUM_FACES;
                } else {
                    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                    uavDesc.Texture2D.MipSlice = i;
                }

                hr = _device->CreateUnorderedAccessView(res->texture.Get(), &uavDesc, res->uav[i].GetAddressOf());
                if (FAILED(hr)) {
                    LOGE << "Failed to CreateUnorderedAccessView with:";
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return std::unique_ptr<TextureObject>();
                }
            }
        }

        return res;
    }

    bool DX11::Dispatch(const Command& cmd) const
    {
        auto found = _shaders.find(cmd.shader);

        if (found == _shaders.end()) {
            auto fmt = boost::format("Dispatch error: could not find shader \"%1%\"") % cmd.shader;
            LOGE << boost::str(fmt);
            return false;
        }

        // unbind previously set UAV (assuming there is only ONE was ever used for now)
        std::array<ID3D11UnorderedAccessView*, 1> nullUAV{ nullptr };

        _context->CSSetUnorderedAccessViews(0, 1, nullUAV.data(), nullptr);

        auto& cs = found->second;

        // static containers so they don't get resized all the the time
        static VectorSet<uint32_t, ID3D11ShaderResourceView*> vmSRV;
        static VectorSet<uint32_t, ID3D11UnorderedAccessView*> vmUAV;
        static VectorSet<uint32_t, ID3D11SamplerState*> vmSS;

        auto lambda = [&](auto kvp, auto & container)
        {
            auto f = cs.bindSlots.find(kvp.first);

            if (f != cs.bindSlots.end()) {
                container.insert(f->second, kvp.second.Get());
            } else {
                auto fmt = boost::format("Dispatch error: could not find bindSlots \"%1%\"") % kvp.first;
                LOGE << boost::str(fmt);
                return false;
            }

            return true;
        };

        // SRV
        {
            vmSRV.beginInsert();

            for (auto kvp : cmd.srvBindings) {
                if (!lambda(kvp, vmSRV))
                    return false;
            }

            vmSRV.endInsert();
        }

        // UAV
        {
            vmUAV.beginInsert();

            for (auto kvp : cmd.uavBindings) {
                if (!lambda(kvp, vmUAV))
                    return false;
            }

            vmUAV.endInsert();
        }

        // Sample states
        {
            vmSS.beginInsert();

            for (auto kvp : cmd.ssBindings) {
                if (!lambda(kvp, vmSS))
                    return false;
            }

            vmSS.endInsert();
        }

        // Set shader data
        _context->CSSetShader(cs.shader.Get(), nullptr, 0);

        // Constant buffers
        auto foundCB = _cBuffers.find(cmd.cbufferStr);

        if (foundCB != _cBuffers.end()) {
            std::array<ID3D11Buffer*, 1> cbs{ foundCB->second.Get() };

            _context->CSSetConstantBuffers(0, 1, cbs.data());
        }

        _context->CSSetSamplers(0, (uint32_t)vmSS.size(), vmSS.data());
        _context->CSSetUnorderedAccessViews(0, (uint32_t)vmUAV.size(), vmUAV.data(), nullptr);
        _context->CSSetShaderResources(0, vmSRV.size(), vmSRV.data());
        _context->Dispatch(cmd.dispatch[0], cmd.dispatch[1], cmd.dispatch[2]);
        //_context->Flush();
        return true;
    }

    bool DX11::GetDXGIFactory(IDXGIFactory1** pFactory)
    {
        if (!pFactory)
            return false;

        *pFactory = nullptr;

        typedef HRESULT(WINAPI * pfn_CreateDXGIFactory1)(REFIID riid, _Out_ void** ppFactory);

        static pfn_CreateDXGIFactory1 s_CreateDXGIFactory1 = nullptr;

        if (!s_CreateDXGIFactory1) {
            auto hModDXGI = LoadLibrary(L"dxgi.dll");
            if (!hModDXGI)
                return false;

            s_CreateDXGIFactory1 = reinterpret_cast<pfn_CreateDXGIFactory1>(reinterpret_cast<void*>(GetProcAddress(hModDXGI, "CreateDXGIFactory1")));

            if (!s_CreateDXGIFactory1)
                return false;
        }

        return SUCCEEDED(s_CreateDXGIFactory1(IID_PPV_ARGS(pFactory)));
    }

    bool DX11::Initialize(const std::string& shaderPath, bool isWarp)
    {
        auto adapter = 0;

        if (isWarp)
            adapter = -1;

        if (!CreateDevice(adapter, _device.GetAddressOf())) {
            LOGE << "Failed to create DX11 device";
            return false;
        }

        // get context
        _device->GetImmediateContext(_context.ReleaseAndGetAddressOf());
        if (_context == nullptr) {
            LOGE << "Failed to obtain immediate context";
            return false;
        }

        if (!LoadShaders(shaderPath)) {
            LOGE << "Failed to load shaders.";
            return false;
        }

        // sampler states
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

        auto hr = _device->CreateSamplerState(&samplerDesc, _samplers[SS_Point].GetAddressOf());
        if (FAILED(hr)) {
            LOGE << "Failed to CreateSamplerState for nearest with:";
            _com_error err(hr);
            LOGE << err.ErrorMessage();
            return false;
        }

        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

        hr = _device->CreateSamplerState(&samplerDesc, _samplers[SS_Linear].GetAddressOf());
        if (FAILED(hr)) {
            LOGE << "Failed to CreateSamplerState for linear with:";
            _com_error err(hr);
            LOGE << err.ErrorMessage();
            return false;
        }

        return true;
    }

    std::unique_ptr<DX11::MappedResource> DX11::MapTexture(const std::unique_ptr<TextureObject>& tObj, uint32_t index)
    {
        auto res = std::make_unique<MappedResource>(_context, tObj, index);

        auto hr = _context->Map(tObj->texture.Get(), 0, D3D11_MAP_READ, 0, res->Get());

        if (FAILED(hr)) {
            _com_error err(hr);
            LOGE << "Failed to map texture with:";
            LOGE << err.ErrorMessage();
            return std::unique_ptr<MappedResource>();
        }

        return res;
    }

    /// <summary>
    /// Load all shaders in /data
    /// </summary>
    bool DX11::LoadShaders(const std::string& shaderPath)
    {
        std::string ext{ ".cso" };

        // Count the number of .cso found
        boost::filesystem::directory_iterator begin(shaderPath), end;

        auto fileCounter = [&](const boost::filesystem::directory_entry & d)
        {
            return (!is_directory(d.path()) && (d.path().extension() == ext));
        };

        auto numFiles = std::count_if(begin, end, fileCounter);
        auto fmt = boost::format("Found %1% compiled shaders in /%2%") % numFiles % shaderPath;

        LOGD << boost::str(fmt);

        for (auto& iter : boost::filesystem::recursive_directory_iterator(shaderPath)) {
            if (iter.path().extension() == ext) {
                auto path = iter.path().string();

                fmt = boost::format("Loading %1%..") % path;

                LOG_INDENT_START << boost::str(fmt);

                ID3DBlob* blob = nullptr;
                auto hr = D3DReadFileToBlob(ninniku::strToWStr(path).c_str(), &blob);

                if (FAILED(hr)) {
                    fmt = boost::format("Failed to D3DReadFileToBlob: %1% with:") % path;
                    LOGE << boost::str(fmt);
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return false;
                }

                // reflection
                ID3D11ShaderReflection* reflect = nullptr;
                D3D11_SHADER_DESC desc;

                hr = D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&reflect));
                if (FAILED(hr)) {
                    fmt = boost::format("Failed to call D3DReflect on shader: %1% with:") % path;
                    LOGE << boost::str(fmt);
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return false;
                }

                hr = reflect->GetDesc(&desc);
                if (FAILED(hr)) {
                    fmt = boost::format("Failed to GetDesc from shader reflection on shader: %1% with:") % path;
                    LOGE << boost::str(fmt);
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return false;
                }

                auto bindings = ParseShaderResources(desc, reflect);

                // create shader
                DX11CS shader;

                hr = _device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, shader.GetAddressOf());

                if (FAILED(hr)) {
                    fmt = boost::format("Failed to CreateComputeShader: %1% with:") % path;
                    LOGE << boost::str(fmt);
                    _com_error err(hr);
                    LOGE << err.ErrorMessage();
                    return false;
                } else {
                    auto name = iter.path().stem().string();

                    fmt = boost::format("Adding CS: \"%1%\" to library") % name;
                    LOGD << boost::str(fmt);

                    _shaders.insert(std::make_pair(name, ComputeShader{ shader, bindings }));
                }

                LOG_INDENT_END;
            }
        }

        return true;
    }

    std::unordered_map<std::string, uint32_t> DX11::ParseShaderResources(const D3D11_SHADER_DESC& desc, ID3D11ShaderReflection* reflection)
    {
        std::unordered_map<std::string, uint32_t> resMap;

        // parse parameter bind slots
        for (uint32_t i = 0; i < desc.BoundResources; ++i) {
            D3D11_SHADER_INPUT_BIND_DESC res;

            auto hr = reflection->GetResourceBindingDesc(i, &res);
            std::string restypeStr = "UNKNOWN";

            switch (res.Type) {
                case D3D_SIT_TEXTURE:
                    restypeStr = "D3D_SIT_TEXTURE";
                    break;

                case D3D_SIT_SAMPLER:
                    restypeStr = "D3D_SIT_SAMPLER";
                    break;

                case D3D_SIT_UAV_RWTYPED:
                    restypeStr = "D3D_SIT_UAV_RWTYPED";
                    break;

                case D3D_SIT_CBUFFER:
                    restypeStr = "D3D10_SIT_CBUFFER";
                    break;
            }

            auto fmt = boost::format("Resource: Name=\"%1%\", Type=%2%, Slot=%3%") % res.Name % restypeStr % res.BindPoint;

            LOGD << boost::str(fmt);

            // if the texture is a constant buffer, we want to create it a slot in the map
            if (res.Type == D3D_SIT_CBUFFER) {
                _cBuffers.insert(std::make_pair(res.Name, DX11Buffer{}));
            }

            resMap.insert(std::make_pair(res.Name, res.BindPoint));
        }

        return resMap;
    }

    bool DX11::UpdateConstantBuffer(const std::string& name, void* data, uint32_t size)
    {
        auto found = _cBuffers.find(name);

        if (found == _cBuffers.end()) {
            auto fmt = boost::format("Constant buffer \"%1%\" was not found in any of the shaders parsed") % name;
            LOGE << boost::str(fmt);

            return false;
        }

        if (found->second == nullptr) {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = size;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            D3D11_SUBRESOURCE_DATA initialData = {};
            initialData.pSysMem = data;

            auto hr = _device->CreateBuffer(&desc, &initialData, &_cBuffers[name]);
            if (FAILED(hr)) {
                auto fmt = boost::format("Failed to CreateBuffer constant buffer \"%1%\" with:") % name;
                LOGE << boost::str(fmt);
                _com_error err(hr);
                LOGE << err.ErrorMessage();
                return false;
            }
        } else {
            // constant buffer already exists so just update it

            D3D11_MAPPED_SUBRESOURCE mapped = {};
            auto src = _cBuffers[name].Get();
            auto hr = _context->Map(src, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

            if (FAILED(hr)) {
                auto fmt = boost::format("Failed to map constant buffer \"%1%\" with:") % name;
                LOGE << boost::str(fmt);
                _com_error err(hr);
                LOGE << err.ErrorMessage();
                return false;
            }

            memcpy_s(mapped.pData, size, data, size);
            _context->Unmap(src, 0);
        }

        return true;
    }
} // namespace ninniku