#include <Windows.h>
#include <SDL3/SDL.h>

#include "../Core/Core.hpp"
#include "Controller.hpp"
#include "../Client/Client.hpp"

// ==========================================================
// Global State
// ==========================================================

ControllerState g_Controller;
TouchpadConfig g_TouchpadConfig;

// ==========================================================
// Static State
// ==========================================================

static SDL_Gamepad* s_pGamepad = nullptr;
static GamepadCapabilities s_capabilities;
static GyroState s_gyroState;

// ==========================================================
// Button Mappings
// ==========================================================

static std::pair<SDL_GamepadButton, int> s_buttonMappings[] =
{
    { SDL_GAMEPAD_BUTTON_SOUTH,          15 },   // Jump (A)
    { SDL_GAMEPAD_BUTTON_EAST,           14 },   // Crouch (B)
    { SDL_GAMEPAD_BUTTON_WEST,           88 },   // Reload/Activate (X)
    { SDL_GAMEPAD_BUTTON_NORTH,          106 },  // SlowMo (Y)
    { SDL_GAMEPAD_BUTTON_LEFT_STICK,     70 },   // Medkit
    { SDL_GAMEPAD_BUTTON_RIGHT_STICK,    114 },  // Flashlight
    { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,  81 },   // Throw Grenade
    { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 19 },   // Melee
    { SDL_GAMEPAD_BUTTON_DPAD_UP,        77 },   // Next Weapon
    { SDL_GAMEPAD_BUTTON_DPAD_DOWN,      73 },   // Next Grenade
    { SDL_GAMEPAD_BUTTON_DPAD_LEFT,      20 },   // Lean Left
    { SDL_GAMEPAD_BUTTON_DPAD_RIGHT,     21 },   // Lean Right
    { SDL_GAMEPAD_BUTTON_BACK,           78 },   // Mission Status
    { SDL_GAMEPAD_BUTTON_START,          -1 },   // Menu (Escape)
};

static std::pair<SDL_GamepadAxis, int> s_triggerMappings[] =
{
    { SDL_GAMEPAD_AXIS_LEFT_TRIGGER,  71 },      // Aim
    { SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 17 },      // Fire
};

// ==========================================================
// Constants
// ==========================================================

static constexpr Sint16 STICK_DEADZONE = 16384;
static constexpr Sint16 TRIGGER_THRESHOLD = 8000;
static constexpr ULONGLONG MENU_REPEAT_DELAY = 500;
static constexpr ULONGLONG MENU_REPEAT_RATE = 100;
static constexpr ULONGLONG TRANSITION_GRACE_PERIOD = 200;

// ==========================================================
// Touchpad State
// ==========================================================

struct TouchpadState
{
    bool wasDown = false;
    float lastX = 0.0f;
    float lastY = 0.0f;
};

static TouchpadState s_touchpadFinger;

// ==========================================================
// Controller Database
// ==========================================================

static bool LoadGamepadMappings()
{
    const char* paths[] =
    {
        "gamecontrollerdb.txt",
        "../gamecontrollerdb.txt",
    };

    for (const char* path : paths)
    {
        int result = SDL_AddGamepadMappingsFromFile(path);
        if (result >= 0)
        {
            return true;
        }
    }

    return false;
}

// ==========================================================
// Capability Detection
// ==========================================================

static GamepadStyle DetectGamepadStyle(SDL_Gamepad* pGamepad)
{
    if (!pGamepad)
        return GamepadStyle::Unknown;

    SDL_GamepadType type = SDL_GetGamepadType(pGamepad);

    switch (type)
    {
        case SDL_GAMEPAD_TYPE_XBOX360:
        case SDL_GAMEPAD_TYPE_XBOXONE:
            return GamepadStyle::Xbox;

        case SDL_GAMEPAD_TYPE_PS3:
        case SDL_GAMEPAD_TYPE_PS4:
        case SDL_GAMEPAD_TYPE_PS5:
            return GamepadStyle::PlayStation;

        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            return GamepadStyle::Nintendo;

        default:
            return GamepadStyle::Xbox;
    }
}

