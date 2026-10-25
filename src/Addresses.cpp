#pragma once

#include "Globals.cpp"

enum class Addr
{
	// FixDirectInputFps
	DirectInputFps_HID1,
	DirectInputFps_HID2,

	// HavokPhysicsFix
	PhysicsSimUpdate,
	PhysicsSimTerm,
	hkWorldStepDeltaTime,
	RigidBodyKeyframe,
	RigidBodySoftKeyframe,
	RigidBodyTerm,
	RigidBodySetKeyframed,
	RigidBodyApplyForce,
	RigidBodyApplyTorque,
	RigidBodyMarkTransformDirty,
	ObjectMarkAttachmentsDirty,

	// OptimizeSaveSpeed
	FileWrite,
	FileOpen,
	FileSeek,
	FileSeekEnd,
	FileTell,
	FileClose,
	FileGetSize,
	FileIsEOF,
	FileDestructor,

	// FixNvidiaShadowCorruption
	DrawMeshBatch,
	RendererDevice,
	DisablePrimitiveRendering,

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
	Console_VtableFloat,
	Console_VtableInt,
	Console_Present,
	Console_ResetDevice,
	Console_WindowProc,
	Console_Print,
	Console_ClientManager,
	Console_ServerManager,
	Console_ExecuteCommand,
	Console_DebugLevel,
	Console_GetEngineVars,
	Console_GetNumEngineVars,

	// ConsoleDebugTools
	Console_ClientInterface,
	Console_ServerInterface,
	Console_ClientCommon,
	Console_UpdateObject,

	Count
};

namespace
{
	constexpr uintptr_t kAddressTable[static_cast<size_t>(Addr::Count)][4] =
	{
		// FixDirectInputFps
		/* DirectInputFps_HID1              */ {  0x840DD,  0x841FD,  0xB895D,  0xB99AD },
		/* DirectInputFps_HID2              */ {  0x84057,  0x84177,  0xB88D7,  0xB9927 },

		// HavokPhysicsFix
		/* PhysicsSimUpdate                 */ {  0x4D100,  0x4D220,  0x6B330,  0x6BFA0 },
		/* PhysicsSimTerm                   */ {  0x4D2D0,  0x4D3F0,  0x6B600,  0x6C270 },
		/* hkWorldStepDeltaTime             */ {  0x96F40,  0x97060,  0xDA710,  0xDB780 },
		/* RigidBodyKeyframe                */ {  0x4B230,  0x4B350,  0x68ED0,  0x69B40 },
		/* RigidBodySoftKeyframe            */ {  0x3ACE0,  0x3AE00,  0x526A0,  0x53340 },
		/* RigidBodyTerm                    */ {  0x4ABA0,  0x4ACC0,  0x68840,  0x694B0 },
		/* RigidBodySetKeyframed            */ {  0x4BF10,  0x4C030,  0x69DB0,  0x6AA20 },
		/* RigidBodyApplyForce              */ {  0x4B760,  0x4B880,  0x69560,  0x6A1D0 },
		/* RigidBodyApplyTorque             */ {  0x4B980,  0x4BAA0,  0x69780,  0x6A3F0 },
		/* RigidBodyMarkTransformDirty      */ {  0x4B200,  0x4B320,  0x68EA0,  0x69B10 },
		/* ObjectMarkAttachmentsDirty       */ {  0x29220,  0x29340,  0x3B240,  0x3B420 },

		// OptimizeSaveSpeed
		/* FileWrite                        */ {  0x1C670,  0x1C790,  0x29720,  0x29900 },
		/* FileOpen                         */ {  0x1C810,  0x1C930,  0x298F0,  0x29AD0 },
		/* FileSeek                         */ {  0x1C6B0,  0x1C7D0,  0x29760,  0x29940 },
		/* FileSeekEnd                      */ {  0x1C6E0,  0x1C800,  0x29790,  0x29970 },
		/* FileTell                         */ {  0x1C700,  0x1C820,  0x297B0,  0x29990 },
		/* FileClose                        */ {  0x1C7A0,  0x1C8C0,  0x29850,  0x29A30 },
		/* FileGetSize                      */ {  0x1C750,  0x1C870,  0x29800,  0x299E0 },
		/* FileIsEOF                        */ {  0x1C850,  0x1C970,  0x29930,  0x29B10 },
		/* FileDestructor                   */ {  0x1C7D0,  0x1C8F0,  0x298B0,  0x29A90 },

		// FixNvidiaShadowCorruption
		/* DrawMeshBatch                    */ { 0x1123C0, 0x1124E0, 0x1B5BB0, 0x1B7120 },
		/* RendererDevice                   */ { 0x176FF0, 0x176FF0, 0x21BFD0, 0x21E010 },
		/* DisablePrimitiveRendering        */ { 0x16D964, 0x16D964, 0x212F34, 0x214F44 },

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
		/* Console_VtableFloat              */ { 0x15E698,      0x0, 0x1FEBF8, 0x200C48 },
		/* Console_VtableInt                */ { 0x15E6A0,      0x0, 0x1FEC00, 0x200C50 },
		/* Console_Present                  */ {  0xF8730,      0x0, 0x18ED10, 0x190310 },
		/* Console_ResetDevice              */ {  0xF9350,      0x0, 0x18FAA0, 0x1910D0 },
		/* Console_WindowProc               */ {  0x7D9C0,      0x0,  0xAF650,  0xB06B0 },
		/* Console_Print                    */ {  0x90C70,      0x0,  0xCA780,  0xCB7F0 },
		/* Console_ClientManager            */ { 0x16FB50,      0x0, 0x213BA8, 0x215BC8 },
		/* Console_ServerManager            */ { 0x172484,      0x0, 0x21654C, 0x21856C },
		/* Console_ExecuteCommand           */ {  0x16480,      0x0,  0x20E30,  0x20FE0 },
		/* Console_DebugLevel               */ { 0x16F6C4,      0x0, 0x21370C, 0x21572C },
		/* Console_GetEngineVars            */ {  0x19CF0,      0x0,  0x25C00,  0x25DD0 },
		/* Console_GetNumEngineVars         */ {  0x19D00,      0x0,  0x25C10,  0x25DE0 },

		// ConsoleDebugTools
		/* Console_ClientInterface          */ { 0x16A2A8,      0x0, 0x20E2E0, 0x2102E0 },
		/* Console_ServerInterface          */ { 0x16BCF8,      0x0, 0x20FD30, 0x211D38 },
		/* Console_ClientCommon             */ { 0x169F60,      0x0, 0x20DF98, 0x20FF98 },
		/* Console_UpdateObject             */ {  0x573E0,      0x0,  0x79900,  0x7A590 },
	};
}

uintptr_t GetAddress(Addr id)
{
	uintptr_t offset = kAddressTable[static_cast<size_t>(id)][g_State.CurrentFEARGame];
	if (offset == 0)
		return 0;

	return g_State.BaseAddress + offset;
}
