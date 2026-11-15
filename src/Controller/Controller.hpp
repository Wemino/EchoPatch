#pragma once
#include <SDL3/SDL.h>

// ==========================================================
// Types and Enums
// ==========================================================

static constexpr int MAX_COMMAND_ID = 114;

// Reserved lParam bits (25-28), never set by real key messages
static constexpr LPARAM SIMULATED_KEY_FLAG = 0x1E000000;

static constexpr wchar_t COMMAND_PROMPT_CHAR = 0xE000;
static constexpr wchar_t INPUT_STRING_CHAR = 0xE0F0;
static constexpr wchar_t BUTTON_GLYPH_CHAR = 0xE100;
static constexpr wchar_t TRIGGER_GLYPH_CHAR = BUTTON_GLYPH_CHAR + SDL_GAMEPAD_BUTTON_COUNT;

enum class GamepadStyle
{
    Xbox,
    PlayStation,
    Nintendo,
    Unknown
};

enum class GamepadGlyphs
{
    Xbox360,
    XboxOne,
    XboxSeries,
    PS3,
    PS4,
    PS5,
    SwitchPro,
    JoyCon,
    GameCube,
    SteamController,
    SteamDeck,
    Count
};

struct GamepadCapabilities
{
    GamepadStyle style = GamepadStyle::Unknown;
    GamepadGlyphs glyphs = GamepadGlyphs::Xbox360;
    bool hasTouchpad = false;
    bool hasGyro = false;
    bool hasAccel = false;
    const char* name = nullptr;
    Uint16 vendorId = 0;
    Uint16 productId = 0;
};

struct GyroState
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool isValid = false;
};

struct ButtonState
{
    bool isPressed = false;
    bool wasHandled = false;
    ULONGLONG pressStartTime = 0;
    ULONGLONG lastRepeatTime = 0;
    int commandId = -1;
};

struct RumbleState
{
    int activePriority = 0;
    ULONGLONG endTime = 0;
};

struct TouchpadConfig
{
    int currentWidth = 0;
    int currentHeight = 0;
};

struct ControllerState
{
    bool isConnected = false;
    ULONGLONG menuToGameTransitionTime = 0;
    ButtonState gameButtons[SDL_GAMEPAD_BUTTON_COUNT];
    ButtonState triggerButtons[2];
    ButtonState menuButtons[6];
    ButtonState leftShoulderState;
    ButtonState rightShoulderState;
    bool commandActive[MAX_COMMAND_ID + 1] = { false };
    bool usingControllerInput = false;
    ULONGLONG lastTouchpadInputTime = 0;
    bool touchpadCursorActive = false;
    RumbleState rumble;
};

struct ButtonPromptInfo
{
    const wchar_t* buttonName;
    bool isHoldAction;
    bool isTrigger;
    int index;
};

// ==========================================================
// Global State (extern)
// ==========================================================

extern ControllerState g_Controller;
extern TouchpadConfig g_TouchpadConfig;

// ==========================================================
// Initialization / Shutdown
// ==========================================================

bool InitializeSDLGamepad();
void ShutdownSDLGamepad();

// ==========================================================
// Accessors - Controller
// ==========================================================

SDL_Gamepad* GetGamepad();
const GamepadCapabilities& GetCapabilities();
GamepadStyle GetGamepadStyle();
bool IsControllerConnected();

// ==========================================================
// Accessors - Gyro
// ==========================================================

bool HasGyro();
bool IsGyroEnabled();
const GyroState& GetGyroState();

// ==========================================================
// Accessors - Touchpad
// ==========================================================

bool HasTouchpad();
bool IsTouchpadRecentlyUsed();

// ==========================================================
// Accessors - Rumble
// ==========================================================

bool IsRumbleEnabled();

// ==========================================================
// Configuration - Gyro
// ==========================================================

void SetGyroEnabled(bool enabled);
void SetGyroSensitivity(float sensitivity);
void SetGyroSmoothing(float smoothing);
void SetGyroInvertY(bool invert);
void SetGyroCalibrationPersistence(bool enabled);
void ResetGyroState();

// ==========================================================
// Configuration - Rumble
// ==========================================================

void SetRumbleEnabled(bool enabled);

// ==========================================================
// Processing - Gyro
// ==========================================================

void GetProcessedGyroDelta(float& outYaw, float& outPitch);

// ==========================================================
// Main Poll Function
// ==========================================================

void PollController();

// ==========================================================
// Configuration
// ==========================================================

void ConfigureButton(int buttonIndex, int commandId, int holdCommandId = -1, int holdTimeMs = 0);
void ConfigureTrigger(int triggerIndex, int commandId, int holdCommandId = -1, int holdTimeMs = 0);

// ==========================================================
// Utilities
// ==========================================================

const wchar_t* GetGamepadButtonName(int commandId, bool shortName);
ButtonPromptInfo GetGamepadButtonPromptInfo(int commandId, bool shortName);
const wchar_t* GetGamepadButtonPrompt(int commandId, bool shortName);
const wchar_t* GetGamepadInputName(bool isTrigger, int index, bool shortName);
wchar_t GetGamepadButtonGlyph(bool isTrigger, int index);
SDL_GamepadButton GetGamepadMenuButton(bool isConfirm);
void RefreshGamepadPrompts();
void SetGamepadRumble(Uint16 lowFreq, Uint16 highFreq, Uint32 durationMs, int priority = 0);
void OnKeyboardMouseInput();
bool ShouldShowControllerPrompts();