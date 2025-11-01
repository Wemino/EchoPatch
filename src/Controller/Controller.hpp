#pragma once

#include <map>

struct ControllerState
{
	XINPUT_STATE state;
	bool isConnected = false;

	// Command activation states
	bool commandActive[117] = { false };

	// Button state tracking
	struct ButtonState
	{
		bool isPressed = false;
		bool wasHandled = false;
		ULONGLONG pressStartTime = 0;
		ULONGLONG lastRepeatTime = 0;
	};

	// Menu navigation states
	ButtonState menuButtons[6];
	ULONGLONG menuToGameTransitionTime = 0;

	// Game button states
	std::map<WORD, ButtonState> gameButtons;

	// ScreenPerformanceAdvanced
	ButtonState leftShoulderState;
	ButtonState rightShoulderState;
};

extern ControllerState g_Controller;

// Button mapping table exposed for config updates
extern std::pair<WORD, int> g_buttonMappings[16];

void PollController();
bool InitializeXInput();
void ShutdownXInput();