static bool DetectTouchpadSupport(SDL_Gamepad* pGamepad)
{
    if (!pGamepad)
        return false;

    return SDL_GetNumGamepadTouchpads(pGamepad) > 0;
}

static bool DetectGyroSupport(SDL_Gamepad* pGamepad)
{
    if (!pGamepad)
        return false;

    return SDL_GamepadHasSensor(pGamepad, SDL_SENSOR_GYRO);
}

static void LoadGamepadCapabilities(SDL_Gamepad* pGamepad)
{
    s_capabilities = GamepadCapabilities();

    if (!pGamepad)
        return;

    s_capabilities.style = DetectGamepadStyle(pGamepad);
    s_capabilities.hasTouchpad = DetectTouchpadSupport(pGamepad);
    s_capabilities.hasGyro = DetectGyroSupport(pGamepad);
    s_capabilities.name = SDL_GetGamepadName(pGamepad);
    s_capabilities.vendorId = SDL_GetGamepadVendor(pGamepad);
    s_capabilities.productId = SDL_GetGamepadProduct(pGamepad);

    // Enable gyro sensor if supported
    if (s_capabilities.hasGyro)
    {
        SDL_SetGamepadSensorEnabled(pGamepad, SDL_SENSOR_GYRO, true);
    }
}

// ==========================================================
// Initialization / Shutdown
// ==========================================================

bool InitializeSDLGamepad()
{
    if (!SDL_Init(SDL_INIT_GAMEPAD))
    {
        return false;
    }

    LoadGamepadMappings();

    return true;
}

void ShutdownSDLGamepad()
{
    if (s_pGamepad)
    {
        SDL_CloseGamepad(s_pGamepad);
        s_pGamepad = nullptr;
    }

    s_capabilities = GamepadCapabilities();
    s_gyroState = GyroState();

    SDL_Quit();
}

// ==========================================================
// Accessors
// ==========================================================

SDL_Gamepad* GetGamepad()
{
    return s_pGamepad;
}

const GamepadCapabilities& GetCapabilities()
{
    return s_capabilities;
}

GamepadStyle GetGamepadStyle()
{
    return s_capabilities.style;
}

bool HasTouchpad()
{
    return s_capabilities.hasTouchpad;
}

bool HasGyro()
{
    return s_capabilities.hasGyro;
}

const GyroState& GetGyroState()
{
    return s_gyroState;
}

bool IsControllerConnected()
{
    return s_pGamepad && SDL_GamepadConnected(s_pGamepad);
}

// ==========================================================
// Gyro Processing
// ==========================================================

void ProcessGyro()
{
    s_gyroState.isValid = false;

    if (!s_pGamepad || !s_capabilities.hasGyro)
        return;

    float data[3] = { 0.0f, 0.0f, 0.0f };

    if (SDL_GetGamepadSensorData(s_pGamepad, SDL_SENSOR_GYRO, data, 3))
    {
        s_gyroState.x = data[0];
        s_gyroState.y = data[1];
        s_gyroState.z = data[2];
        s_gyroState.isValid = true;
    }
}

// ==========================================================
// Internal Helpers
// ==========================================================

static inline bool IsInTransitionPeriod()
{
    if (g_Controller.menuToGameTransitionTime == 0)
        return false;

    return (GetTickCount64() - g_Controller.menuToGameTransitionTime) < TRANSITION_GRACE_PERIOD;
}

static void NotifyHUDConnectionChange()
{
    HUDWeaponListUpdateTriggerNames(g_State.CHUDWeaponList);
    HUDGrenadeListUpdateTriggerNames(g_State.CHUDGrenadeList);
    HUDSwapUpdateTriggerName();
}

static void ClearMenuButtonStates()
{
    for (int i = 0; i < 6; i++)
    {
        g_Controller.menuButtons[i] = {};
    }
}

