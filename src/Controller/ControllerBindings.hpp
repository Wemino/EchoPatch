#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "../Globals.cpp"
#include "Controller.hpp"

// Gamepad button mapping shared by the ini loader and the in-game button mapping menu
namespace ControllerBindings
{
    struct BindingSlot
    {
        const char* iniKey;
        bool isTrigger;
        int index;
        int* tapCommand;
        int* holdCommand;
        int* holdTime;
        int defaultTap;
        int defaultHold;
        int defaultHoldTime;
    };

    // Ordered as displayed in the button mapping menu, see s_groups in ScreenJoystickHook
    static BindingSlot s_slots[] =
    {
        // Face buttons
        { "GAMEPAD_SOUTH",          false, SDL_GAMEPAD_BUTTON_SOUTH,          &GAMEPAD_SOUTH,          &GAMEPAD_SOUTH_HOLD,          &GAMEPAD_SOUTH_HOLD_TIME,          15,  -1,  500 },
        { "GAMEPAD_EAST",           false, SDL_GAMEPAD_BUTTON_EAST,           &GAMEPAD_EAST,           &GAMEPAD_EAST_HOLD,           &GAMEPAD_EAST_HOLD_TIME,           14,  -1,  500 },
        { "GAMEPAD_WEST",           false, SDL_GAMEPAD_BUTTON_WEST,           &GAMEPAD_WEST,           &GAMEPAD_WEST_HOLD,           &GAMEPAD_WEST_HOLD_TIME,           88,  -1,  500 },
        { "GAMEPAD_NORTH",          false, SDL_GAMEPAD_BUTTON_NORTH,          &GAMEPAD_NORTH,          &GAMEPAD_NORTH_HOLD,          &GAMEPAD_NORTH_HOLD_TIME,          106, -1,  500 },

        // Shoulders & triggers
        { "GAMEPAD_LEFT_SHOULDER",  false, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,  &GAMEPAD_LEFT_SHOULDER,  &GAMEPAD_LEFT_SHOULDER_HOLD,  &GAMEPAD_LEFT_SHOULDER_HOLD_TIME,  81,  -1,  500 },
        { "GAMEPAD_RIGHT_SHOULDER", false, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, &GAMEPAD_RIGHT_SHOULDER, &GAMEPAD_RIGHT_SHOULDER_HOLD, &GAMEPAD_RIGHT_SHOULDER_HOLD_TIME, 19,  -1,  500 },
        { "GAMEPAD_LEFT_TRIGGER",   true,  0,                                 &GAMEPAD_LEFT_TRIGGER,   &GAMEPAD_LEFT_TRIGGER_HOLD,   &GAMEPAD_LEFT_TRIGGER_HOLD_TIME,   71,  -1,  500 },
        { "GAMEPAD_RIGHT_TRIGGER",  true,  1,                                 &GAMEPAD_RIGHT_TRIGGER,  &GAMEPAD_RIGHT_TRIGGER_HOLD,  &GAMEPAD_RIGHT_TRIGGER_HOLD_TIME,  17,  -1,  500 },

        // D-Pad
        { "GAMEPAD_DPAD_UP",        false, SDL_GAMEPAD_BUTTON_DPAD_UP,        &GAMEPAD_DPAD_UP,        &GAMEPAD_DPAD_UP_HOLD,        &GAMEPAD_DPAD_UP_HOLD_TIME,        77,  102, 500 },
        { "GAMEPAD_DPAD_DOWN",      false, SDL_GAMEPAD_BUTTON_DPAD_DOWN,      &GAMEPAD_DPAD_DOWN,      &GAMEPAD_DPAD_DOWN_HOLD,      &GAMEPAD_DPAD_DOWN_HOLD_TIME,      73,  50,  500 },
        { "GAMEPAD_DPAD_LEFT",      false, SDL_GAMEPAD_BUTTON_DPAD_LEFT,      &GAMEPAD_DPAD_LEFT,      &GAMEPAD_DPAD_LEFT_HOLD,      &GAMEPAD_DPAD_LEFT_HOLD_TIME,      20,  -1,  500 },
        { "GAMEPAD_DPAD_RIGHT",     false, SDL_GAMEPAD_BUTTON_DPAD_RIGHT,     &GAMEPAD_DPAD_RIGHT,     &GAMEPAD_DPAD_RIGHT_HOLD,     &GAMEPAD_DPAD_RIGHT_HOLD_TIME,     21,  -1,  500 },

        // Sticks & menu buttons
        { "GAMEPAD_LEFT_STICK",     false, SDL_GAMEPAD_BUTTON_LEFT_STICK,     &GAMEPAD_LEFT_STICK,     &GAMEPAD_LEFT_STICK_HOLD,     &GAMEPAD_LEFT_STICK_HOLD_TIME,     70,  -1,  500 },
        { "GAMEPAD_RIGHT_STICK",    false, SDL_GAMEPAD_BUTTON_RIGHT_STICK,    &GAMEPAD_RIGHT_STICK,    &GAMEPAD_RIGHT_STICK_HOLD,    &GAMEPAD_RIGHT_STICK_HOLD_TIME,    114, -1,  500 },
        { "GAMEPAD_BACK",           false, SDL_GAMEPAD_BUTTON_BACK,           &GAMEPAD_BACK,           &GAMEPAD_BACK_HOLD,           &GAMEPAD_BACK_HOLD_TIME,           78,  74,  500 },
        { "GAMEPAD_START",          false, SDL_GAMEPAD_BUTTON_START,          &GAMEPAD_START,          &GAMEPAD_START_HOLD,          &GAMEPAD_START_HOLD_TIME,          13,  75,  500 },

        // Extra buttons
        { "GAMEPAD_MISC1",          false, SDL_GAMEPAD_BUTTON_MISC1,          &GAMEPAD_MISC1,          &GAMEPAD_MISC1_HOLD,          &GAMEPAD_MISC1_HOLD_TIME,          50,  -1,  500 },
        { "GAMEPAD_RIGHT_PADDLE1",  false, SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1,  &GAMEPAD_RIGHT_PADDLE1,  &GAMEPAD_RIGHT_PADDLE1_HOLD,  &GAMEPAD_RIGHT_PADDLE1_HOLD_TIME,  82,  -1,  500 },
        { "GAMEPAD_RIGHT_PADDLE2",  false, SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2,  &GAMEPAD_RIGHT_PADDLE2,  &GAMEPAD_RIGHT_PADDLE2_HOLD,  &GAMEPAD_RIGHT_PADDLE2_HOLD_TIME,  -1,  -1,  500 },
        { "GAMEPAD_LEFT_PADDLE1",   false, SDL_GAMEPAD_BUTTON_LEFT_PADDLE1,   &GAMEPAD_LEFT_PADDLE1,   &GAMEPAD_LEFT_PADDLE1_HOLD,   &GAMEPAD_LEFT_PADDLE1_HOLD_TIME,   -1,  -1,  500 },
        { "GAMEPAD_LEFT_PADDLE2",   false, SDL_GAMEPAD_BUTTON_LEFT_PADDLE2,   &GAMEPAD_LEFT_PADDLE2,   &GAMEPAD_LEFT_PADDLE2_HOLD,   &GAMEPAD_LEFT_PADDLE2_HOLD_TIME,   -1,  -1,  500 },
    };

