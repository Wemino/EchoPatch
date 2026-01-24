#pragma once
#include <SDL3/SDL.h>

// ==========================================================
// Types and Enums
// ==========================================================

static constexpr int MAX_COMMAND_ID = 114;

enum class GamepadStyle
{
    Xbox,
    PlayStation,
    Nintendo,
    Unknown
};

struct GamepadCapabilities
{
    GamepadStyle style = GamepadStyle::Unknown;
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
    int simulatedKeyPressCount = 0;
};

struct ButtonPromptInfo
{
    const wchar_t* buttonName;
    bool isHoldAction;
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
void SetGamepadRumble(Uint16 lowFreq, Uint16 highFreq, Uint32 durationMs);
void OnKeyboardMouseInput();
bool ShouldShowControllerPrompts();