#include <Windows.h>

#include "../Core/Core.hpp"
#include "Controller.hpp"
#include "../Client/Client.hpp"

ControllerState g_Controller;

std::pair<WORD, int> g_buttonMappings[] =
{
    { XINPUT_GAMEPAD_A, 15 }, // Jump
    { XINPUT_GAMEPAD_B, 14 }, // Crouch
    { XINPUT_GAMEPAD_X, 88 }, // Reload/Activate
    { XINPUT_GAMEPAD_Y, 106 }, // SlowMo
    { XINPUT_GAMEPAD_LEFT_THUMB, 70 }, // Medkit
    { XINPUT_GAMEPAD_RIGHT_THUMB, 114 }, // Flashlight
    { XINPUT_GAMEPAD_LEFT_SHOULDER, 81 }, // Throw Grenade
    { XINPUT_GAMEPAD_RIGHT_SHOULDER, 19 }, // Melee
    { XINPUT_GAMEPAD_DPAD_UP, 77 }, // Next Weapon
    { XINPUT_GAMEPAD_DPAD_DOWN, 73 }, // Next Grenade
    { XINPUT_GAMEPAD_DPAD_LEFT, 20 }, // Lean Left
    { XINPUT_GAMEPAD_DPAD_RIGHT, 21 }, // Lean Right
    { XINPUT_GAMEPAD_LEFT_TRIGGER, 71 }, // Aim
    { XINPUT_GAMEPAD_RIGHT_TRIGGER, 17 }, // Fire
    { XINPUT_GAMEPAD_BACK, 78 }, // Mission Status
    { XINPUT_GAMEPAD_START, -1 }, // Menu
};

static constexpr int MENU_NAVIGATION_MAP[][2] =
{
    {XINPUT_GAMEPAD_DPAD_UP,    VK_UP},
    {XINPUT_GAMEPAD_DPAD_DOWN,  VK_DOWN},
    {XINPUT_GAMEPAD_DPAD_LEFT,  VK_LEFT},
    {XINPUT_GAMEPAD_DPAD_RIGHT, VK_RIGHT},
    {XINPUT_GAMEPAD_A,          VK_RETURN},
    {XINPUT_GAMEPAD_B,          VK_ESCAPE}
};

