#pragma once

#include "../Globals.cpp"

void ApplyClientFXPatch()
{
    HookHelper::BeginBatch();
    ApplyHighFPSFixesClientFXPatch();
    ApplyControllerClientFXPatch();
    ApplyFixTranslucentRenderingClientFXPatch();
    HookHelper::EndBatch();
}