static void ReleaseAllGameButtons()
{
    for (const auto& [button, commandId] : s_buttonMappings)
    {
        auto& btnState = g_Controller.gameButtons[button];
        if (!btnState.isPressed)
            continue;

        if (commandId == -1)
        {
            PostMessage(g_State.hWnd, WM_KEYUP, VK_ESCAPE, 0);
        }
        else
        {
            g_Controller.commandActive[commandId] = false;
            OnCommandOff(g_State.g_pGameClientShell, commandId);
        }
        btnState = {};
    }

    for (const auto& [axis, commandId] : s_triggerMappings)
    {
        auto& btnState = g_Controller.triggerButtons[axis];
        if (!btnState.isPressed)
            continue;

        g_Controller.commandActive[commandId] = false;
        OnCommandOff(g_State.g_pGameClientShell, commandId);
        btnState = {};
    }
}

static inline bool IsStickDirectionPressed(int direction)
{
    Sint16 lx = SDL_GetGamepadAxis(s_pGamepad, SDL_GAMEPAD_AXIS_LEFTX);
    Sint16 ly = SDL_GetGamepadAxis(s_pGamepad, SDL_GAMEPAD_AXIS_LEFTY);
    Sint16 rx = SDL_GetGamepadAxis(s_pGamepad, SDL_GAMEPAD_AXIS_RIGHTX);
    Sint16 ry = SDL_GetGamepadAxis(s_pGamepad, SDL_GAMEPAD_AXIS_RIGHTY);

    switch (direction)
    {
        case 0: return (ly < -STICK_DEADZONE) || (ry < -STICK_DEADZONE); // Up
        case 1: return (ly > STICK_DEADZONE) || (ry > STICK_DEADZONE);  // Down
        case 2: return (lx < -STICK_DEADZONE) || (rx < -STICK_DEADZONE); // Left
        case 3: return (lx > STICK_DEADZONE) || (rx > STICK_DEADZONE);  // Right
        default: return false;
    }
}

// ==========================================================
// Configuration
// ==========================================================

void ConfigureGamepadMappings(int btnA, int btnB, int btnX, int btnY, int btnLeftStick, int btnRightStick, int btnLeftShoulder, int btnRightShoulder, int btnDpadUp, int btnDpadDown, int btnDpadLeft, int btnDpadRight, int btnBack, int axisLeftTrigger, int axisRightTrigger)
{
    for (auto& [button, commandId] : s_buttonMappings)
    {
        switch (button)
        {
            case SDL_GAMEPAD_BUTTON_SOUTH:          commandId = btnA; break;
            case SDL_GAMEPAD_BUTTON_EAST:           commandId = btnB; break;
            case SDL_GAMEPAD_BUTTON_WEST:           commandId = btnX; break;
            case SDL_GAMEPAD_BUTTON_NORTH:          commandId = btnY; break;
            case SDL_GAMEPAD_BUTTON_LEFT_STICK:     commandId = btnLeftStick; break;
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK:    commandId = btnRightStick; break;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  commandId = btnLeftShoulder; break;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: commandId = btnRightShoulder; break;
            case SDL_GAMEPAD_BUTTON_DPAD_UP:        commandId = btnDpadUp; break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN:      commandId = btnDpadDown; break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT:      commandId = btnDpadLeft; break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:     commandId = btnDpadRight; break;
            case SDL_GAMEPAD_BUTTON_BACK:           commandId = btnBack; break;
            default: break;
        }
    }

    for (auto& [axis, commandId] : s_triggerMappings)
    {
        switch (axis)
        {
            case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:  commandId = axisLeftTrigger; break;
            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: commandId = axisRightTrigger; break;
            default: break;
        }
    }
}

// ==========================================================
// Button Names
// ==========================================================

struct ButtonNames
{
    const wchar_t* shortName;
    const wchar_t* longName;
};

struct ButtonNameSet
{
    ButtonNames south;
    ButtonNames east;
    ButtonNames west;
    ButtonNames north;
    ButtonNames leftShoulder;
    ButtonNames rightShoulder;
    ButtonNames leftTrigger;
    ButtonNames rightTrigger;
    ButtonNames leftStick;
    ButtonNames rightStick;
    ButtonNames back;
    ButtonNames start;
    ButtonNames dpadUp;
    ButtonNames dpadDown;
    ButtonNames dpadLeft;
    ButtonNames dpadRight;
};

