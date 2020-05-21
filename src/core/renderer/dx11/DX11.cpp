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

#include "pch.h"
#include "DX11.h"

#pragma comment(lib, "D3DCompiler.lib")

#include "../../../globals.h"
#include "../../../utils/log.h"
#include "../../../utils/misc.h"
#include "../../../utils/objectTracker.h"
#include "../../../utils/VectorSet.h"
#include "../DXCommon.h"

#include <comdef.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>

namespace ninniku
{
    DX11::DX11(ERenderer type)
        : _type{ type }
    {
    }

    void DX11::CopyBufferResource(const CopyBufferSubresourceParam& params)
    {
        auto srcImpl = static_cast<const DX11BufferImpl*>(params.src);
        auto srcInternal = srcImpl->_impl.lock();

        auto dstImpl = static_cast<const DX11BufferImpl*>(params.dst);
        auto dstInternal = dstImpl->_impl.lock();

        _context->CopyResource(dstInternal->_buffer.Get(), srcInternal->_buffer.Get());
    }

    std::tuple<uint32_t, uint32_t> DX11::CopyTextureSubresource(const CopyTextureSubresourceParam& params)
    {
        auto srcImpl = static_cast<const DX11TextureImpl*>(params.src);
        auto srcInternal = srcImpl->_impl.lock();

        auto dstImpl = static_cast<const DX11TextureImpl*>(params.dst);
        auto dstInternal = dstImpl->_impl.lock();

        uint32_t dstSub = D3D11CalcSubresource(params.dstMip, params.dstFace, dstImpl->GetDesc()->numMips);
        uint32_t srcSub = D3D11CalcSubresource(params.srcMip, params.srcFace, srcImpl->GetDesc()->numMips);

        _context->CopySubresourceRegion(dstInternal->GetResource(), dstSub, 0, 0, 0, srcInternal->GetResource(), srcSub, nullptr);

        return std::make_tuple(srcSub, dstSub);
    }

    DebugMarkerHandle DX11::CreateDebugMarker(const std::string_view& name) const
    {
        DX11Marker marker;
#ifdef _DO_CAPTURE
        _context->QueryInterface(IID_PPV_ARGS(marker.GetAddressOf()));
#endif

        return std::make_unique<DX11DebugMarker>(marker, name);
    }

    BufferHandle DX11::CreateBuffer(const BufferParamHandle& params)
    {
        auto isSRV = (params->viewflags & EResourceViews::RV_SRV) != 0;
        auto isUAV = (params->viewflags & EResourceViews::RV_UAV) != 0;
        auto isCPURead = (params->viewflags & EResourceViews::RV_CPU_READ) != 0;

        // let's only support default for now
        D3D11_USAGE usage = isCPURead ? D3D11_USAGE_STAGING : D3D11_USAGE_DEFAULT;
        std::string_view usageStr = isCPURead ? "D3D11_USAGE_STAGING" : "D3D11_USAGE_DEFAULT";

        auto fmt = boost::format("Creating Buffer: ElementSize=%1%, NumElements=%2%, Usage=%3%") % params->elementSize % params->numElements % usageStr;

        LOGD << boost::str(fmt);

        uint32_t cpuFlags = 0;
        uint32_t bindFlags = 0;

        if (isCPURead)
            cpuFlags = D3D11_CPU_ACCESS_READ;

        if (isSRV)
            bindFlags |= D3D11_BIND_SHADER_RESOURCE;

        if (isUAV)
            bindFlags |= D3D11_BIND_UNORDERED_ACCESS;

        // Do not support ByteAddressBuffer for now, default to StructuredBuffer
        uint32_t miscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

        D3D11_BUFFER_DESC desc = {};

        // we need to pad to 4 bytes because Buffer data is an array of uint32_t
        if (params->elementSize % 4 != 0) {
            LOGE << "ElementSize must be a multiple of 4";
            return BufferHandle();
        }

        desc.ByteWidth = params->numElements * params->elementSize;
        desc.Usage = usage;
        desc.BindFlags = bindFlags;
        desc.MiscFlags = miscFlags;
        desc.CPUAccessFlags = cpuFlags;
        desc.StructureByteStride = params->elementSize;

        auto impl = std::make_shared<DX11BufferInternal>();

        _tracker.RegisterObject(impl);

        impl->_desc = params;

        // do not support initial data for now
        auto hr = _device->CreateBuffer(&desc, nullptr, impl->_buffer.GetAddressOf());

        if (CheckAPIFailed(hr, "ID3D11Device::CreateBuffer"))
            return BufferHandle();

        if (isSRV) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.NumElements = params->numElements;

            auto srv = new DX11ShaderResourceView();

            hr = _device->CreateShaderResourceView(impl->_buffer.Get(), &srvDesc, srv->_resource.GetAddressOf());

            if (CheckAPIFailed(hr, "ID3D11Device::CreateShaderResourceView with D3D11_SRV_DIMENSION_BUFFER")) {
                return BufferHandle();
            } else {
                impl->_srv.reset(srv);
            }
        }

