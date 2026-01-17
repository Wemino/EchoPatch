#pragma once
#include <SDL3/SDL.h>

// ==========================================================
// Types and Enums
// ==========================================================

static constexpr int MAX_COMMAND_ID = 116;

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
    bool holdTriggered = false;
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

void ConfigureGamepadMappings(int btnSouth, int btnEast, int btnWest, int btnNorth, int btnLeftStick, int btnRightStick, int btnLeftShoulder, int btnRightShoulder, int btnDpadUp, int btnDpadDown, int btnDpadLeft, int btnDpadRight, int btnBack, int btnStart, int axisLeftTrigger, int axisRightTrigger, int btnMisc1, int btnRightPaddle1, int btnLeftPaddle1, int btnRightPaddle2, int btnLeftPaddle2);
void ConfigureGamepadHoldMappings(int btnSouthHold, int btnSouthHoldTime, int btnEastHold, int btnEastHoldTime, int btnWestHold, int btnWestHoldTime, int btnNorthHold, int btnNorthHoldTime, int btnLeftStickHold, int btnLeftStickHoldTime, int btnRightStickHold, int btnRightStickHoldTime, int btnLeftShoulderHold, int btnLeftShoulderHoldTime, int btnRightShoulderHold, int btnRightShoulderHoldTime, int btnDpadUpHold, int btnDpadUpHoldTime, int btnDpadDownHold, int btnDpadDownHoldTime, int btnDpadLeftHold, int btnDpadLeftHoldTime, int btnDpadRightHold, int btnDpadRightHoldTime, int btnBackHold, int btnBackHoldTime, int btnStartHold, int btnStartHoldTime, int axisLeftTriggerHold, int axisLeftTriggerHoldTime, int axisRightTriggerHold, int axisRightTriggerHoldTime, int btnMisc1Hold, int btnMisc1HoldTime, int btnRightPaddle1Hold, int btnRightPaddle1HoldTime, int btnLeftPaddle1Hold, int btnLeftPaddle1HoldTime, int btnRightPaddle2Hold, int btnRightPaddle2HoldTime, int btnLeftPaddle2Hold, int btnLeftPaddle2HoldTime);

// ==========================================================
// Utilities
// ==========================================================

const wchar_t* GetGamepadButtonName(int commandId, bool shortName);
void SetGamepadRumble(Uint16 lowFreq, Uint16 highFreq, Uint32 durationMs);
void OnKeyboardMouseInput();
bool ShouldShowControllerPrompts();