static const ButtonNameSet s_xboxNames =
{
    { L"A",      L"A Button" },
    { L"B",      L"B Button" },
    { L"X",      L"X Button" },
    { L"Y",      L"Y Button" },
    { L"LB",     L"Left Bumper" },
    { L"RB",     L"Right Bumper" },
    { L"LT",     L"Left Trigger" },
    { L"RT",     L"Right Trigger" },
    { L"LS",     L"Left Stick" },
    { L"RS",     L"Right Stick" },
    { L"Back",   L"Back Button" },
    { L"Start",  L"Start Button" },
    { L"D-Up",   L"D-Pad Up" },
    { L"D-Down", L"D-Pad Down" },
    { L"D-Left", L"D-Pad Left" },
    { L"D-Right",L"D-Pad Right" },
};

static const ButtonNameSet s_playstationNames =
{
    { L"Cross",    L"Cross Button" },
    { L"Circle",   L"Circle Button" },
    { L"Square",   L"Square Button" },
    { L"Triangle", L"Triangle Button" },
    { L"L1",       L"L1 Button" },
    { L"R1",       L"R1 Button" },
    { L"L2",       L"L2 Trigger" },
    { L"R2",       L"R2 Trigger" },
    { L"L3",       L"L3 Button" },
    { L"R3",       L"R3 Button" },
    { L"Share",    L"Share Button" },
    { L"Options",  L"Options Button" },
    { L"D-Up",     L"D-Pad Up" },
    { L"D-Down",   L"D-Pad Down" },
    { L"D-Left",   L"D-Pad Left" },
    { L"D-Right",  L"D-Pad Right" },
};

static const ButtonNameSet s_nintendoNames =
{
    { L"B",      L"B Button" },
    { L"A",      L"A Button" },
    { L"Y",      L"Y Button" },
    { L"X",      L"X Button" },
    { L"L",      L"L Button" },
    { L"R",      L"R Button" },
    { L"ZL",     L"ZL Trigger" },
    { L"ZR",     L"ZR Trigger" },
    { L"LS",     L"Left Stick" },
    { L"RS",     L"Right Stick" },
    { L"-",      L"- Button" },
    { L"+",      L"+ Button" },
    { L"D-Up",   L"D-Pad Up" },
    { L"D-Down", L"D-Pad Down" },
    { L"D-Left", L"D-Pad Left" },
    { L"D-Right",L"D-Pad Right" },
};

static const ButtonNameSet& GetButtonNameSet()
{
    switch (s_capabilities.style)
    {
        case GamepadStyle::PlayStation:
            return s_playstationNames;
        case GamepadStyle::Nintendo:
            return s_nintendoNames;
        case GamepadStyle::Xbox:
        default:
            return s_xboxNames;
    }
}

static const wchar_t* GetButtonNameForButton(SDL_GamepadButton button, bool shortName)
{
    const ButtonNameSet& names = GetButtonNameSet();

    switch (button)
    {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            return shortName ? names.south.shortName : names.south.longName;
        case SDL_GAMEPAD_BUTTON_EAST:
            return shortName ? names.east.shortName : names.east.longName;
        case SDL_GAMEPAD_BUTTON_WEST:
            return shortName ? names.west.shortName : names.west.longName;
        case SDL_GAMEPAD_BUTTON_NORTH:
            return shortName ? names.north.shortName : names.north.longName;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            return shortName ? names.leftShoulder.shortName : names.leftShoulder.longName;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return shortName ? names.rightShoulder.shortName : names.rightShoulder.longName;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
            return shortName ? names.leftStick.shortName : names.leftStick.longName;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            return shortName ? names.rightStick.shortName : names.rightStick.longName;
        case SDL_GAMEPAD_BUTTON_BACK:
            return shortName ? names.back.shortName : names.back.longName;
        case SDL_GAMEPAD_BUTTON_START:
            return shortName ? names.start.shortName : names.start.longName;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            return shortName ? names.dpadUp.shortName : names.dpadUp.longName;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            return shortName ? names.dpadDown.shortName : names.dpadDown.longName;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            return shortName ? names.dpadLeft.shortName : names.dpadLeft.longName;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            return shortName ? names.dpadRight.shortName : names.dpadRight.longName;
        default:
            return nullptr;
    }
}

