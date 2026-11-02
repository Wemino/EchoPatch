#pragma once

#include <algorithm>
#include <cmath>

#include "../../Globals.cpp"
#include "../../Shaders/ssaa_resolve_cso.hpp"

// ===========================
// SSAAResolve
// ===========================

static const uint8_t* const ResolvePixelShaders[] = { SSAAResolvePS2_cso, SSAAResolvePS3_cso, SSAAResolvePS4_cso, SSAAResolvePS5_cso };
static constexpr int MinResolveTaps = 2;

static IDirect3DDevice9* s_resolveDevice = nullptr;
static IDirect3DVertexShader9* s_resolveVertexShader = nullptr;
static IDirect3DPixelShader9* s_resolvePixelShader = nullptr;
static IDirect3DVertexDeclaration9* s_resolveVertexDecl = nullptr;
static D3DFORMAT s_resolveFormat = D3DFMT_UNKNOWN;
static int s_resolveTaps = 0;
static DWORD s_resolveMaxRenderTargets = 1;
static bool s_resolveLinear = false;
static bool s_resolveFailed = false;

static void ReleaseSSAAResolve()
{
    if (s_resolveVertexShader) s_resolveVertexShader->Release();
    if (s_resolvePixelShader) s_resolvePixelShader->Release();
    if (s_resolveVertexDecl) s_resolveVertexDecl->Release();

    s_resolveDevice = nullptr;
    s_resolveVertexShader = nullptr;
    s_resolvePixelShader = nullptr;
    s_resolveVertexDecl = nullptr;
    s_resolveFormat = D3DFMT_UNKNOWN;
    s_resolveTaps = 0;
    s_resolveFailed = false;
}

