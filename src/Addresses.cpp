#pragma once

#include "Globals.cpp"

enum class Addr
{
	// FixDirectInputFps
	DirectInputFps_HID1,
	DirectInputFps_HID2,

	// HighFPSFixes
	SetVelocity,
	ProcessBreakableConstraint,
	ProcessBallSocketConstraint,
	ProcessLimitedHingeConstraint,
	BuildJacobianRow,
	ProcessTwistLimitConstraint,
	ProcessConeLimitConstraint,

	// OptimizeSaveSpeed
	FileWrite,
	FileOpen,
	FileSeek,
	FileSeekEnd,
	FileTell,
	FileClose,

	// FixNvidiaShadowCorruption
	LoadWorldShadows,
	VertexBufferPool,

	// FixAspectRatioBlur
	GetShaderFile,

	// FastVRAMDetection
	GetVRAM_1,
	GetVRAM_2,

	// FixScriptedAnimationCrash
	GetSocketTransform,

	// FixKeyboardInputLanguage
	GetDeviceObjectName,
	GetDeviceObjectDesc,

	// FixWindow
	DisableNoWinKey,

	// SSAAScale
	SSAALinearFilter,
	CreateRenderTarget,

	// ReducedMipMapBias
	MipMapBias,
	LoadWorld,
	DestroyTextureWrapper,
	CreateTextureWrapper,

	// ClientHook
	LoadGameDLL,

	// SkipIntro
	CreateVideoTexture,

	// ConsoleVariableHook
	SetConsoleVariableFloat,

	// AutoResolution
	ScreenWidth,
	ScreenHeight,

	// MainLoop
	MainGameLoop,

	// DynamicVsync
	InitializePresentationParameters,

	// DisablePunkBuster
	PunkBusterRet,
	PunkBusterZero,

	// ForceRenderMode
	SetRenderMode,

	// DisableJoystick
	DisableJoystickInit,

	// DeviceCreationHook
	CreateAndInitializeDevice,

	// ConsoleEnabled
	Console_Init,
	Console_CursorLock,
	Console_CvarListHead,
	Console_CvarArrayStart,
	Console_CvarArrayEnd,
	Console_CmdArrayStart,
	Console_CmdArrayEnd,
	Console_VtableFloat,
	Console_VtableInt,
	Console_EndScene,
	Console_ResetDevice,
	Console_WindowProc,
	Console_ConsoleOutput,
	Console_SetCvarString,
	Console_SetCvarFloat,
	Console_RegClient,
	Console_RegServer,
	Console_UnregClient,
	Console_UnregServer,
	Console_RunCmd,
	Console_DebugLevel,

	Count
};