static const wchar_t* GetButtonNameForAxis(SDL_GamepadAxis axis, bool shortName)
{
    const ButtonNameSet& names = GetButtonNameSet();

    switch (axis)
    {
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
            return shortName ? names.leftTrigger.shortName : names.leftTrigger.longName;
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
            return shortName ? names.rightTrigger.shortName : names.rightTrigger.longName;
        default:
            return nullptr;
    }
}

const wchar_t* GetGamepadButtonName(int commandId, bool shortName)
{
    for (const auto& [button, cmdId] : s_buttonMappings)
    {
        if (cmdId == commandId)
        {
            const wchar_t* name = GetButtonNameForButton(button, shortName);
            if (name)
                return name;
        }
    }

    for (const auto& [axis, cmdId] : s_triggerMappings)
    {
        if (cmdId == commandId)
        {
            const wchar_t* name = GetButtonNameForAxis(axis, shortName);
            if (name)
                return name;
        }
    }

    return nullptr;
}

// ==========================================================
// Event Processing
// ==========================================================

static void OnGamepadConnected(SDL_JoystickID deviceId)
{
    if (s_pGamepad)
        return;

    s_pGamepad = SDL_OpenGamepad(deviceId);
    if (s_pGamepad)
    {
        LoadGamepadCapabilities(s_pGamepad);
    }
}

static void OnGamepadDisconnected(SDL_JoystickID deviceId)
{
    if (!s_pGamepad)
        return;

    if (deviceId == SDL_GetGamepadID(s_pGamepad))
    {
        SDL_CloseGamepad(s_pGamepad);
        s_pGamepad = nullptr;
        s_capabilities = GamepadCapabilities();
        s_gyroState = GyroState();
        s_touchpadFinger = TouchpadState();
    }
}

static void ProcessSDLEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_GAMEPAD_ADDED:
                OnGamepadConnected(event.gdevice.which);
                break;

            case SDL_EVENT_GAMEPAD_REMOVED:
                OnGamepadDisconnected(event.gdevice.which);
                break;
        }
    }
}

// ==========================================================
// Touchpad Input
// ==========================================================

static void ProcessTouchpadMouse()
{
    if (!s_pGamepad || !s_capabilities.hasTouchpad || !TouchpadEnabled)
        return;

    bool fingerDown = false;
    float x = 0.0f, y = 0.0f, pressure = 0.0f;

    if (SDL_GetGamepadTouchpadFinger(s_pGamepad, 0, 0, &fingerDown, &x, &y, &pressure))
    {
        if (fingerDown)
        {
            if (s_touchpadFinger.wasDown)
            {
                float deltaX = (x - s_touchpadFinger.lastX) * g_TouchpadConfig.currentWidth;
                float deltaY = (y - s_touchpadFinger.lastY) * g_TouchpadConfig.currentHeight;

                if (deltaX != 0.0f || deltaY != 0.0f)
                {
                    INPUT input = {};
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_MOVE;
                    input.mi.dx = static_cast<LONG>(deltaX);
                    input.mi.dy = static_cast<LONG>(deltaY);
                    SendInput(1, &input, sizeof(INPUT));
                }
            }

            s_touchpadFinger.lastX = x;
            s_touchpadFinger.lastY = y;
            s_touchpadFinger.wasDown = true;
        }
        else
        {
            s_touchpadFinger.wasDown = false;
        }
    }
}

