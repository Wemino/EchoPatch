#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

// ===========================
// HardwareVertexProcessing
// ===========================

static void ApplyHardwareVertexProcessing()
{
    if (!HardwareVertexProcessing) return;

    DWORD addr_BehaviorFlags = GetAddress(Addr::DeviceBehaviorFlags);

    // 'and edi, 60h', mixed vertex processing when the GPU supports hardware T&L
    if (MemoryHelper::ReadMemory<uint8_t>(addr_BehaviorFlags) != 0x60) return;

    // D3DCREATE_MIXED_VERTEXPROCESSING -> D3DCREATE_HARDWARE_VERTEXPROCESSING, software vertex processing is never used
    MemoryHelper::WriteMemory<uint8_t>(addr_BehaviorFlags, 0x20);
}
