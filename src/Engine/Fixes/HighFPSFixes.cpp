#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

static float lastPositiveYVelocity = 0.0f;

int(__stdcall* SetVelocity)(int, float*) = nullptr;

// ========================
// HighFPSFixes
// ========================

static int __stdcall SetVelocity_Hook(int obj, float* vel)
{
    if (obj)
    {
        float currentY = vel[1];

        if (g_State.pendingVelocityFix)
        {
            g_State.pendingVelocityFix = false;

            if (g_State.simulationFrameTime > 0.0f && g_State.simulationFrameTime < TARGET_FRAME_TIME && currentY > 0.0f && currentY < lastPositiveYVelocity)
            {
                float frameRatio = static_cast<float>(g_State.simulationFrameTime) / TARGET_FRAME_TIME;
                float preserveRatio = (1.0f - frameRatio) * 0.125f;

                float difference = lastPositiveYVelocity - currentY;
                vel[1] = currentY + difference * preserveRatio;
                return SetVelocity(obj, vel);
            }
        }
        if (currentY > 0.0f)
        {
            lastPositiveYVelocity = currentY;
        }
        else
        {
            lastPositiveYVelocity = 0.0f;
        }
    }

    return SetVelocity(obj, vel);
}

static void ApplyFixHighFPSPhysics()
{
    if (!HighFPSFixes) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::SetVelocity), &SetVelocity_Hook, (LPVOID*)&SetVelocity, g_State.CurrentFEARGame == FEAR);
}