    static constexpr int SLOT_COUNT = static_cast<int>(sizeof(s_slots) / sizeof(s_slots[0]));

    static constexpr int MIN_HOLD_TIME = 100;
    static constexpr int MAX_HOLD_TIME = 2000;

    // ========================================
    // Action Catalog
    // ========================================

    struct GamepadAction
    {
        int commandId;
        const wchar_t* name;
    };

    static constexpr int COMMAND_NONE = -1;
    static constexpr int COMMAND_WEAPON_SLOT_FIRST = 30;
    static constexpr int COMMAND_WEAPON_SLOT_LAST = 39;
    static constexpr int COMMAND_DEPLOYABLE_TURRET = 43;

    // Display order of the action cycle in the menu
    static const GamepadAction s_actions[] =
    {
        { COMMAND_NONE, L"None" },

        // Combat
        { 17,  L"Fire" },
        { 71,  L"Aim / Zoom" },
        { 19,  L"Melee Attack" },
        { 81,  L"Throw Grenade" },
        { 106, L"Slow Motion" },

        // Movement
        { 15,  L"Jump" },
        { 14,  L"Crouch" },
        { 16,  L"Hold to Walk" },
        { 18,  L"Toggle Walk/Run" },
        { 20,  L"Lean Left" },
        { 21,  L"Lean Right" },
        { 82,  L"Center View" },

        // Weapons
        { 88,  L"Reload / Activate" },
        { 77,  L"Next Weapon" },
        { 76,  L"Previous Weapon" },
        { 103, L"Last Weapon" },
        { 102, L"Drop Weapon" },
        { 50,  L"Holster Weapon" },
        { 30,  L"Weapon Slot 1" },
        { 31,  L"Weapon Slot 2" },
        { 32,  L"Weapon Slot 3" },
        { 33,  L"Weapon Slot 4" },
        { 34,  L"Weapon Slot 5" },
        { 35,  L"Weapon Slot 6" },
        { 36,  L"Weapon Slot 7" },
        { 37,  L"Weapon Slot 8" },
        { 38,  L"Weapon Slot 9" },
        { 39,  L"Weapon Slot 10" },

        // Grenades and equipment
        { 73,  L"Next Grenade" },
        { 40,  L"Frag Grenade" },
        { 41,  L"Proximity Mine" },
        { 42,  L"Remote Bomb" },
        { 43,  L"Deployable Turret" },

        // Items
        { 70,  L"Use Medkit" },
        { 114, L"Flashlight" },

        // System
        { 13,  L"Pause Menu" },
        { 78,  L"Mission Status" },
        { 74,  L"Quick Save" },
        { 75,  L"Quick Load" },
    };

    static int NormalizeCommand(int commandId)
    {
        return (commandId <= 0 || commandId > MAX_COMMAND_ID) ? COMMAND_NONE : commandId;
    }

