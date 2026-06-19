#pragma once
#define NOMINMAX

#include "../Globals.cpp"
#include "../Controller/Controller.hpp"
#include "../Controller/ScreenJoystickHook.hpp"
#include "../ClientFX/ClientFX.hpp"
#include "../Server/Server.hpp"

// ======================
// Client Function Pointers
// ======================

bool(__thiscall* LoadFxDll)(int, char*, char) = nullptr;

// HighFPSFixes
void(__thiscall* UpdateOnGround)(int) = nullptr;
void(__thiscall* UpdateWaveProp)(int, float) = nullptr;
double(__thiscall* GetMaxRecentVelocityMag)(int) = nullptr;
void(__cdecl* PolyGridFXCollisionHandlerCB)(int, int, int*, int*, float, BYTE*, int) = nullptr;
void(__thiscall* UpdateNormalControlFlags)(int) = nullptr;
void(__thiscall* UpdateNormalFriction)(int) = nullptr;
double(__thiscall* GetTimerElapsedS)(int) = nullptr;
void(__thiscall* MoveLocalSolidObject)(int*) = nullptr;
void(__thiscall* EnterSlowMo)(int*, int, bool, double, bool) = nullptr;
unsigned int(__thiscall* HandleMsgSlowMo)(int*, void*) = nullptr;
void(__thiscall* UpdateSlowMo)(int*) = nullptr;

// FixKeyboardInputLanguage
void(__thiscall* LoadUserProfile)(int, bool, bool) = nullptr;
bool(__thiscall* RestoreDefaults)(int, uint8_t) = nullptr;

// WeaponFixes
BYTE* (__thiscall* AimMgrCtor)(BYTE*) = nullptr;
void(__thiscall* AnimationClearLock)(DWORD) = nullptr;
void(__thiscall* UpdateWeaponModel)(DWORD*) = nullptr;
void(__thiscall* SetAnimProp)(DWORD*, int, int) = nullptr;
bool(__thiscall* InitAnimations)(DWORD*) = nullptr;
void(__thiscall* NextWeapon)(DWORD*) = nullptr;
void(__thiscall* PreviousWeapon)(DWORD*) = nullptr;
unsigned __int8(__thiscall* GetWeaponSlot)(int, int) = nullptr;

// HighResolutionReflections
bool(__thiscall* RenderTargetGroupFXInit)(int, DWORD*) = nullptr;

// SSAAScale
void(__thiscall* Camera_UpdateRenderTarget)(int) = nullptr;

// EnablePersistentWorldState
float(__stdcall* GetShatterLifetime)(int) = nullptr;
int(__stdcall* CreateFX)(char*, int, int) = nullptr;

// HUDScaling
void(__thiscall* HUDTerminate)(int) = nullptr;
bool(__thiscall* HUDInit)(int) = nullptr;
void(__thiscall* HUDRender)(int, int) = nullptr;
int(__thiscall* HUDWeaponListReset)(int) = nullptr;
bool(__thiscall* HUDWeaponListInit)(int) = nullptr;
bool(__thiscall* HUDGrenadeListInit)(int) = nullptr;
int(__thiscall* ScreenDimsChanged)(int) = nullptr;
DWORD* (__stdcall* LayoutDBGetPosition)(DWORD*, int, char*, int) = nullptr;
float* (__stdcall* GetRectF)(DWORD*, int, char*, int) = nullptr;
int(__stdcall* DBGetRecord)(int, char*) = nullptr;
int(__stdcall* DBGetInt32)(int, unsigned int, int) = nullptr;
float(__stdcall* DBGetFloat)(int, unsigned int, float) = nullptr;
const char* (__stdcall* DBGetString)(int, unsigned int, int) = nullptr;
void(__stdcall* InitAdditionalTextureData)(int, int, int*, DWORD*, DWORD*, float) = nullptr;
void(__thiscall* HUDPausedInit)(int) = nullptr;