static void HandleControllerButton(WORD button, int commandId)
{
    auto& btnState = g_Controller.gameButtons[button];
    bool isPressed;

    // Activate instead of Reload
    if (commandId == 88 && (g_State.canActivate || g_State.canSwap || g_State.isOperatingTurret))
    {
        commandId = 87;
    }

    if (button == XINPUT_GAMEPAD_LEFT_TRIGGER)
    {
        // Left trigger
        isPressed = g_Controller.state.Gamepad.bLeftTrigger > 30;
    }
    else if (button == XINPUT_GAMEPAD_RIGHT_TRIGGER)
    {
        // Right trigger
        isPressed = g_Controller.state.Gamepad.bRightTrigger > 30;
    }
    else
    {
        // Regular button
        isPressed = (g_Controller.state.Gamepad.wButtons & button) != 0;
    }

    // Check if we recently transitioned from menu to prevent immediate actions
    ULONGLONG currentTime = GetTickCount64();
    if (g_Controller.menuToGameTransitionTime > 0 && currentTime - g_Controller.menuToGameTransitionTime < 200) // 200ms grace period
    {
        // During transition period, only register button state but don't execute commands
        btnState.isPressed = isPressed;
        return;
    }

    if (isPressed && !btnState.isPressed)
    {
        // Press Escape to show the menu
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
    else if (isPressed)
    {
        if (commandId != -1)
        {
            g_Controller.commandActive[commandId] = true;
        }
    }
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

// Main function to poll controller and process all button mappings
void PollController()
{
    // Save previous connection state
    bool wasConnected = g_Controller.isConnected;

    // Update controller state once per frame
    g_Controller.isConnected = XInputGetState(0, &g_Controller.state) == ERROR_SUCCESS;

    // Check for connection state change
    if (wasConnected != g_Controller.isConnected)
    {
        HUDWeaponListUpdateTriggerNames(g_State.CHUDWeaponList);
        HUDGrenadeListUpdateTriggerNames(g_State.CHUDGrenadeList);
        HUDSwapUpdateTriggerName();
    }

    // Handle transition from gameplay to menu
    static bool wasPlaying = g_State.isPlaying;
    static bool wasMsgBoxVisible = g_State.isMsgBoxVisible;

    if (wasPlaying && !g_State.isPlaying && g_Controller.isConnected)
    {
        for (const auto& mapping : g_buttonMappings)
        {
            WORD button = mapping.first;
            int commandId = mapping.second;
            auto& btnState = g_Controller.gameButtons[button];

            if (btnState.isPressed)
            {
                if (commandId == -1) // Start button
                {
                    PostMessage(g_State.hWnd, WM_KEYUP, VK_ESCAPE, 0);
                }
                else
                {
                    g_Controller.commandActive[commandId] = false;
                    OnCommandOff(g_State.g_pGameClientShell, commandId);
                }
                btnState.isPressed = false;
                btnState.wasHandled = false;
            }
        }
    }

    if (!wasPlaying && g_State.isPlaying && g_Controller.isConnected)
    {
        // Set transition timestamp to prevent immediate actions
        g_Controller.menuToGameTransitionTime = GetTickCount64();

        // Clear any lingering menu button states
        for (int i = 0; i < 6; i++)
        {
            g_Controller.menuButtons[i].isPressed = false;
            g_Controller.menuButtons[i].pressStartTime = 0;
            g_Controller.menuButtons[i].lastRepeatTime = 0;
        }
    }

    // Handle message box visibility changes
    if (wasMsgBoxVisible && !g_State.isMsgBoxVisible && g_Controller.isConnected)
    {
        // Set transition timestamp to prevent immediate actions
        g_Controller.menuToGameTransitionTime = GetTickCount64();

        // Clear any lingering menu button states
        for (int i = 0; i < 6; i++)
        {
            g_Controller.menuButtons[i].isPressed = false;
            g_Controller.menuButtons[i].pressStartTime = 0;
            g_Controller.menuButtons[i].lastRepeatTime = 0;
        }
    }

    wasPlaying = g_State.isPlaying;
    wasMsgBoxVisible = g_State.isMsgBoxVisible;

    if (!g_Controller.isConnected)
    {
        memset(g_Controller.commandActive, 0, sizeof(g_Controller.commandActive));
        return;
    }

    // Menu navigation
    if (!g_State.isPlaying || g_State.isMsgBoxVisible)
    {
        const ULONGLONG currentTime = GetTickCount64();

        for (int i = 0; i < 6; i++)
        {
            auto& btnState = g_Controller.menuButtons[i];
            bool pressed = false;

            if (i < 4) // D-Pad
            {
                const bool buttonPressed = (g_Controller.state.Gamepad.wButtons & MENU_NAVIGATION_MAP[i][0]);
                bool joystickPressed = false;

                switch (i)
                {
                    case 0: // Up
                        joystickPressed = (g_Controller.state.Gamepad.sThumbLY > 16384) || (g_Controller.state.Gamepad.sThumbRY > 16384);
                        break;
                    case 1: // Down
                        joystickPressed = (g_Controller.state.Gamepad.sThumbLY < -16384) || (g_Controller.state.Gamepad.sThumbRY < -16384);
                        break;
                    case 2: // Left
                        joystickPressed = (g_Controller.state.Gamepad.sThumbLX < -16384) || (g_Controller.state.Gamepad.sThumbRX < -16384);
                        break;
                    case 3: // Right
                        joystickPressed = (g_Controller.state.Gamepad.sThumbLX > 16384) || (g_Controller.state.Gamepad.sThumbRX > 16384);
                        break;
                }

                pressed = buttonPressed || joystickPressed;

                // Handle auto-repeat
                if (pressed)
                {
                    if (!btnState.isPressed)
                    {
                        // Initial press
                        btnState.pressStartTime = currentTime;
                        btnState.lastRepeatTime = currentTime;
                    }
                    else
                    {
                        // Calculate time since last valid input
                        DWORD elapsedSinceStart = currentTime - btnState.pressStartTime;
                        DWORD elapsedSinceLastRepeat = currentTime - btnState.lastRepeatTime;

                        if (elapsedSinceStart > 500 && elapsedSinceLastRepeat > 100)
                        {
                            // Trigger repeat
                            PostMessage(g_State.hWnd, WM_KEYDOWN, MENU_NAVIGATION_MAP[i][1], 0);
                            btnState.lastRepeatTime = currentTime;
                        }
                    }
                }
            }
            else // A/B
            {
                pressed = (g_Controller.state.Gamepad.wButtons & MENU_NAVIGATION_MAP[i][0]);
            }

            // Handle state changes
            if (pressed != btnState.isPressed)
            {
                PostMessage(g_State.hWnd, pressed ? WM_KEYDOWN : WM_KEYUP, MENU_NAVIGATION_MAP[i][1], 0);
                btnState.isPressed = pressed;

                // Reset timing on release
                if (!pressed)
                {
                    btnState.pressStartTime = 0;
                    btnState.lastRepeatTime = 0;
                }
            }
        }

        // Handle shoulder buttons
        auto UpdateScreenPerformanceSetting = [&](DWORD button, auto& btnState, int direction)
            {
                bool pressed = (g_Controller.state.Gamepad.wButtons & button);
                if (pressed != btnState.isPressed)
                {
                    if (pressed)
                    {
                        if (g_State.pCurrentType != 0 && g_State.maxCurrentType != -1)
                        {
                            g_State.currentType = (g_State.currentType + direction + g_State.maxCurrentType) % g_State.maxCurrentType;
                            SetCurrentType(g_State.pCurrentType, g_State.currentType);
                        }
                    }
                    btnState.isPressed = pressed;
                }
            };

        UpdateScreenPerformanceSetting(XINPUT_GAMEPAD_LEFT_SHOULDER, g_Controller.leftShoulderState, -1);
        UpdateScreenPerformanceSetting(XINPUT_GAMEPAD_RIGHT_SHOULDER, g_Controller.rightShoulderState, 1);
    }
    // Handle in-game controls
    else
    {
        // Reset command states
        memset(g_Controller.commandActive, 0, sizeof(g_Controller.commandActive));

        // Process buttons
        for (const auto& mapping : g_buttonMappings)
        {
            HandleControllerButton(mapping.first, mapping.second);
        }
    }
}