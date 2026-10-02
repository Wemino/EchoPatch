#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

// ===========================
// FixNvidiaShadowCorruption
// ===========================

int(__thiscall* DrawMeshBatch)(const uint32_t*, const uint32_t*, uint32_t, uint32_t) = nullptr;

static IDirect3DDevice9* const* s_rendererDevice = nullptr;
static const int* s_disablePrimitiveRendering = nullptr;

static int __fastcall DrawMeshBatch_Hook(const uint32_t* mesh, int, const uint32_t* section, uint32_t primCount, uint32_t numVertices)
{
    IDirect3DDevice9* device = *s_rendererDevice;
    const uint32_t stride = reinterpret_cast<const uint8_t*>(section)[22];
    const uint32_t firstVertex = section[0];
    const uint32_t baseVertex = section[3];
    const uint64_t offset = static_cast<uint64_t>(baseVertex) * stride;

    if (!device || firstVertex < baseVertex || offset > mesh[6] || offset % 4)
        return DrawMeshBatch(mesh, section, primCount, numVertices);

    IDirect3DVertexBuffer9* vertexBuffer = reinterpret_cast<IDirect3DVertexBuffer9*>(static_cast<uintptr_t>(mesh[2]));
    device->SetVertexDeclaration(reinterpret_cast<IDirect3DVertexDeclaration9*>(static_cast<uintptr_t>(section[12])));

    // Fails on devices without stream offset support
    if (FAILED(device->SetStreamSource(0, vertexBuffer, static_cast<UINT>(offset), stride)))
        return DrawMeshBatch(mesh, section, primCount, numVertices);

    device->SetIndices(reinterpret_cast<IDirect3DIndexBuffer9*>(static_cast<uintptr_t>(mesh[3])));

    if (*s_disablePrimitiveRendering)
        return *s_disablePrimitiveRendering;

    return device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, firstVertex - baseVertex, numVertices, section[2], primCount);
}

static void ApplyFixNvidiaShadowCorruption()
{
    if (!FixNvidiaShadowCorruption) return;

    s_rendererDevice = reinterpret_cast<IDirect3DDevice9* const*>(GetAddress(Addr::RendererDevice));
    s_disablePrimitiveRendering = reinterpret_cast<const int*>(GetAddress(Addr::DisablePrimitiveRendering));
    if (!s_rendererDevice || !s_disablePrimitiveRendering) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::DrawMeshBatch), &DrawMeshBatch_Hook, (LPVOID*)&DrawMeshBatch, g_State.CurrentFEARGame == FEAR);
}