    static int GetWeaponSlotCount()
    {
        int capacity = EnableCustomMaxWeaponCapacity ? MaxWeaponCapacity : 3;
        return std::clamp(capacity, 1, COMMAND_WEAPON_SLOT_LAST - COMMAND_WEAPON_SLOT_FIRST + 1);
    }

    static bool IsActionAvailable(int commandId)
    {
        if (commandId >= COMMAND_WEAPON_SLOT_FIRST && commandId <= COMMAND_WEAPON_SLOT_LAST)
            return commandId - COMMAND_WEAPON_SLOT_FIRST < GetWeaponSlotCount();

        // The deployable turret was added by Extraction Point
        if (commandId == COMMAND_DEPLOYABLE_TURRET)
            return g_State.IsExpansion();

        return true;
    }

    // Name of any command, including the ones hidden from the menu or set manually in the ini
    static const wchar_t* GetActionName(int commandId)
    {
        commandId = NormalizeCommand(commandId);

        for (const auto& action : s_actions)
        {
            if (action.commandId == commandId)
                return action.name;
        }

        static wchar_t s_unknownName[32];
        swprintf_s(s_unknownName, L"Command %d", commandId);
        return s_unknownName;
    }

    // Commands listed in the action cycle of the menu, index 0 is "None"
    static const std::vector<int>& GetActionList()
    {
        static std::vector<int> s_actionList;

        if (s_actionList.empty())
        {
            for (const auto& action : s_actions)
            {
                if (IsActionAvailable(action.commandId))
                    s_actionList.push_back(action.commandId);
            }
        }

        return s_actionList;
    }

    // ==========================================================
    // Ini / Runtime
    // ==========================================================

    static std::string HoldKey(const BindingSlot& slot)
    {
        return std::string(slot.iniKey) + "_HOLD";
    }

    static std::string HoldTimeKey(const BindingSlot& slot)
    {
        return std::string(slot.iniKey) + "_HOLD_TIME";
    }

    static void LoadFromIni()
    {
        for (auto& slot : s_slots)
        {
            *slot.tapCommand = IniHelper::ReadInteger("Controller", slot.iniKey, slot.defaultTap);
            *slot.holdCommand = IniHelper::ReadInteger("Controller", HoldKey(slot).c_str(), slot.defaultHold);
            *slot.holdTime = IniHelper::ReadInteger("Controller", HoldTimeKey(slot).c_str(), slot.defaultHoldTime);
        }
    }

    static void Apply(const BindingSlot& slot)
    {
        if (slot.isTrigger)
        {
            ConfigureTrigger(slot.index, *slot.tapCommand, *slot.holdCommand, *slot.holdTime);
        }
        else
        {
            ConfigureButton(slot.index, *slot.tapCommand, *slot.holdCommand, *slot.holdTime);
        }
    }

    static void ApplyAll()
    {
        for (const auto& slot : s_slots)
        {
            Apply(slot);
        }
    }

    // Updates the ini structure
    static void WriteToIni(const BindingSlot& slot)
    {
        auto& section = IniHelper::iniReader["Controller"];
        section[slot.iniKey] = std::to_string(*slot.tapCommand);
        section[HoldKey(slot)] = std::to_string(*slot.holdCommand);
        section[HoldTimeKey(slot)] = std::to_string(*slot.holdTime);
    }

    static void SetCommand(int slotIndex, bool isHold, int commandId)
    {
        if (slotIndex < 0 || slotIndex >= SLOT_COUNT)
            return;

        BindingSlot& slot = s_slots[slotIndex];
        int* target = isHold ? slot.holdCommand : slot.tapCommand;
        commandId = NormalizeCommand(commandId);

        if (NormalizeCommand(*target) == commandId)
            return;

        *target = commandId;
        Apply(slot);
        WriteToIni(slot);
        IniHelper::Save();
    }

    // The value used by most buttons, shown by the hold duration slider
    static int GetCommonHoldTime()
    {
        int bestTime = s_slots[0].defaultHoldTime;
        int bestCount = 0;

        for (const auto& candidate : s_slots)
        {
            int count = 0;
            for (const auto& slot : s_slots)
            {
                if (*slot.holdTime == *candidate.holdTime)
                    count++;
            }

            if (count > bestCount)
            {
                bestCount = count;
                bestTime = *candidate.holdTime;
            }
        }

        return bestTime;
    }

    static void SetAllHoldTimes(int holdTimeMs)
    {
        holdTimeMs = std::clamp(holdTimeMs, MIN_HOLD_TIME, MAX_HOLD_TIME);

        for (auto& slot : s_slots)
        {
            *slot.holdTime = holdTimeMs;
            Apply(slot);
            WriteToIni(slot);
        }

        IniHelper::Save();
    }

    static void RestoreDefaults()
    {
        for (auto& slot : s_slots)
        {
            *slot.tapCommand = slot.defaultTap;
            *slot.holdCommand = slot.defaultHold;
            *slot.holdTime = slot.defaultHoldTime;
            Apply(slot);
            WriteToIni(slot);
        }

        IniHelper::Save();
    }
}
