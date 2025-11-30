#pragma once

#include <SDL3/SDL.h>
#include <unordered_map>

struct ButtonState
{
    bool isPressed = false;
    bool wasHandled = false;
    ULONGLONG pressStartTime = 0;
    ULONGLONG lastRepeatTime = 0;
};

struct ControllerState
{
    bool isConnected = false;
    ULONGLONG menuToGameTransitionTime = 0;

    std::unordered_map<int, ButtonState> gameButtons;
    std::unordered_map<int, ButtonState> triggerButtons;
    ButtonState menuButtons[6];

    // ScreenPerformanceAdvanced
    ButtonState leftShoulderState;
    ButtonState rightShoulderState;

    bool commandActive[117] = { false };
};

extern ControllerState g_Controller;

struct TouchpadConfig
{
    int currentWidth = 0;
    int currentHeight = 0;
};

extern TouchpadConfig g_TouchpadConfig;

enum class GamepadStyle
{
    Xbox,
    PlayStation,
    Nintendo,
    Unknown
};

const wchar_t* GetGamepadButtonName(int commandId, bool shortName);

SDL_Gamepad* GetGamepad();

bool InitializeSDLGamepad();
void ShutdownSDLGamepad();
void PollController();
void SetGamepadRumble(Uint16 lowFreq, Uint16 highFreq, Uint32 durationMs);
void ConfigureGamepadMappings(int btnA, int btnB, int btnX, int btnY, int btnLeftStick, int btnRightStick, int btnLeftShoulder, int btnRightShoulder, int btnDpadUp, int btnDpadDown, int btnDpadLeft, int btnDpadRight, int btnBack, int axisLeftTrigger, int axisRightTrigger);
const wchar_t* GetGamepadButtonName(int commandId, bool shortName);
GamepadStyle GetGamepadStyle();