static void ProcessTouchpadClick()
{
    if (!s_pGamepad || !s_capabilities.hasTouchpad || !TouchpadClickEnabled)
        return;

    static bool s_wasTouchpadPressed = false;
    bool isPressed = SDL_GetGamepadButton(s_pGamepad, SDL_GAMEPAD_BUTTON_TOUCHPAD);

    if (isPressed && !s_wasTouchpadPressed)
    {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
    }
    else if (!isPressed && s_wasTouchpadPressed)
    {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    s_wasTouchpadPressed = isPressed;
}

// ==========================================================
// Gameplay Input Handlers
// ==========================================================

static void HandleGamepadButton(SDL_GamepadButton button, int commandId)
{
    auto& btnState = g_Controller.gameButtons[button];

    // Activate instead of Reload
    if (commandId == 88 && (g_State.canActivate || g_State.canSwap || g_State.isOperatingTurret))
    {
        commandId = 87;
    }

    bool isPressed = SDL_GetGamepadButton(s_pGamepad, button);

    if (IsInTransitionPeriod())
    {
        btnState.isPressed = isPressed;
        return;
    }

    // Button just pressed
    if (isPressed && !btnState.isPressed)
    {
        if (commandId == -1)
        {
            PostMessage(g_State.hWnd, WM_KEYDOWN, VK_ESCAPE, 0);
        }
        else
        {
            g_Controller.commandActive[commandId] = true;
            OnCommandOn(g_State.g_pGameClientShell, commandId);
            btnState.wasHandled = true;
            btnState.pressStartTime = GetTickCount64();
        }
    }
    // Button held
    else if (isPressed)
    {
        if (commandId != -1)
        {
            g_Controller.commandActive[commandId] = true;
        }
    }
    // Button just released
    else if (!isPressed && btnState.isPressed)
    {
        if (commandId == -1)
        {
            PostMessage(g_State.hWnd, WM_KEYUP, VK_ESCAPE, 0);
        }
        else
        {
            g_Controller.commandActive[commandId] = false;
            OnCommandOff(g_State.g_pGameClientShell, commandId);
            btnState.wasHandled = false;
        }
    }

    btnState.isPressed = isPressed;
}

static void HandleGamepadTrigger(SDL_GamepadAxis axis, int commandId)
{
    auto& btnState = g_Controller.triggerButtons[axis];

    Sint16 value = SDL_GetGamepadAxis(s_pGamepad, axis);
    bool isPressed = value > TRIGGER_THRESHOLD;

    if (IsInTransitionPeriod())
    {
        btnState.isPressed = isPressed;
        return;
    }

    if (isPressed && !btnState.isPressed)
    {
        g_Controller.commandActive[commandId] = true;
        OnCommandOn(g_State.g_pGameClientShell, commandId);
        btnState.wasHandled = true;
        btnState.pressStartTime = GetTickCount64();
    }
    else if (isPressed)
    {
        g_Controller.commandActive[commandId] = true;
    }
    else if (!isPressed && btnState.isPressed)
    {
        g_Controller.commandActive[commandId] = false;
        OnCommandOff(g_State.g_pGameClientShell, commandId);
        btnState.wasHandled = false;
    }

    btnState.isPressed = isPressed;
}

static void ProcessGameplayInput()
{
    memset(g_Controller.commandActive, 0, sizeof(g_Controller.commandActive));

    for (const auto& [button, commandId] : s_buttonMappings)
    {
        HandleGamepadButton(button, commandId);
    }

    for (const auto& [axis, commandId] : s_triggerMappings)
    {
        HandleGamepadTrigger(axis, commandId);
    }
}

// ==========================================================
// Menu Navigation
// ==========================================================

static void ProcessMenuNavigation()
{
    const ULONGLONG currentTime = GetTickCount64();

    SDL_GamepadButton confirmButton = SDL_GAMEPAD_BUTTON_SOUTH;
    SDL_GamepadButton cancelButton = SDL_GAMEPAD_BUTTON_EAST;

    if (s_capabilities.style == GamepadStyle::Nintendo)
    {
        confirmButton = SDL_GAMEPAD_BUTTON_EAST;
        cancelButton = SDL_GAMEPAD_BUTTON_SOUTH;
    }

    const struct { SDL_GamepadButton button; int vkey; } menuNavigation[] =
    {
        { SDL_GAMEPAD_BUTTON_DPAD_UP,    VK_UP },
        { SDL_GAMEPAD_BUTTON_DPAD_DOWN,  VK_DOWN },
        { SDL_GAMEPAD_BUTTON_DPAD_LEFT,  VK_LEFT },
        { SDL_GAMEPAD_BUTTON_DPAD_RIGHT, VK_RIGHT },
        { confirmButton,                 VK_RETURN },
        { cancelButton,                  VK_ESCAPE },
    };

    for (int i = 0; i < 6; i++)
    {
        auto& btnState = g_Controller.menuButtons[i];
        bool pressed = false;

        if (i < 4) // D-Pad directions
        {
            bool dpadPressed = SDL_GetGamepadButton(s_pGamepad, menuNavigation[i].button);
            bool stickPressed = IsStickDirectionPressed(i);
            pressed = dpadPressed || stickPressed;

            if (pressed && btnState.isPressed)
            {
                ULONGLONG elapsed = currentTime - btnState.pressStartTime;
                ULONGLONG sinceLast = currentTime - btnState.lastRepeatTime;

                if (elapsed > MENU_REPEAT_DELAY && sinceLast > MENU_REPEAT_RATE)
                {
                    PostMessage(g_State.hWnd, WM_KEYDOWN, menuNavigation[i].vkey, 0);
                    btnState.lastRepeatTime = currentTime;
                }
            }
        }
        else // Confirm/Cancel buttons
        {
            pressed = SDL_GetGamepadButton(s_pGamepad, menuNavigation[i].button);
        }

        if (pressed != btnState.isPressed)
        {
            PostMessage(g_State.hWnd, pressed ? WM_KEYDOWN : WM_KEYUP, menuNavigation[i].vkey, 0);
            btnState.isPressed = pressed;

            if (pressed)
            {
                btnState.pressStartTime = currentTime;
                btnState.lastRepeatTime = currentTime;
            }
            else
            {
                btnState.pressStartTime = 0;
                btnState.lastRepeatTime = 0;
            }
        }
    }

    // Shoulder buttons for CPU/GPU settings navigation
    auto HandleShoulderSetting = [](SDL_GamepadButton button, ButtonState& state, int direction)
    {
        bool pressed = SDL_GetGamepadButton(s_pGamepad, button);
        if (pressed != state.isPressed)
        {
            if (pressed && g_State.pCurrentType != 0 && g_State.maxCurrentType != -1)
            {
                g_State.currentType = (g_State.currentType + direction + g_State.maxCurrentType) % g_State.maxCurrentType;
                SetCurrentType(g_State.pCurrentType, g_State.currentType);
            }
            state.isPressed = pressed;
        }
    };

    HandleShoulderSetting(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, g_Controller.leftShoulderState, -1);
    HandleShoulderSetting(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, g_Controller.rightShoulderState, 1);
}

// ==========================================================
// Main Poll Function
// ==========================================================

void PollController()
{
    ProcessSDLEvents();

    // Update connection state
    bool wasConnected = g_Controller.isConnected;
    g_Controller.isConnected = IsControllerConnected();

    if (wasConnected != g_Controller.isConnected)
    {
        NotifyHUDConnectionChange();
    }

    static bool s_wasPlaying = false;
    static bool s_wasMsgBoxVisible = false;

    // Gameplay -> Menu transition
    if (s_wasPlaying && !g_State.isPlaying && g_Controller.isConnected)
    {
        ReleaseAllGameButtons();
    }

    // Menu -> Gameplay transition
    if ((!s_wasPlaying && g_State.isPlaying) || (s_wasMsgBoxVisible && !g_State.isMsgBoxVisible))
    {
        if (g_Controller.isConnected)
        {
            g_Controller.menuToGameTransitionTime = GetTickCount64();
            ClearMenuButtonStates();
        }
    }

    s_wasPlaying = g_State.isPlaying;
    s_wasMsgBoxVisible = g_State.isMsgBoxVisible;

    if (!g_Controller.isConnected)
    {
        memset(g_Controller.commandActive, 0, sizeof(g_Controller.commandActive));
        return;
    }

    ProcessGyro();
    ProcessTouchpadMouse();
    ProcessTouchpadClick();

    if (!g_State.isPlaying || g_State.isMsgBoxVisible)
    {
        ProcessMenuNavigation();
    }
    else
    {
        ProcessGameplayInput();
    }
}

// ==========================================================
// Utilities
// ==========================================================

void SetGamepadRumble(Uint16 lowFreq, Uint16 highFreq, Uint32 durationMs)
{
    if (s_pGamepad)
    {
        SDL_RumbleGamepad(s_pGamepad, lowFreq, highFreq, durationMs);
    }
}