        if (isUAV) {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.NumElements = params->numElements;

            auto uav = new DX11UnorderedAccessView();

            hr = _device->CreateUnorderedAccessView(impl->_buffer.Get(), &uavDesc, uav->_resource.GetAddressOf());

            if (CheckAPIFailed(hr, "ID3D11Device::CreateUnorderedAccessView with D3D11_SRV_DIMENSION_BUFFER")) {
                return BufferHandle();
            } else {
                impl->_uav.reset(uav);
            }
        }

        return std::make_unique<DX11BufferImpl>(impl);
    }

    BufferHandle DX11::CreateBuffer(const BufferHandle& src)
    {
        auto implSrc = static_cast<const DX11BufferImpl*>(src.get());
        auto internalSrc = implSrc->_impl.lock();

        assert(internalSrc->_desc->elementSize % 4 == 0);

        auto marker = CreateDebugMarker("CreateBufferFromBufferObject");

        auto dst = CreateBuffer(internalSrc->_desc);
        auto implDst = static_cast<const DX11BufferImpl*>(dst.get());
        auto internalDst = implDst->_impl.lock();

        // copy src to dst
        {
            CopyBufferSubresourceParam copyParams = {};

            copyParams.src = src.get();
            copyParams.dst = dst.get();

            CopyBufferResource(copyParams);
        }

        // create a temporary object readable from CPU to fill internalDst->_data with a map
        auto stride = internalSrc->_desc->elementSize / 4;
        auto params = internalSrc->_desc->Duplicate();

        // allocate memory
        internalDst->_data.resize(stride * internalSrc->_desc->numElements);
        params->viewflags = RV_CPU_READ;

        auto temp = CreateBuffer(params);

        // copy src to temp
        {
            CopyBufferSubresourceParam copyParams = {};

            copyParams.src = src.get();
            copyParams.dst = temp.get();

            CopyBufferResource(copyParams);
        }

        auto mapped = Map(temp);
        auto dxMapped = static_cast<const DX11MappedResource*>(mapped.get());
        uint32_t dstPitch = static_cast<uint32_t>(internalDst->_data.size() * sizeof(uint32_t));

        memcpy_s(&internalDst->_data.front(), dstPitch, mapped->GetData(), std::min(dstPitch, dxMapped->GetRowPitch()));

        return dst;
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

        Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
        D3D_DRIVER_TYPE driverType;

        if (adapter >= 0) {
            Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;

            if (DXCommon::GetDXGIFactory<IDXGIFactory1>(dxgiFactory.GetAddressOf())) {
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

        UINT createDeviceFlags = 0;

        if (Globals::Instance()._useDebugLayer)
            createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

        std::array<D3D_FEATURE_LEVEL, 1> featureLevels = { D3D_FEATURE_LEVEL_11_1 };

        D3D_FEATURE_LEVEL fl;

        auto hr = s_DynamicD3D11CreateDevice(pAdapter.Get(),
                                             driverType,
                                             nullptr, createDeviceFlags, featureLevels.data(), static_cast<uint32_t>(featureLevels.size()),
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
        }

        return false;
    }

    TextureHandle DX11::CreateTexture(const TextureParamHandle& params)
    {
        auto isSRV = (params->viewflags & EResourceViews::RV_SRV) != 0;
        auto isUAV = (params->viewflags & EResourceViews::RV_UAV) != 0;
        auto isCPURead = (params->viewflags & EResourceViews::RV_CPU_READ) != 0;
        auto is3d = params->depth > 1;
        auto is1d = params->height == 1;
        auto is2d = (!is3d) && (!is1d);
        auto isCube = is2d && (params->arraySize == CUBEMAP_NUM_FACES);
        auto isCubeArray = is2d && (params->arraySize > CUBEMAP_NUM_FACES) && ((params->arraySize % CUBEMAP_NUM_FACES) == 0);

        D3D11_USAGE usage;
        std::string_view usageStr;

        if ((params->imageDatas.size() > 0) && (!isUAV)) {
            // this for original data
            usage = D3D11_USAGE_IMMUTABLE;
            usageStr = "D3D11_USAGE_IMMUTABLE";
        } else if ((params->imageDatas.size() == 0) && (isCPURead)) {
            // only to read back data to the CPU
            usage = D3D11_USAGE_STAGING;
            usageStr = "D3D11_USAGE_STAGING";
        } else {
            usage = D3D11_USAGE_DEFAULT;
            usageStr = "D3D11_USAGE_DEFAULT";
        }

        auto fmt = boost::format("Creating Texture: Size=%1%x%2%, Mips=%3%, Usage=%4%") % params->width % params->height % params->numMips % usageStr;

        LOGD << boost::str(fmt);

        uint32_t miscFlags = 0;

        if (isCube || isCubeArray)
            miscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        uint32_t cpuFlags = 0;
        uint32_t bindFlags = 0;

        if (isCPURead)
            cpuFlags = D3D11_CPU_ACCESS_READ;

        if (isSRV)
            bindFlags |= D3D11_BIND_SHADER_RESOURCE;

        if (isUAV)
            bindFlags |= D3D11_BIND_UNORDERED_ACCESS;

        auto numImageImpls = params->imageDatas.size();
        std::vector<D3D11_SUBRESOURCE_DATA> initialData(numImageImpls);

        for (auto i = 0; i < numImageImpls; ++i) {
            auto& subParam = params->imageDatas[i];
            auto& initData = initialData[i];

            initData.pSysMem = subParam.data;
            initData.SysMemPitch = subParam.rowPitch;
            initData.SysMemSlicePitch = subParam.depthPitch;
        }

        // we want to use unique_ptr here so that the object is properly destroyed in case it fails before we return
        auto impl = std::make_shared<DX11TextureInternal>();

        _tracker.RegisterObject(impl);

        impl->_desc = params;

        auto res = std::make_unique<DX11TextureImpl>(impl);

        HRESULT hr;
        std::string_view apiName;

        if (is2d) {
            D3D11_TEXTURE2D_DESC desc = {};

            desc.Width = params->width;
            desc.Height = params->height;
            desc.MipLevels = params->numMips;
            desc.ArraySize = params->arraySize;
            desc.Format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(params->format));
            desc.SampleDesc.Count = 1;
            desc.Usage = usage;
            desc.BindFlags = bindFlags;
            desc.CPUAccessFlags = cpuFlags;
            desc.MiscFlags = miscFlags;

            impl->_texture.emplace<DX11Tex2D>();
            hr = _device->CreateTexture2D(&desc, initialData.data(), std::get<DX11Tex2D>(impl->_texture).GetAddressOf());
            apiName = "ID3D11Device::CreateTexture2D";
        } else if (is1d) {
            D3D11_TEXTURE1D_DESC desc = {};

            desc.Width = params->width;
            desc.MipLevels = params->numMips;
            desc.ArraySize = params->arraySize;
            desc.Format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(params->format));
            desc.Usage = usage;
            desc.BindFlags = bindFlags;
            desc.CPUAccessFlags = cpuFlags;
            desc.MiscFlags = miscFlags;

            impl->_texture.emplace<DX11Tex1D>();
            hr = _device->CreateTexture1D(&desc, initialData.data(), std::get<DX11Tex1D>(impl->_texture).GetAddressOf());
            apiName = "ID3D11Device::CreateTexture1D";
        } else {
            // is3d
            D3D11_TEXTURE3D_DESC desc = {};

            desc.Width = params->width;
            desc.Height = params->height;
            desc.Depth = params->depth;
            desc.MipLevels = params->numMips;
            desc.Format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(params->format));
            desc.Usage = usage;
            desc.BindFlags = bindFlags;
            desc.CPUAccessFlags = cpuFlags;
            desc.MiscFlags = miscFlags;

            impl->_texture.emplace<DX11Tex3D>();
            hr = _device->CreateTexture3D(&desc, initialData.data(), std::get<DX11Tex3D>(impl->_texture).GetAddressOf());
            apiName = "ID3D11Device::CreateTexture3D";
        }

        if (CheckAPIFailed(hr, apiName))
            return TextureHandle();

        if (isSRV) {
            TextureSRVParams srvParams = {};

            srvParams.obj = res.get();
            srvParams.texParams = params;
            srvParams.is1d = is1d;
            srvParams.is2d = is2d;
            srvParams.is3d = is3d;
            srvParams.isCube = isCube;
            srvParams.isCubeArray = isCubeArray;

            if (!MakeTextureSRV(srvParams))
                return TextureHandle();
        }

        if (isUAV) {
            impl->_uav.resize(params->numMips);

            // we have to create an UAV for each miplevel
            for (uint32_t i = 0; i < params->numMips; ++i) {
                D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(params->format));

                if (params->arraySize > 1) {
                    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = i;
                    uavDesc.Texture2DArray.ArraySize = params->arraySize;
                } else {
                    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                    uavDesc.Texture2D.MipSlice = i;
                }

                auto uav = new DX11UnorderedAccessView();

                hr = _device->CreateUnorderedAccessView(impl->GetResource(), &uavDesc, uav->_resource.GetAddressOf());

                if (CheckAPIFailed(hr, "ID3D11Device::CreateUnorderedAccessView")) {
                    return TextureHandle();
                } else {
                    impl->_uav[i].reset(uav);
                }
            }
        }

        return res;
    }

    std::string_view DX11::DxSRVDimensionToString(D3D_SRV_DIMENSION dimension)
    {
        std::string_view res;

        switch (dimension) {
            case D3D11_SRV_DIMENSION_BUFFER:
                res = "D3D11_SRV_DIMENSION_BUFFER";
                break;
            case D3D11_SRV_DIMENSION_TEXTURE1D:
                res = "D3D11_SRV_DIMENSION_TEXTURE1D";
                break;
            case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
                res = "D3D11_SRV_DIMENSION_TEXTURE1DARRAY";
                break;
            case D3D11_SRV_DIMENSION_TEXTURE2D:
                res = "D3D11_SRV_DIMENSION_TEXTURE2D";
                break;
            case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
                res = "D3D11_SRV_DIMENSION_TEXTURE2DARRAY";
                break;
            case D3D11_SRV_DIMENSION_TEXTURE3D:
                res = "D3D11_SRV_DIMENSION_TEXTURE3D";
                break;
            case D3D11_SRV_DIMENSION_TEXTURECUBE:
                res = "D3D11_SRV_DIMENSION_TEXTURECUBE";
                break;
            case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
                res = "D3D11_SRV_DIMENSION_TEXTURECUBEARRAY";
                break;
            case D3D11_SRV_DIMENSION_BUFFEREX:
                res = "D3D11_SRV_DIMENSION_BUFFEREX";
                break;
            default:
                throw new std::exception("Unsupported view dimension");
                break;
        }

        return res;
    }

    bool DX11::Dispatch(const CommandHandle& cmd)
    {
        auto found = _shaders.find(cmd->shader);

        if (found == _shaders.end()) {
            auto fmt = boost::format("Dispatch error: could not find shader \"%1%\"") % cmd->shader;
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

        // all of this to wrap a static_cast
        static std::function<ID3D11ShaderResourceView*(const ShaderResourceView*)> srvCast = &DX11::castGenericResourceToDX11Resource<ShaderResourceView, DX11ShaderResourceView, ID3D11ShaderResourceView>;
        static std::function<ID3D11UnorderedAccessView*(const UnorderedAccessView*)> uavCast = &DX11::castGenericResourceToDX11Resource<UnorderedAccessView, DX11UnorderedAccessView, ID3D11UnorderedAccessView>;
        static std::function<ID3D11SamplerState*(const SamplerState*)> ssCast = &DX11::castGenericResourceToDX11Resource<SamplerState, DX11SamplerState, ID3D11SamplerState>;

        auto lambda = [&](auto & kvp, auto & container, auto castFn)
        {
            auto f = cs.bindSlots.find(kvp.first);

            if (f != cs.bindSlots.end()) {
                container.insert(f->second, castFn(kvp.second));
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

            for (auto& kvp : cmd->srvBindings) {
                if (!lambda(kvp, vmSRV, srvCast))
                    return false;
            }

            vmSRV.endInsert();
        }

        // UAV
        {
            vmUAV.beginInsert();

            for (auto& kvp : cmd->uavBindings) {
                if (!lambda(kvp, vmUAV, uavCast))
                    return false;
            }

            vmUAV.endInsert();
        }

        // Sample states
        {
            vmSS.beginInsert();

            for (auto& kvp : cmd->ssBindings) {
                if (!lambda(kvp, vmSS, ssCast))
                    return false;
            }

            vmSS.endInsert();
        }

        // Set shader data
        _context->CSSetShader(cs.shader.Get(), nullptr, 0);

        // Constant buffers
        auto foundCB = _cBuffers.find(cmd->cbufferStr);

        if (foundCB != _cBuffers.end()) {
            std::array<ID3D11Buffer*, 1> cbs{ foundCB->second.Get() };

            _context->CSSetConstantBuffers(0, 1, cbs.data());
        }

        _context->CSSetSamplers(0, vmSS.size(), vmSS.data());
        _context->CSSetUnorderedAccessViews(0, vmUAV.size(), vmUAV.data(), nullptr);
        _context->CSSetShaderResources(0, vmSRV.size(), vmSRV.data());
        _context->Dispatch(cmd->dispatch[0], cmd->dispatch[1], cmd->dispatch[2]);
        _context->Flush();
        return true;
    }

    void DX11::Finalize()
    {
        _tracker.ReleaseObjects();
    }

    bool DX11::Initialize()
    {
        auto adapter = 0;

        if ((_type & ERenderer::RENDERER_WARP) != 0)
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

        // sampler states
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.MinLOD = -D3D11_FLOAT32_MAX;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

        auto sampler = new DX11SamplerState();

        auto hr = _device->CreateSamplerState(&samplerDesc, sampler->_resource.GetAddressOf());

        if (CheckAPIFailed(hr, "ID3D11Device::CreateSamplerState for nearest")) {
            return false;
        } else {
            _samplers[static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Point)].reset(sampler);
        }

        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

        sampler = new DX11SamplerState();

        hr = _device->CreateSamplerState(&samplerDesc, sampler->_resource.GetAddressOf());

        if (CheckAPIFailed(hr, "ID3D11Device::CreateSamplerState for linear")) {
            return false;
        } else {
            _samplers[static_cast<std::underlying_type<ESamplerState>::type>(ESamplerState::SS_Linear)].reset(sampler);
        }

        return true;
    }

    bool DX11::LoadShader(const std::filesystem::path& path)
    {
        if (std::filesystem::is_directory(path)) {
            return LoadShaders(path);
        } else if (path.extension() == ShaderExt) {
            auto fmt = boost::format("Loading %1%..") % path;

            LOG_INDENT_START << boost::str(fmt);

            ID3DBlob* blob = nullptr;
            auto hr = D3DReadFileToBlob(ninniku::strToWStr(path.string()).c_str(), &blob);

            if (FAILED(hr)) {
                fmt = boost::format("Failed to D3DReadFileToBlob: %1% with:") % path;
                LOGE << boost::str(fmt);
                _com_error err(hr);
                LOGE << err.ErrorMessage();
                LOG_INDENT_END;
                return false;
            }

            if (!LoadShader(path, blob)) {
                LOG_INDENT_END;
                return false;
            }

            LOG_INDENT_END;
        }

        return true;
    }

    bool DX11::LoadShader(const std::string_view& name, const void* pData, const uint32_t size)
    {
        auto fmt = boost::format("Loading shader: %1% directly from memory");

        ID3DBlob* pBlob = nullptr;
        auto hr = D3DCreateBlob(size, &pBlob);

        if (CheckAPIFailed(hr, "D3DCreateBlob"))
            return false;

        memcpy_s(pBlob->GetBufferPointer(), size, pData, size);

        return LoadShader(name, pBlob);
    }

    bool DX11::LoadShader(const std::filesystem::path& path, ID3DBlob* pBlob)
    {
        // reflection
        ID3D11ShaderReflection* reflect = nullptr;
        D3D11_SHADER_DESC desc;

        auto hr = D3DReflect(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(&reflect));
        if (FAILED(hr)) {
            auto fmt = boost::format("Failed to call D3DReflect on shader: %1% with:") % path;
            LOGE << boost::str(fmt);
            _com_error err(hr);
            LOGE << err.ErrorMessage();
            return false;
        }

        hr = reflect->GetDesc(&desc);
        if (FAILED(hr)) {
            auto fmt = boost::format("Failed to GetDesc from shader reflection on shader: %1% with:") % path;
            LOGE << boost::str(fmt);
            _com_error err(hr);
            LOGE << err.ErrorMessage();
            return false;
        }

        auto bindings = ParseShaderResources(desc.BoundResources, reflect);

        // create shader
        DX11CS shader;

        hr = _device->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, shader.GetAddressOf());

        if (CheckAPIFailed(hr, "CreateComputeShader")) {
            return false;
        } else {
            auto name = path.stem().string();
            LOGDF(boost::format("Adding CS: \"%1%\" to library") % name);

            _shaders.emplace(name, DX11ComputeShader{ shader, bindings });
        }

        return true;
    }

    /// <summary>
    /// Load all shaders in /data
    /// </summary>
    bool DX11::LoadShaders(const std::filesystem::path& shaderPath)
    {
        // check if directory is valid
        if (!std::filesystem::is_directory(shaderPath)) {
            auto fmt = boost::format("Failed to open directory: %1%") % shaderPath;
            LOGE << boost::str(fmt);

            return false;
        }

        // Count the number of .cso found
        std::filesystem::directory_iterator begin(shaderPath), end;

        auto fileCounter = [&](const std::filesystem::directory_entry & d)
        {
            return (!is_directory(d.path()) && (d.path().extension() == ShaderExt));
        };

        auto numFiles = std::count_if(begin, end, fileCounter);
        auto fmt = boost::format("Found %1% compiled shaders in /%2%") % numFiles % shaderPath;

        LOGD << boost::str(fmt);

        for (auto& iter : std::filesystem::recursive_directory_iterator(shaderPath)) {
            LoadShader(iter.path());
        }

        return true;
    }

    bool DX11::MakeTextureSRV(const TextureSRVParams& params)
    {
        auto impl = static_cast<DX11TextureImpl*>(params.obj);
        auto internal = impl->_impl.lock();

        auto lmbd = [&](ID3D11Resource * resource, D3D11_SHADER_RESOURCE_VIEW_DESC & desc, SRVHandle & target)
        {
            auto srv = new DX11ShaderResourceView();
            auto hr = _device->CreateShaderResourceView(resource, &desc, srv->_resource.GetAddressOf());

            auto fmt = boost::format("ID3D11Device::CreateShaderResourceView with %1%") % DxSRVDimensionToString(desc.ViewDimension);

            if (CheckAPIFailed(hr, boost::str(fmt))) {
                return false;
            } else {
                target.reset(srv);
            }

            return true;
        };

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

        srvDesc.Format = static_cast<DXGI_FORMAT>(NinnikuTFToDXGIFormat(params.texParams->format));

        if (params.isCubeArray) {
            srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBEARRAY;
            srvDesc.TextureCubeArray = {};
            srvDesc.TextureCubeArray.MipLevels = params.texParams->numMips;
            srvDesc.TextureCubeArray.NumCubes = params.texParams->arraySize / CUBEMAP_NUM_FACES;

            if (!lmbd(internal->GetResource(), srvDesc, internal->_srvCubeArray))
                return false;
        }

        if (params.isCube) {
            // To sample texture as cubemap
            srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube = {};
            srvDesc.TextureCube.MipLevels = params.texParams->numMips;

            if (!lmbd(internal->GetResource(), srvDesc, internal->_srvCube))
                return false;

            // To sample texture as array, one for each miplevel
            internal->_srvArray.resize(params.texParams->numMips);

            for (uint32_t i = 0; i < params.texParams->numMips; ++i) {
                srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray = {};
                srvDesc.Texture2DArray.ArraySize = CUBEMAP_NUM_FACES;
                srvDesc.Texture2DArray.MostDetailedMip = i;
                srvDesc.Texture2DArray.MipLevels = 1;

                if (!lmbd(internal->GetResource(), srvDesc, internal->_srvArray[i]))
                    return false;
            }

            // one for array with all mips
            srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray = {};
            srvDesc.Texture2DArray.ArraySize = CUBEMAP_NUM_FACES;
            srvDesc.Texture2DArray.MostDetailedMip = 0;
            srvDesc.Texture2DArray.MipLevels = params.texParams->numMips;

            if (!lmbd(internal->GetResource(), srvDesc, internal->_srvArrayWithMips))
                return false;
        } else if (params.texParams->arraySize > 1) {
            // one for each miplevel
            internal->_srvArray.resize(params.texParams->numMips);

            for (uint32_t i = 0; i < params.texParams->numMips; ++i) {
                if (params.is1d) {
                    srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1DARRAY;
                    srvDesc.Texture1DArray = {};
                    srvDesc.Texture1DArray.ArraySize = params.texParams->arraySize;
                    srvDesc.Texture1DArray.MostDetailedMip = i;
                    srvDesc.Texture1DArray.MipLevels = 1;
                } else {
                    srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
                    srvDesc.Texture2DArray = {};
                    srvDesc.Texture2DArray.ArraySize = params.texParams->arraySize;
                    srvDesc.Texture2DArray.MostDetailedMip = i;
                    srvDesc.Texture2DArray.MipLevels = 1;
                }

                if (!lmbd(internal->GetResource(), srvDesc, internal->_srvArray[i]))
                    return false;
            }
        } else {
            if (params.is3d) {
                srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D = {};
                srvDesc.Texture3D.MipLevels = params.texParams->numMips;
            } else if (params.is1d) {
                srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1D;
                srvDesc.Texture1D = {};
                srvDesc.Texture1D.MipLevels = params.texParams->numMips;
            } else {
                srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D = {};
                srvDesc.Texture2D.MipLevels = params.texParams->numMips;
            }

            if (!lmbd(internal->GetResource(), srvDesc, internal->_srvDefault))
                return false;
        }

        return true;
    }

    MappedResourceHandle DX11::Map(const BufferHandle& bObj)
    {
        auto res = std::make_unique<DX11MappedResource>(_context, bObj);
        auto impl = static_cast<const DX11BufferImpl*>(bObj.get());
        auto internal = impl->_impl.lock();

        auto hr = _context->Map(internal->_buffer.Get(), 0, D3D11_MAP_READ, 0, res->Get());

        if (CheckAPIFailed(hr, "ID3D11DeviceContext::Map"))
            return std::unique_ptr<MappedResource>();

        return res;
    }

    MappedResourceHandle DX11::Map(const TextureHandle& tObj, const uint32_t index)
    {
        auto res = std::make_unique<DX11MappedResource>(_context, tObj, index);
        auto impl = static_cast<const DX11TextureImpl*>(tObj.get());
        auto internal = impl->_impl.lock();

        auto hr = _context->Map(internal->GetResource(), index, D3D11_MAP_READ, 0, res->Get());

        if (CheckAPIFailed(hr, "ID3D11DeviceContext::Map"))
            return std::unique_ptr<MappedResource>();

        return res;
    }

    StringMap<uint32_t> DX11::ParseShaderResources(uint32_t numBoundResources, ID3D11ShaderReflection* reflection)
    {
        StringMap<uint32_t> res;

        // parse parameter bind slots
        for (uint32_t i = 0; i < numBoundResources; ++i) {
            D3D11_SHADER_INPUT_BIND_DESC bindDesc;

            auto hr = reflection->GetResourceBindingDesc(i, &bindDesc);

            if (CheckAPIFailed(hr, "ID3D11ShaderReflection::GetResourceBindingDesc"))
                return res;

            std::string_view restypeStr;

            switch (bindDesc.Type) {
                case D3D_SIT_CBUFFER:
                    restypeStr = "D3D_SIT_CBUFFER";
                    break;

                case D3D_SIT_SAMPLER:
                    restypeStr = "D3D_SIT_SAMPLER";
                    break;

                case D3D_SIT_STRUCTURED:
                    restypeStr = "D3D_SIT_STRUCTURED";
                    break;

                case D3D_SIT_TEXTURE:
                    restypeStr = "D3D_SIT_TEXTURE";
                    break;

                case D3D_SIT_UAV_RWTYPED:
                    restypeStr = "D3D_SIT_UAV_RWTYPED";
                    break;

                case D3D_SIT_UAV_RWSTRUCTURED:
                    restypeStr = "D3D_SIT_UAV_RWSTRUCTURED";
                    break;

                default:
                    LOG << "DX11::ParseShaderResources unsupported type";
                    return res;
            }

            auto fmt = boost::format("Resource: Name=\"%1%\", Type=%2%, Slot=%3%") % bindDesc.Name % restypeStr % bindDesc.BindPoint;

            LOGD << boost::str(fmt);

            // if the texture is a constant buffer, we want to create it a slot for it in the map
            if (bindDesc.Type == D3D_SIT_CBUFFER) {
                _cBuffers.emplace(bindDesc.Name, DX11Buffer{});
            }

            res.emplace(bindDesc.Name, bindDesc.BindPoint);
        }

        return res;
    }

    bool DX11::UpdateConstantBuffer(const std::string_view& name, void* data, const uint32_t size)
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

            if (CheckAPIFailed(hr, "ID3D11Device::CreateBuffer"))
                return false;
        } else {
            // constant buffer already exists so just update it

            D3D11_MAPPED_SUBRESOURCE mapped = {};
            auto src = _cBuffers[name].Get();
            auto hr = _context->Map(src, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

            if (CheckAPIFailed(hr, "ID3D11DeviceContext::Map"))
                return false;

            memcpy_s(mapped.pData, size, data, size);
            _context->Unmap(src, 0);
        }

        return true;
    }
} // namespace ninniku