// AutoResolution
void(__cdecl* AutoDetectPerformanceSettings)() = nullptr;
void(__thiscall* SetOption)(int, int, int, int, int) = nullptr;
bool(__thiscall* SetQueuedConsoleVariable)(int, const char*, float, int) = nullptr;

// SDLGamepadSupport
bool(__thiscall* IsCommandOn)(int, int) = nullptr;
bool(__thiscall* OnCommandOn)(int, int) = nullptr;
bool(__thiscall* OnCommandOff)(int, int) = nullptr;
double(__thiscall* GetExtremalCommandValue)(int, int) = nullptr;
double(__thiscall* GetZoomMag)(int) = nullptr;
int(__thiscall* HUDActivateObjectSetObject)(int, void**, int, int, int, int) = nullptr;
int(__thiscall* SetOperatingTurret)(int, int) = nullptr;
const wchar_t* (__thiscall* GetTriggerNameFromCommandID)(int, int) = nullptr;
void(__thiscall* UseCursor)(int, bool, bool) = nullptr;
bool(__thiscall* OnMouseMove)(int, int, int) = nullptr;
void(__thiscall* ForceMouseUpdate)(DWORD*);
void(__thiscall* HUDSwapUpdate)(int) = nullptr;
void(__thiscall* SwitchToScreen)(int, int) = nullptr;
void(__thiscall* SetCurrentType)(int, int) = nullptr;
void(__cdecl* HUDSwapUpdateTriggerName)() = nullptr;
void(__thiscall* ApplyLocalRotationOffset)(int, float*) = nullptr;
void(__thiscall* UpdatePlayerMovement)(int) = nullptr;
void(__thiscall* BeginAim)(BYTE*) = nullptr;
void(__thiscall* EndAim)(BYTE*) = nullptr;
void(__thiscall* CClientWeaponFire)(DWORD*) = nullptr;
void(__thiscall* HandleMsgPlayerDamage)(DWORD*, int*) = nullptr;
unsigned int(__thiscall* UpdateHealth)(DWORD*, unsigned int) = nullptr;
int(__thiscall* UpdateArmor)(DWORD*, unsigned int) = nullptr;
void(__thiscall* CHUDMgr_StartFlicker)(DWORD*, float) = nullptr;
bool(__cdecl* CClientWeapon_WeaponPath_OnImpactCB)(DWORD*, int) = nullptr;
bool(__thiscall* HandleFallLand)(DWORD*, float, int) = nullptr;
void(__thiscall* CTurretFX_SetDamageState)(DWORD*) = nullptr;
void(__thiscall* CycleCtrlSetSelIndex)(int, unsigned __int8) = nullptr;
const wchar_t* (__stdcall* LoadGameString)(int, char*) = nullptr;
bool(__stdcall* DEditLoadModule)(const char*) = nullptr;

// ConsoleEnabled
void(__stdcall* SetInputState)(bool) = nullptr;

// EnableCustomMaxWeaponCapacity
uint8_t(__thiscall* GetWeaponCapacity)(int) = nullptr;

// DisableHipFireAccuracyPenalty
void(__thiscall* AccuracyMgrUpdate)(float*) = nullptr;

// SDLGamepadSupport & HUDScaling
int(__thiscall* HUDWeaponListUpdateTriggerNames)(int) = nullptr;
int(__thiscall* HUDGrenadeListUpdateTriggerNames)(int) = nullptr;
void(__thiscall* SliderSetSliderPos)(int, int) = nullptr;

// EnableCustomMaxWeaponCapacity & WeaponFixes
void(__thiscall* OnEnterWorld)(int) = nullptr;

// SDLGamepadSupport & ConsoleEnabled
static int(__stdcall* HookedWindowProc)(HWND, UINT, WPARAM, LPARAM) = nullptr;
bool(__thiscall* ChangeState)(int, int, int) = nullptr;
void(__thiscall* MsgBoxShow)(int, const wchar_t*, int, int, bool) = nullptr;
void(__thiscall* MsgBoxHide)(int, int) = nullptr;