namespace
{
	constexpr uintptr_t kAddressTable[static_cast<size_t>(Addr::Count)][4] =
	{
		// FixDirectInputFps
		/* DirectInputFps_HID1              */ {  0x840DD,  0x841FD,  0xB895D,  0xB99AD },
		/* DirectInputFps_HID2              */ {  0x84057,  0x84177,  0xB88D7,  0xB9927 },

		// HighFPSFixes
		/* SetVelocity                      */ {   0x7D70,   0x7D70,   0xDDC0,   0xDDF0 },
		/* ProcessBreakableConstraint       */ {  0x4A050,  0x4A170,  0x66D80,  0x679F0 },
		/* ProcessBallSocketConstraint      */ {  0x98390,  0x984B0,  0xDC4B0,  0xDD520 },
		/* ProcessLimitedHingeConstraint    */ {  0x99050,  0x99170,  0xDD4C0,  0xDE530 },
		/* BuildJacobianRow                 */ {  0xA8D30,  0xA8E50,  0xFF710, 0x100780 },
		/* ProcessTwistLimitConstraint      */ {  0xA9940,  0xA9A60, 0x100320, 0x101390 },
		/* ProcessConeLimitConstraint       */ {  0xA9F00,  0xAA020, 0x1008E0, 0x101950 },

		// OptimizeSaveSpeed
		/* FileWrite                        */ {  0x1C670,  0x1C790,  0x29720,  0x29900 },
		/* FileOpen                         */ {  0x1C810,  0x1C930,  0x298F0,  0x29AD0 },
		/* FileSeek                         */ {  0x1C6B0,  0x1C7D0,  0x29760,  0x29940 },
		/* FileSeekEnd                      */ {  0x1C6E0,  0x1C800,  0x29790,  0x29970 },
		/* FileTell                         */ {  0x1C700,  0x1C820,  0x297B0,  0x29990 },
		/* FileClose                        */ {  0x1C7A0,  0x1C8C0,  0x29850,  0x29A30 },

		// FixNvidiaShadowCorruption
		/* LoadWorldShadows                 */ {  0xF7AC0,  0xF7BE0, 0x18D930, 0x18EF30 },
		/* VertexBufferPool                 */ { 0x112234, 0x112354, 0x1B5A24, 0x1B6F94 },

		// FixAspectRatioBlur
		/* GetShaderFile                    */ { 0x111700, 0x111820, 0x1B4770, 0x1B5DF0 },

		// FastVRAMDetection
		/* GetVRAM_1                        */ {  0x8E950,  0x8EA70,  0xC70A0,  0xC80F0 },
		/* GetVRAM_2                        */ {      0x0,      0x0, 0x18DC60, 0x18F260 },

		// FixScriptedAnimationCrash
		/* GetSocketTransform               */ {  0x36F10,      0x0,      0x0,      0x0 },

		// FixKeyboardInputLanguage
		/* GetDeviceObjectName              */ {  0x815A0,  0x816C0,  0xB52D0,  0xB6350 },
		/* GetDeviceObjectDesc              */ {  0x81E10,  0x81F30,  0xB5DE0,  0xB6E10 },

		// FixWindow
		/* DisableNoWinKey                  */ {  0x81B4B,  0x81C6B,  0xB58CB,  0xB68FB },

		// SSAAScale
		/* SSAALinearFilter                 */ {  0xF4C28,  0xF4D48, 0x18A108, 0x18B638 },
		/* CreateRenderTarget               */ {  0xF3F10,  0xF4030, 0x189350, 0x18A880 },

		// ReducedMipMapBias
		/* MipMapBias                       */ { 0x16D5C4, 0x16D5C4, 0x212B94, 0x212B94 },
		/* LoadWorld                        */ {  0x60D50,  0x60E70,  0x84E10,  0x85B70 },
		/* DestroyTextureWrapper            */ { 0x10A1A0, 0x10A2C0, 0x1A7DD0, 0x1A94A0 },
		/* CreateTextureWrapper             */ { 0x10B790, 0x10B8B0, 0x1AA040, 0x1AB700 },

		// ClientHook
		/* LoadGameDLL                      */ {  0x7D730,  0x7D850,  0xAF260,  0xB02C0 },

		// SkipIntro
		/* CreateVideoTexture               */ {  0xF5990,  0xF5AB0, 0x18AFC0, 0x18C4F0 },

		// ConsoleVariableHook
		/* SetConsoleVariableFloat          */ {   0x9360,   0x9360,  0x10120,  0x10360 },

		// AutoResolution
		/* ScreenWidth                      */ { 0x16ABF4, 0x16ABF4, 0x20EC2C, 0x210C2C },
		/* ScreenHeight                     */ { 0x16ABF8, 0x16ABF8, 0x20EC30, 0x210C30 },

		// MainLoop
		/* MainGameLoop                     */ {   0xFB20,   0xFC30,  0x19100,  0x192B0 },

		// DynamicVsync
		/* InitializePresentationParameters */ {  0xF8B80,  0xF8CA0, 0x18F2B0, 0x1908D0 },

		// DisablePunkBuster
		/* PunkBusterRet                    */ { 0x139B60, 0x139C80, 0x17CDB0, 0x17E2E0 },
		/* PunkBusterZero                   */ { 0x16EAEC, 0x16EAEC, 0x211B34, 0x213B44 },

		// ForceRenderMode
		/* SetRenderMode                    */ {   0xA800,   0xA800,  0x11710,  0x119B0 },

		// DisableJoystick
		/* DisableJoystickInit              */ {  0x84166,  0x84286,  0xB89E6,  0xB9A36 },

		// DeviceCreationHook
		/* CreateAndInitializeDevice        */ {  0xF91E0,  0xF9300, 0x18F930, 0x190F60 },

		// ConsoleEnabled
		/* Console_Init                     */ { 0x176FF0,      0x0, 0x21BFD0, 0x21E010 },
		/* Console_CursorLock               */ { 0x16ABB4,      0x0, 0x20EBEC, 0x210BEC },
		/* Console_CvarListHead             */ { 0x1773D4,      0x0, 0x21C3B4, 0x21E414 },
		/* Console_CvarArrayStart           */ { 0x16D0F8,      0x0, 0x2126C8, 0x2146D8 },
		/* Console_CvarArrayEnd             */ { 0x16DAB8,      0x0, 0x213088, 0x215098 },
		/* Console_CmdArrayStart            */ { 0x16AC50,      0x0, 0x20EC88, 0x210C88 },
		/* Console_CmdArrayEnd              */ { 0x16B4D8,      0x0, 0x20F510, 0x211510 },
		/* Console_VtableFloat              */ { 0x15E698,      0x0, 0x1FEBF8, 0x200C48 },
		/* Console_VtableInt                */ { 0x15E6A0,      0x0, 0x1FEC00, 0x200C50 },
		/* Console_EndScene                 */ {  0xF8670,      0x0, 0x18EC30, 0x190230 },
		/* Console_ResetDevice              */ {  0xF9350,      0x0, 0x18FAA0, 0x1910D0 },
		/* Console_WindowProc               */ {  0x7D9C0,      0x0,  0xAF650,  0xB06B0 },
		/* Console_ConsoleOutput            */ {  0x16880,      0x0,  0x21510,  0x216C0 },
		/* Console_SetCvarString            */ {  0x15D10,      0x0,  0x20640,  0x207F0 },
		/* Console_SetCvarFloat             */ {  0x15D40,      0x0,  0x20670,  0x20820 },
		/* Console_RegClient                */ {   0xA220,      0x0,  0x11020,  0x112C0 },
		/* Console_RegServer                */ {  0x5C780,      0x0,  0x80390,  0x810D0 },
		/* Console_UnregClient              */ {   0xA290,      0x0,  0x11090,  0x11330 },
		/* Console_UnregServer              */ {  0x5C800,      0x0,  0x80410,  0x81150 },
		/* Console_RunCmd                   */ {   0x9320,      0x0,  0x100E0,  0x10320 },
		/* Console_DebugLevel               */ { 0x16F6C4,      0x0, 0x21370C, 0x21572C },
	};
}

uintptr_t GetAddress(Addr id)
{
	uintptr_t offset = kAddressTable[static_cast<size_t>(id)][g_State.CurrentFEARGame];
	if (offset == 0)
		return 0;

	return g_State.BaseAddress + offset;
}
