#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

int(__stdcall* SetVelocity)(int, float*) = nullptr;
int(__thiscall* ProcessBreakableConstraint)(int, float*, int) = nullptr;
void(__cdecl* ProcessTwistLimitConstraint)(int, float*, float*) = nullptr;
void(__cdecl* ProcessConeLimitConstraint)(int, float*, float*, float*) = nullptr;
void(__cdecl* BuildJacobianRow) (int, float*, float*) = nullptr;
void(__thiscall* ProcessBallSocketConstraint)(int, float*, float*) = nullptr;
void(__thiscall* ProcessLimitedHingeConstraint)(int, float*, float*) = nullptr;

static inline bool ShouldClampRagdoll(int thisPtr)
{
    int owner = *reinterpret_cast<int*>(thisPtr + 0x14);
    if (!owner) return false;

    // Ragdolls have 5+ bodies (bones), simple props have 2-4
    int bodyCount = *reinterpret_cast<int*>(owner + 0x40);
    return bodyCount >= 5;
}

static inline bool IsHangingProp(int thisPtr)
{
    int owner = *reinterpret_cast<int*>(thisPtr + 0x14);
    if (!owner) return false;
    int bodyCount = *reinterpret_cast<int*>(owner + 0x40);
    return bodyCount == 3;
}

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

            if (g_State.simulationFrameTime > 0.0f && g_State.simulationFrameTime < TARGET_FRAME_TIME && currentY > 0.0f && currentY < g_State.lastPositiveYVelocity)
            {
                float frameRatio = static_cast<float>(g_State.simulationFrameTime) / TARGET_FRAME_TIME;
                float preserveRatio = (1.0f - frameRatio) * 0.125f;

                float difference = g_State.lastPositiveYVelocity - currentY;
                vel[1] = currentY + difference * preserveRatio;
                return SetVelocity(obj, vel);
            }
        }
        if (currentY > 0.0f)
        {
            g_State.lastPositiveYVelocity = currentY;
        }
        else
        {
            g_State.lastPositiveYVelocity = 0.0f;
        }
    }

    return SetVelocity(obj, vel);
}


static int __fastcall ProcessBreakableConstraint_Hook(int thisPtr, int, float* constraintInstance, int a3)
{
    // Stabilize the industrial strip light of the first map
    if (IsHangingProp(thisPtr) && constraintInstance[3] > 250.0f)
    {
        constraintInstance[3] = 250.0f;
        return ProcessBreakableConstraint(thisPtr, constraintInstance, a3);
    }

    return ProcessBreakableConstraint(thisPtr, constraintInstance, a3);
}

static void __fastcall ProcessBallSocketConstraint_Hook(int thisPtr, int, float* in, float* out)
{
    g_State.isProcessingRagdoll = ShouldClampRagdoll(thisPtr);
    ProcessBallSocketConstraint(thisPtr, in, out);
    g_State.isProcessingRagdoll = false;
}

static void __fastcall ProcessLimitedHingeConstraint_Hook(int thisPtr, int, float* in, float* out)
{
    g_State.isProcessingRagdoll = ShouldClampRagdoll(thisPtr);
    ProcessLimitedHingeConstraint(thisPtr, in, out);
    g_State.isProcessingRagdoll = false;
}

static void __cdecl BuildJacobianRow_Hook(int jacobianData, float* constraintInstance, float* queryIn)
{
    if (g_State.isProcessingRagdoll && constraintInstance[3] > 250.0f)
    {
        float timeStepBak = constraintInstance[3];
        constraintInstance[3] = 250.0f;
        BuildJacobianRow(jacobianData, constraintInstance, queryIn);
        constraintInstance[3] = timeStepBak;
        return;
    }

    BuildJacobianRow(jacobianData, constraintInstance, queryIn);
}

static void __cdecl ProcessTwistLimitConstraint_Hook(int twistParams, float* constraintInstance, float* queryIn)
{
    if (g_State.isProcessingRagdoll && constraintInstance[3] > 250.0f)
    {
        float timeStepBak = constraintInstance[3];
        constraintInstance[3] = 250.0f;
        ProcessTwistLimitConstraint(twistParams, constraintInstance, queryIn);
        constraintInstance[3] = timeStepBak;
        return;
    }

    ProcessTwistLimitConstraint(twistParams, constraintInstance, queryIn);
}

static void __cdecl ProcessConeLimitConstraint_Hook(int pivotA, float* pivotB, float* constraintInstance, float* queryIn)
{
    if (g_State.isProcessingRagdoll && constraintInstance[3] > 250.0f)
    {
        float timeStepBak = constraintInstance[3];
        constraintInstance[3] = 250.0f;
        ProcessConeLimitConstraint(pivotA, pivotB, constraintInstance, queryIn);
        constraintInstance[3] = timeStepBak;
        return;
    }

    ProcessConeLimitConstraint(pivotA, pivotB, constraintInstance, queryIn);
}

static void ApplyFixHighFPSPhysics()
{
    if (!HighFPSFixes) return;

    const bool checkRegion = g_State.CurrentFEARGame == FEAR;

    HookHelper::ApplyHook((void*)GetAddress(Addr::SetVelocity), &SetVelocity_Hook, (LPVOID*)&SetVelocity, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::ProcessBreakableConstraint), &ProcessBreakableConstraint_Hook, (LPVOID*)&ProcessBreakableConstraint, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::ProcessBallSocketConstraint), &ProcessBallSocketConstraint_Hook, (LPVOID*)&ProcessBallSocketConstraint, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::ProcessLimitedHingeConstraint), &ProcessLimitedHingeConstraint_Hook, (LPVOID*)&ProcessLimitedHingeConstraint, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::BuildJacobianRow), &BuildJacobianRow_Hook, (LPVOID*)&BuildJacobianRow, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::ProcessTwistLimitConstraint), &ProcessTwistLimitConstraint_Hook, (LPVOID*)&ProcessTwistLimitConstraint, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::ProcessConeLimitConstraint), &ProcessConeLimitConstraint_Hook, (LPVOID*)&ProcessConeLimitConstraint, checkRegion);
}