static bool CreateSSAAResolve(IDirect3DDevice9* device, D3DFORMAT format, int taps)
{
    if (device == s_resolveDevice && format == s_resolveFormat && taps == s_resolveTaps)
        return !s_resolveFailed;

    ReleaseSSAAResolve();
    s_resolveDevice = device;
    s_resolveFormat = format;
    s_resolveTaps = taps;
    s_resolveFailed = true;

    D3DCAPS9 caps;
    if (taps < MinResolveTaps || taps >= MinResolveTaps + (int)std::size(ResolvePixelShaders))
        return false;

    if (FAILED(device->GetDeviceCaps(&caps)) || caps.PixelShaderVersion < D3DPS_VERSION(3, 0) || caps.VertexShaderVersion < D3DVS_VERSION(3, 0))
        return false;

    s_resolveMaxRenderTargets = std::clamp(caps.NumSimultaneousRTs, 1ul, 4ul);

    IDirect3D9* d3d = nullptr;
    D3DDEVICE_CREATION_PARAMETERS params;
    D3DDISPLAYMODE mode;
    s_resolveLinear = false;

    if (SUCCEEDED(device->GetDirect3D(&d3d)) && SUCCEEDED(device->GetCreationParameters(&params)) && SUCCEEDED(d3d->GetAdapterDisplayMode(params.AdapterOrdinal, &mode)))
    {
        s_resolveLinear = SUCCEEDED(d3d->CheckDeviceFormat(params.AdapterOrdinal, params.DeviceType, mode.Format, D3DUSAGE_RENDERTARGET | D3DUSAGE_QUERY_SRGBREAD | D3DUSAGE_QUERY_SRGBWRITE, D3DRTYPE_TEXTURE, format));
    }

    if (d3d) d3d->Release();

    const D3DVERTEXELEMENT9 elements[] =
    {
        { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END()
    };

    if (FAILED(device->CreateVertexShader(reinterpret_cast<const DWORD*>(SSAAResolveVS_cso), &s_resolveVertexShader)) ||
        FAILED(device->CreatePixelShader(reinterpret_cast<const DWORD*>(ResolvePixelShaders[taps - MinResolveTaps]), &s_resolvePixelShader)) ||
        FAILED(device->CreateVertexDeclaration(elements, &s_resolveVertexDecl)))
    {
        return false;
    }

    s_resolveFailed = false;
    return true;
}

static bool ResolveSSAA(IDirect3DDevice9* device, IDirect3DTexture9* source, IDirect3DSurface9* dest)
{
    D3DSURFACE_DESC sourceDesc, destDesc;
    if (FAILED(source->GetLevelDesc(0, &sourceDesc)) || FAILED(dest->GetDesc(&destDesc)))
        return false;

    float ratioX = (float)sourceDesc.Width / destDesc.Width;
    float ratioY = (float)sourceDesc.Height / destDesc.Height;
    int taps = (int)std::ceil(std::max(ratioX, ratioY)) + 1;

    if (!CreateSSAAResolve(device, sourceDesc.Format, taps))
        return false;

    IDirect3DSurface9* renderTargets[4] = {};
    IDirect3DSurface9* depthStencil = nullptr;
    IDirect3DVertexShader9* vertexShader = nullptr;
    IDirect3DPixelShader9* pixelShader = nullptr;
    IDirect3DVertexDeclaration9* vertexDecl = nullptr;
    IDirect3DVertexBuffer9* streamSource = nullptr;
    IDirect3DBaseTexture9* texture = nullptr;
    UINT streamOffset = 0, streamStride = 0;
    DWORD fvf = 0;
    D3DVIEWPORT9 viewport;
    RECT scissorRect;
    float psConstants[2][4];

    DWORD renderStates[][2] =
    {
        { D3DRS_ZENABLE, D3DZB_FALSE }, { D3DRS_ZWRITEENABLE, FALSE }, { D3DRS_ALPHABLENDENABLE, FALSE },
        { D3DRS_ALPHATESTENABLE, FALSE }, { D3DRS_STENCILENABLE, FALSE }, { D3DRS_CULLMODE, D3DCULL_NONE },
        { D3DRS_SCISSORTESTENABLE, FALSE }, { D3DRS_COLORWRITEENABLE, 0xF }, { D3DRS_FILLMODE, D3DFILL_SOLID },
        { D3DRS_CLIPPLANEENABLE, 0 }, { D3DRS_FOGENABLE, FALSE }, { D3DRS_SRGBWRITEENABLE, s_resolveLinear }
    };

    DWORD samplerStates[][2] =
    {
        { D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP }, { D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP }, { D3DSAMP_MAGFILTER, D3DTEXF_POINT },
        { D3DSAMP_MINFILTER, D3DTEXF_POINT }, { D3DSAMP_MIPFILTER, D3DTEXF_NONE }, { D3DSAMP_MAXMIPLEVEL, 0 },
        { D3DSAMP_MIPMAPLODBIAS, 0 }, { D3DSAMP_SRGBTEXTURE, s_resolveLinear }
    };

    for (DWORD i = 0; i < s_resolveMaxRenderTargets; i++)
    {
        device->GetRenderTarget(i, &renderTargets[i]);
    }

    device->GetDepthStencilSurface(&depthStencil);
    device->GetViewport(&viewport);
    device->GetScissorRect(&scissorRect);
    device->GetVertexShader(&vertexShader);
    device->GetPixelShader(&pixelShader);
    device->GetVertexDeclaration(&vertexDecl);
    device->GetFVF(&fvf);
    device->GetStreamSource(0, &streamSource, &streamOffset, &streamStride);
    device->GetTexture(0, &texture);
    device->GetPixelShaderConstantF(0, &psConstants[0][0], 2);

    for (auto& state : renderStates)
    {
        DWORD value = state[1];
        device->GetRenderState((D3DRENDERSTATETYPE)state[0], &state[1]);
        device->SetRenderState((D3DRENDERSTATETYPE)state[0], value);
    }

    for (auto& state : samplerStates)
    {
        DWORD value = state[1];
        device->GetSamplerState(0, (D3DSAMPLERSTATETYPE)state[0], &state[1]);
        device->SetSamplerState(0, (D3DSAMPLERSTATETYPE)state[0], value);
    }

    for (DWORD i = 1; i < s_resolveMaxRenderTargets; i++)
    {
        if (renderTargets[i]) device->SetRenderTarget(i, nullptr);
    }

    D3DVIEWPORT9 destViewport = { 0, 0, destDesc.Width, destDesc.Height, 0.0f, 1.0f };
    device->SetRenderTarget(0, dest);
    device->SetDepthStencilSurface(nullptr);
    device->SetViewport(&destViewport);
    device->SetVertexShader(s_resolveVertexShader);
    device->SetPixelShader(s_resolvePixelShader);
    device->SetVertexDeclaration(s_resolveVertexDecl);
    device->SetTexture(0, source);

    const float constants[2][4] =
    {
        { (float)sourceDesc.Width, (float)sourceDesc.Height, 1.0f / sourceDesc.Width, 1.0f / sourceDesc.Height },
        { ratioX, ratioY, ratioX * 0.5f, ratioY * 0.5f },
    };
    device->SetPixelShaderConstantF(0, &constants[0][0], 2);

    // Covers the whole target, the pixel shader works from VPOS
    const float quad[4][4] =
    {
        { -1.0f,  1.0f, 0.0f, 1.0f },
        {  1.0f,  1.0f, 0.0f, 1.0f },
        { -1.0f, -1.0f, 0.0f, 1.0f },
        {  1.0f, -1.0f, 0.0f, 1.0f },
    };
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));

    // SetRenderTarget resets the viewport, so the targets go first
    for (DWORD i = 0; i < s_resolveMaxRenderTargets; i++)
    {
        if (i == 0 || renderTargets[i]) device->SetRenderTarget(i, renderTargets[i]);
        if (renderTargets[i]) renderTargets[i]->Release();
    }

    device->SetDepthStencilSurface(depthStencil);
    device->SetViewport(&viewport);
    device->SetScissorRect(&scissorRect);
    device->SetVertexShader(vertexShader);
    device->SetPixelShader(pixelShader);
    if (vertexDecl) device->SetVertexDeclaration(vertexDecl);
    if (fvf) device->SetFVF(fvf);
    device->SetStreamSource(0, streamSource, streamOffset, streamStride);
    device->SetTexture(0, texture);
    device->SetPixelShaderConstantF(0, &psConstants[0][0], 2);

    for (const auto& state : renderStates)
    {
        device->SetRenderState((D3DRENDERSTATETYPE)state[0], state[1]);
    }

    for (const auto& state : samplerStates)
    {
        device->SetSamplerState(0, (D3DSAMPLERSTATETYPE)state[0], state[1]);
    }

    if (depthStencil) depthStencil->Release();
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();
    if (vertexDecl) vertexDecl->Release();
    if (streamSource) streamSource->Release();
    if (texture) texture->Release();

    return true;
}
