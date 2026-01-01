#define NOMINMAX

#include <Windows.h>
#include <SDL3/SDL.h>
#include <algorithm>

#include "../Core/Core.hpp"
#include "Controller.hpp"
#include "../Client/Client.hpp"

// ==========================================================
// Global State
// ==========================================================

ControllerState g_Controller;
TouchpadConfig g_TouchpadConfig;

// ==========================================================
// Constants
// ==========================================================

static constexpr Sint16 STICK_DEADZONE = 16384;
static constexpr Sint16 TRIGGER_THRESHOLD = 8000;
static constexpr ULONGLONG MENU_REPEAT_DELAY = 500;
static constexpr ULONGLONG MENU_REPEAT_RATE = 100;
static constexpr ULONGLONG TRANSITION_GRACE_PERIOD = 200;

// ==========================================================
// Static State - Controller
// ==========================================================

static SDL_Gamepad* s_pGamepad = nullptr;
static GamepadCapabilities s_capabilities;

// ==========================================================
// Static State - Frame Timing
// ==========================================================

struct FrameTiming
{
    LARGE_INTEGER lastFrameTime = {};
    LARGE_INTEGER frequency = {};
    float deltaTime = 0.0f;
};

static FrameTiming s_frameTiming;

// ==========================================================
// Static State - Gyro
// ==========================================================

static GyroState s_gyroState;

struct GyroConfig
{
    bool isEnabled = true;
    float sensitivity = 1.0f;
    float smoothing = 0.016f;
    bool invertY = false;
};

struct GyroProcessingState
{
    float smoothedYaw = 0.0f;
    float smoothedPitch = 0.0f;
    LARGE_INTEGER lastFrameTime = {};
    LARGE_INTEGER frequency = {};
    bool isInitialized = false;
};

struct GyroAutoOffset
{
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;

    static constexpr int BUFFER_SIZE = 256;
    float bufferX[BUFFER_SIZE] = {};
    float bufferY[BUFFER_SIZE] = {};
    float bufferZ[BUFFER_SIZE] = {};
    float bufferDt[BUFFER_SIZE] = {};
    int bufferIndex = 0;
    int sampleCount = 0;

    double sumDt = 0.0;
    float resyncAccumulator = 0.0f;
    float pendingDt = 0.0f;

    float stillnessTimer = 0.0f;
    bool hasInitialCalibration = false;

    float startGravX = 0.0f;
    float startGravY = 0.0f;
    float startGravZ = 0.0f;
    bool hasStartGrav = false;
    float maxGravAngleDuringWindow = 0.0f;

    float smAccelX = 0.0f;
    float smAccelY = 0.0f;
    float smAccelZ = 0.0f;
    bool hasSmoothedAccel = false;

    float stabilityCooldownTimer = 0.0f;
    bool isSurfaceMode = false;
};

static GyroConfig s_gyroConfig;
static GyroProcessingState s_gyroProcessing;
static GyroAutoOffset s_gyroOffset;

// ==========================================================
// Static State - Touchpad
// ==========================================================

struct TouchpadState
{
    bool wasDown = false;
    float lastX = 0.0f;
    float lastY = 0.0f;
};

static TouchpadState s_touchpadFinger[2];
static bool s_wasTouchpadPressed[2] = { false, false };

// ==========================================================
// Static State - Button Mappings
// ==========================================================

static int s_buttonCommands[SDL_GAMEPAD_BUTTON_COUNT] = {};
static int s_triggerCommands[2] = {};

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
    ButtonNames buttons[SDL_GAMEPAD_BUTTON_COUNT];
    ButtonNames triggers[2];
};

static const ButtonNameSet s_xboxNames = []()
{
    ButtonNameSet set = {};
    set.buttons[SDL_GAMEPAD_BUTTON_SOUTH] = { L"A", L"A Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_EAST] = { L"B", L"B Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_WEST] = { L"X", L"X Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_NORTH] = { L"Y", L"Y Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER] = { L"LB", L"Left Bumper" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER] = { L"RB", L"Right Bumper" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK] = { L"LS", L"Left Stick" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK] = { L"RS", L"Right Stick" };
    set.buttons[SDL_GAMEPAD_BUTTON_BACK] = { L"Back", L"Back Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_START] = { L"Start", L"Start Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_UP] = { L"D-Up", L"D-Pad Up" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = { L"D-Down", L"D-Pad Down" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = { L"D-Left", L"D-Pad Left" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = { L"D-Right", L"D-Pad Right" };
    set.buttons[SDL_GAMEPAD_BUTTON_MISC1] = { L"Share", L"Share Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1] = { L"P1", L"Paddle 1" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_PADDLE1] = { L"P3", L"Paddle 3" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2] = { L"P2", L"Paddle 2" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_PADDLE2] = { L"P4", L"Paddle 4" };
    set.triggers[0] = { L"LT", L"Left Trigger" };
    set.triggers[1] = { L"RT", L"Right Trigger" };
    return set;
}();

static const ButtonNameSet s_playstationNames = []()
{
    ButtonNameSet set = {};
    set.buttons[SDL_GAMEPAD_BUTTON_SOUTH] = { L"Cross", L"Cross Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_EAST] = { L"Circle", L"Circle Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_WEST] = { L"Square", L"Square Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_NORTH] = { L"Triangle", L"Triangle Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER] = { L"L1", L"L1 Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER] = { L"R1", L"R1 Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK] = { L"L3", L"L3 Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK] = { L"R3", L"R3 Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_BACK] = { L"Share", L"Share Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_START] = { L"Options", L"Options Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_UP] = { L"D-Up", L"D-Pad Up" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = { L"D-Down", L"D-Pad Down" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = { L"D-Left", L"D-Pad Left" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = { L"D-Right", L"D-Pad Right" };
    set.buttons[SDL_GAMEPAD_BUTTON_MISC1] = { L"Mic", L"Microphone Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1] = { L"P1", L"Paddle 1" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_PADDLE1] = { L"P3", L"Paddle 3" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2] = { L"P2", L"Paddle 2" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_PADDLE2] = { L"P4", L"Paddle 4" };
    set.triggers[0] = { L"L2", L"L2 Trigger" };
    set.triggers[1] = { L"R2", L"R2 Trigger" };
    return set;
}();

static const ButtonNameSet s_nintendoNames = []()
{
    ButtonNameSet set = {};
    set.buttons[SDL_GAMEPAD_BUTTON_SOUTH] = { L"B", L"B Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_EAST] = { L"A", L"A Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_WEST] = { L"Y", L"Y Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_NORTH] = { L"X", L"X Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER] = { L"L", L"L Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER] = { L"R", L"R Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK] = { L"LS", L"Left Stick" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK] = { L"RS", L"Right Stick" };
    set.buttons[SDL_GAMEPAD_BUTTON_BACK] = { L"-", L"- Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_START] = { L"+", L"+ Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_UP] = { L"D-Up", L"D-Pad Up" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = { L"D-Down", L"D-Pad Down" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = { L"D-Left", L"D-Pad Left" };
    set.buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = { L"D-Right", L"D-Pad Right" };
    set.buttons[SDL_GAMEPAD_BUTTON_MISC1] = { L"Capture", L"Capture Button" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1] = { L"P1", L"Paddle 1" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_PADDLE1] = { L"P3", L"Paddle 3" };
    set.buttons[SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2] = { L"P2", L"Paddle 2" };
    set.buttons[SDL_GAMEPAD_BUTTON_LEFT_PADDLE2] = { L"P4", L"Paddle 4" };
    set.triggers[0] = { L"ZL", L"ZL Trigger" };
    set.triggers[1] = { L"ZR", L"ZR Trigger" };
    return set;
 }();

static const ButtonNameSet& GetButtonNameSet()
{
    switch (s_capabilities.style)
    {
        case GamepadStyle::PlayStation: return s_playstationNames;
        case GamepadStyle::Nintendo:    return s_nintendoNames;
        default:                        return s_xboxNames;
    }
}

// ==========================================================
// Key Simulation Mappings
// ==========================================================

struct KeySimulation
{
    int commandId;
    int virtualKey;
};

static constexpr KeySimulation s_keySimulations[] =
{
    { 13, VK_ESCAPE },  // Menu
    { 74, VK_F5 },      // Quicksave
    { 75, VK_F9 },      // Quickload
};

static int GetSimulatedKey(int commandId)
{
    for (const auto& sim : s_keySimulations)
    {
        if (sim.commandId == commandId)
            return sim.virtualKey;
    }

    return 0;
}

// ==========================================================
// Frame Timing
// ==========================================================

static void UpdateFrameTiming()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    s_frameTiming.deltaTime = static_cast<float>(currentTime.QuadPart - s_frameTiming.lastFrameTime.QuadPart) / static_cast<float>(s_frameTiming.frequency.QuadPart);

    s_frameTiming.deltaTime = std::clamp(s_frameTiming.deltaTime, 0.0001f, 0.1f);
    s_frameTiming.lastFrameTime = currentTime;
}

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

static void LoadGamepadCapabilities(SDL_Gamepad* pGamepad)
{
    s_capabilities = GamepadCapabilities();

    if (!pGamepad)
        return;

    s_capabilities.style = DetectGamepadStyle(pGamepad);
    s_capabilities.hasTouchpad = SDL_GetNumGamepadTouchpads(pGamepad) > 0;
    s_capabilities.hasGyro = SDL_GamepadHasSensor(pGamepad, SDL_SENSOR_GYRO);
    s_capabilities.hasAccel = SDL_GamepadHasSensor(pGamepad, SDL_SENSOR_ACCEL);
    s_capabilities.name = SDL_GetGamepadName(pGamepad);
    s_capabilities.vendorId = SDL_GetGamepadVendor(pGamepad);
    s_capabilities.productId = SDL_GetGamepadProduct(pGamepad);

    if (s_capabilities.hasGyro)
    {
        SDL_SetGamepadSensorEnabled(pGamepad, SDL_SENSOR_GYRO, true);
    }

    if (s_capabilities.hasAccel)
    {
        SDL_SetGamepadSensorEnabled(pGamepad, SDL_SENSOR_ACCEL, true);
    }
}

static void ResetControllerState()
{
    s_capabilities = GamepadCapabilities();
    s_gyroState = GyroState();
    s_gyroProcessing = GyroProcessingState();
    s_gyroOffset = GyroAutoOffset();
    s_touchpadFinger[0] = TouchpadState();
    s_touchpadFinger[1] = TouchpadState();
    s_wasTouchpadPressed[0] = false;
    s_wasTouchpadPressed[1] = false;
}

// ==========================================================
// Initialization / Shutdown
// ==========================================================

bool InitializeSDLGamepad()
{
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
    {
        SDL_Quit();
        return false;
    }

    LoadGamepadMappings();

    QueryPerformanceFrequency(&s_frameTiming.frequency);
    QueryPerformanceCounter(&s_frameTiming.lastFrameTime);

    return true;
}

void ShutdownSDLGamepad()
{
    if (s_pGamepad)
    {
        SDL_CloseGamepad(s_pGamepad);
        s_pGamepad = nullptr;
    }

    ResetControllerState();
    s_frameTiming = FrameTiming();

    SDL_Quit();
}

// ==========================================================
// Accessors - Controller
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

bool IsControllerConnected()
{
    return s_pGamepad && SDL_GamepadConnected(s_pGamepad);
}

// ==========================================================
// Accessors - Gyro
// ==========================================================

bool HasGyro()
{
    return s_capabilities.hasGyro;
}

bool IsGyroEnabled()
{
    return s_gyroConfig.isEnabled && s_capabilities.hasGyro;
}

const GyroState& GetGyroState()
{
    return s_gyroState;
}

// ==========================================================
// Accessors - Touchpad
// ==========================================================

bool HasTouchpad()
{
    return s_capabilities.hasTouchpad;
}

// ==========================================================
// Configuration - Gyro
// ==========================================================

void SetGyroEnabled(bool enabled)
{
    s_gyroConfig.isEnabled = enabled;
}

void SetGyroSensitivity(float sensitivity)
{
    s_gyroConfig.sensitivity = std::max(sensitivity, 0.0f);
}

void SetGyroSmoothing(float smoothing)
{
    s_gyroConfig.smoothing = std::clamp(smoothing, 0.001f, 0.1f);
}

void SetGyroInvertY(bool invert)
{
    s_gyroConfig.invertY = invert;
}

void ResetGyroState()
{
    s_gyroProcessing.smoothedYaw = 0.0f;
    s_gyroProcessing.smoothedPitch = 0.0f;
    s_gyroProcessing.isInitialized = false;
    s_gyroOffset = GyroAutoOffset();
}

// ==========================================================
// Gyro Auto-Calibration
// ==========================================================

static void ResetStillnessTracking()
{
    s_gyroOffset.stillnessTimer = 0.0f;
    s_gyroOffset.hasStartGrav = false;
    s_gyroOffset.maxGravAngleDuringWindow = 0.0f;
}

static void UpdateGyroOffset(float gyroX, float gyroY, float gyroZ, float accelX, float accelY, float accelZ, float deltaTime)
{
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0f)
        return;

    float dt = fminf(deltaTime, 0.1f);

    if (!std::isfinite(gyroX) || !std::isfinite(gyroY) || !std::isfinite(gyroZ))
        return;
    if (!std::isfinite(accelX) || !std::isfinite(accelY) || !std::isfinite(accelZ))
        return;

    // Freeze guard: ignore exact duplicates since real sensors always have noise
    if (s_gyroOffset.sampleCount > 0)
    {
        int prevIdx = (s_gyroOffset.bufferIndex - 1 + GyroAutoOffset::BUFFER_SIZE) % GyroAutoOffset::BUFFER_SIZE;
        if (gyroX == s_gyroOffset.bufferX[prevIdx] && gyroY == s_gyroOffset.bufferY[prevIdx] && gyroZ == s_gyroOffset.bufferZ[prevIdx])
        {
            s_gyroOffset.pendingDt = fminf(s_gyroOffset.pendingDt + dt, 0.1f);
            return;
        }
    }

    // The second fminf is intentional. First cap limits incoming deltaTime, second cap
    // limits the sum after adding pendingDt. This ensures dt never exceeds 0.1s even when
    // accumulated time from skipped duplicate frames is added.
    dt = fminf(dt + s_gyroOffset.pendingDt, 0.1f);
    s_gyroOffset.pendingDt = 0.0f;

    // ==========================================================
    // Update Ring Buffer
    // ==========================================================

    int idx = s_gyroOffset.bufferIndex;

    if (s_gyroOffset.sampleCount >= GyroAutoOffset::BUFFER_SIZE)
    {
        s_gyroOffset.sumDt -= s_gyroOffset.bufferDt[idx];
    }

    s_gyroOffset.bufferX[idx] = gyroX;
    s_gyroOffset.bufferY[idx] = gyroY;
    s_gyroOffset.bufferZ[idx] = gyroZ;
    s_gyroOffset.bufferDt[idx] = dt;

    s_gyroOffset.sumDt += dt;

    s_gyroOffset.bufferIndex = (idx + 1) % GyroAutoOffset::BUFFER_SIZE;

    if (s_gyroOffset.sampleCount < GyroAutoOffset::BUFFER_SIZE)
        s_gyroOffset.sampleCount++;

    s_gyroOffset.resyncAccumulator += dt;
    if (s_gyroOffset.resyncAccumulator >= 2.0f)
    {
        s_gyroOffset.resyncAccumulator = 0.0f;
        s_gyroOffset.sumDt = 0.0;
        for (int i = 0; i < s_gyroOffset.sampleCount; i++)
        {
            int bufIdx = (s_gyroOffset.bufferIndex - s_gyroOffset.sampleCount + i + GyroAutoOffset::BUFFER_SIZE) % GyroAutoOffset::BUFFER_SIZE;
            s_gyroOffset.sumDt += s_gyroOffset.bufferDt[bufIdx];
        }
    }

    bool isFull = (s_gyroOffset.sampleCount >= GyroAutoOffset::BUFFER_SIZE);
    if ((s_gyroOffset.sumDt < 0.5f && !isFull) || s_gyroOffset.sampleCount < 2)
        return;

    // ==========================================================
    // Smooth Accelerometer
    // ==========================================================

    const float accelTau = 0.1f;
    float accelAlpha = 1.0f - expf(-dt / accelTau);

    if (!s_gyroOffset.hasSmoothedAccel)
    {
        s_gyroOffset.smAccelX = accelX;
        s_gyroOffset.smAccelY = accelY;
        s_gyroOffset.smAccelZ = accelZ;
        s_gyroOffset.hasSmoothedAccel = true;
    }
    else
    {
        s_gyroOffset.smAccelX += (accelX - s_gyroOffset.smAccelX) * accelAlpha;
        s_gyroOffset.smAccelY += (accelY - s_gyroOffset.smAccelY) * accelAlpha;
        s_gyroOffset.smAccelZ += (accelZ - s_gyroOffset.smAccelZ) * accelAlpha;
    }

    // ==========================================================
    // Thresholds
    // ==========================================================

    // Thresholds in radians (gravity angles) and rad/s (gyro rates)
    const float THRESH_GRAV_INIT = 0.0026f;       // 0.15 deg
    const float THRESH_GRAV_REFINE = 0.00087f;    // 0.05 deg
    const float THRESH_GRAV_SURFACE = 0.0035f;    // 0.20 deg
    const float THRESH_TREND = 0.0007f;           // 0.04 deg/s
    const float THRESH_INIT_SIGNAL = 0.35f;       // 20 deg/s
    const float THRESH_REFINE_HANDHELD = 0.0026f; // 0.15 deg/s
    const float THRESH_REFINE_SURFACE = 0.035f;   // 2.0 deg/s
    const float THRESH_RANGE = 0.17f;             // 10 deg/s peak to peak

    // ==========================================================
    // Gate 1: Gravity Magnitude
    // ==========================================================

    float accLenSq = s_gyroOffset.smAccelX * s_gyroOffset.smAccelX + s_gyroOffset.smAccelY * s_gyroOffset.smAccelY + s_gyroOffset.smAccelZ * s_gyroOffset.smAccelZ;

    if (accLenSq < 64.0f || accLenSq > 132.0f)
    {
        ResetStillnessTracking();
        return;
    }

    // ==========================================================
    // Cooldown
    // ==========================================================

    if (s_gyroOffset.stabilityCooldownTimer > 0.0f)
    {
        s_gyroOffset.stabilityCooldownTimer -= dt;
        s_gyroOffset.stillnessTimer = 0.0f;
        s_gyroOffset.hasStartGrav = false;
        return;
    }

    // ==========================================================
    // Anchor Initialization
    // ==========================================================

    float accLen = sqrtf(accLenSq);
    float normGravX = s_gyroOffset.smAccelX / accLen;
    float normGravY = s_gyroOffset.smAccelY / accLen;
    float normGravZ = s_gyroOffset.smAccelZ / accLen;

    if (!s_gyroOffset.hasStartGrav)
    {
        s_gyroOffset.startGravX = normGravX;
        s_gyroOffset.startGravY = normGravY;
        s_gyroOffset.startGravZ = normGravZ;
        s_gyroOffset.hasStartGrav = true;
        s_gyroOffset.maxGravAngleDuringWindow = 0.0f;
    }

    // ==========================================================
    // Gate 2: Gravity Stability
    // ==========================================================

    float dot = s_gyroOffset.startGravX * normGravX + s_gyroOffset.startGravY * normGravY + s_gyroOffset.startGravZ * normGravZ;

    float gravAngle = acosf(fmaxf(-1.0f, fminf(1.0f, dot)));

    if (gravAngle > s_gyroOffset.maxGravAngleDuringWindow)
        s_gyroOffset.maxGravAngleDuringWindow = gravAngle;

    float gravLimit = s_gyroOffset.hasInitialCalibration ? (s_gyroOffset.isSurfaceMode ? THRESH_GRAV_SURFACE : THRESH_GRAV_REFINE) : THRESH_GRAV_INIT;

    if (s_gyroOffset.maxGravAngleDuringWindow > gravLimit)
    {
        // Large movement (about 3 degrees) means the controller was picked up or put down.
        // This is the primary mechanism for exiting surface mode. The hysteresis exit in
        // Gate 3 is technically unreachable because you must pass this gate first, but the
        // large movement check here handles all real world pickup scenarios.
        if (s_gyroOffset.maxGravAngleDuringWindow > 0.05f)
            s_gyroOffset.isSurfaceMode = false;

        ResetStillnessTracking();
        s_gyroOffset.stabilityCooldownTimer = 1.0f;
        return;
    }

    // ==========================================================
    // Calculate Statistics (Time-Weighted)
    // ==========================================================

    // Time weighted statistics. We split the buffer into two halves by time to detect slow
    // drift (trend). Assigning boundary samples entirely to one bucket rather than splitting
    // them proportionally introduces at most 0.8% error at typical poll rates, which is
    // negligible for trend detection purposes.
    double sum1WX = 0, sum1WY = 0, sum1WZ = 0, time1 = 0;
    double sum2WX = 0, sum2WY = 0, sum2WZ = 0, time2 = 0;
    double halfTime = s_gyroOffset.sumDt * 0.5;
    double accumulatedTime = 0;

    int newestIdx = (s_gyroOffset.bufferIndex - 1 + GyroAutoOffset::BUFFER_SIZE) % GyroAutoOffset::BUFFER_SIZE;
    float minX = s_gyroOffset.bufferX[newestIdx], maxX = minX;
    float minY = s_gyroOffset.bufferY[newestIdx], maxY = minY;
    float minZ = s_gyroOffset.bufferZ[newestIdx], maxZ = minZ;

    for (int i = 0; i < s_gyroOffset.sampleCount; i++)
    {
        int bufIdx = (s_gyroOffset.bufferIndex - s_gyroOffset.sampleCount + i + GyroAutoOffset::BUFFER_SIZE) % GyroAutoOffset::BUFFER_SIZE;
        float gx = s_gyroOffset.bufferX[bufIdx];
        float gy = s_gyroOffset.bufferY[bufIdx];
        float gz = s_gyroOffset.bufferZ[bufIdx];
        float sampleDt = s_gyroOffset.bufferDt[bufIdx];

        if (gx < minX) minX = gx;
        if (gx > maxX) maxX = gx;
        if (gy < minY) minY = gy;
        if (gy > maxY) maxY = gy;
        if (gz < minZ) minZ = gz;
        if (gz > maxZ) maxZ = gz;

        if (accumulatedTime < halfTime)
        {
            sum1WX += gx * sampleDt;
            sum1WY += gy * sampleDt;
            sum1WZ += gz * sampleDt;
            time1 += sampleDt;
        }
        else
        {
            sum2WX += gx * sampleDt;
            sum2WY += gy * sampleDt;
            sum2WZ += gz * sampleDt;
            time2 += sampleDt;
        }
        accumulatedTime += sampleDt;
    }

    double totalTime = time1 + time2;
    float meanX = (float)((sum1WX + sum2WX) / totalTime);
    float meanY = (float)((sum1WY + sum2WY) / totalTime);
    float meanZ = (float)((sum1WZ + sum2WZ) / totalTime);
    float meanMag = sqrtf(meanX * meanX + meanY * meanY + meanZ * meanZ);

    // ==========================================================
    // Gate 3: Signal Guard
    // ==========================================================

    float maxRange = fmaxf(maxX - minX, fmaxf(maxY - minY, maxZ - minZ));

    if (maxRange > THRESH_RANGE)
    {
        ResetStillnessTracking();
        return;
    }

    if (!s_gyroOffset.hasInitialCalibration)
    {
        if (meanMag > THRESH_INIT_SIGNAL)
        {
            ResetStillnessTracking();
            return;
        }

        // Yaw spin detection prevents calibrating while the controller is being rotated flat
        // on a table (where gravity doesn't change). The 0.087 rad/s threshold (5 deg/s)
        // allows factory bias up to about 3 deg/s while catching real table spins. The check
        // only runs above this threshold so small meanMag values don't cause ratio instability.
        if (meanMag > 0.087f)
        {
            float yaw = fabsf(meanX * normGravX + meanY * normGravY + meanZ * normGravZ);
            if (yaw > meanMag * 0.9f)
            {
                ResetStillnessTracking();
                return;
            }
        }
    }
    else
    {
        float dx = meanX - s_gyroOffset.offsetX;
        float dy = meanY - s_gyroOffset.offsetY;
        float dz = meanZ - s_gyroOffset.offsetZ;
        float deviation = sqrtf(dx * dx + dy * dy + dz * dz);

        // Surface mode uses hysteresis to prevent thrashing. Enter threshold is tighter than
        // exit threshold. The 1 second stability requirement prevents false positives from
        // brief moments of steady hands during gameplay.
        const float SURFACE_ENTER_THRESH = 0.0017f;

        if (!s_gyroOffset.isSurfaceMode)
        {
            if (s_gyroOffset.maxGravAngleDuringWindow < SURFACE_ENTER_THRESH && s_gyroOffset.stillnessTimer > 1.0f)
                s_gyroOffset.isSurfaceMode = true;
        }

        float deviationLimit = s_gyroOffset.isSurfaceMode ? THRESH_REFINE_SURFACE : THRESH_REFINE_HANDHELD;

        if (deviation > deviationLimit)
        {
            ResetStillnessTracking();
            s_gyroOffset.stabilityCooldownTimer = 0.5f;
            return;
        }

        if (s_gyroOffset.isSurfaceMode && meanMag > 0.087f)
        {
            float yaw = fabsf(meanX * normGravX + meanY * normGravY + meanZ * normGravZ);
            if (yaw > meanMag * 0.9f)
            {
                ResetStillnessTracking();
                return;
            }
        }
    }

    // ==========================================================
    // Gate 4: Trend Stability
    // ==========================================================

    if (time1 <= 0.0 || time2 <= 0.0)
        return;

    float m1x = (float)(sum1WX / time1);
    float m1y = (float)(sum1WY / time1);
    float m1z = (float)(sum1WZ / time1);

    float m2x = (float)(sum2WX / time2);
    float m2y = (float)(sum2WY / time2);
    float m2z = (float)(sum2WZ / time2);

    float trend = fmaxf(fabsf(m2x - m1x), fmaxf(fabsf(m2y - m1y), fabsf(m2z - m1z)));

    if (trend > THRESH_TREND)
    {
        ResetStillnessTracking();
        s_gyroOffset.stabilityCooldownTimer = 0.5f;
        return;
    }

    // ==========================================================
    // Calibration
    // ==========================================================

    s_gyroOffset.stillnessTimer += dt;
    float requiredTime = s_gyroOffset.hasInitialCalibration ? 2.5f : 2.0f;

    if (s_gyroOffset.stillnessTimer > requiredTime)
    {
        if (!s_gyroOffset.hasInitialCalibration)
        {
            s_gyroOffset.offsetX = meanX;
            s_gyroOffset.offsetY = meanY;
            s_gyroOffset.offsetZ = meanZ;
            s_gyroOffset.hasInitialCalibration = true;

            // Anchor reset only happens after initial calibration. During refinement the
            // anchor persists to prevent drift creep from slow continuous rotation.
            s_gyroOffset.hasStartGrav = false;
            s_gyroOffset.maxGravAngleDuringWindow = 0.0f;
        }
        else
        {
            float updateInterval = requiredTime * 0.5f;
            const float DRIFT_CORRECTION_SPEED = 0.0026f;
            float maxStep = DRIFT_CORRECTION_SPEED * updateInterval;

            float diffX = meanX - s_gyroOffset.offsetX;
            float diffY = meanY - s_gyroOffset.offsetY;
            float diffZ = meanZ - s_gyroOffset.offsetZ;

            s_gyroOffset.offsetX += fmaxf(-maxStep, fminf(maxStep, diffX));
            s_gyroOffset.offsetY += fmaxf(-maxStep, fminf(maxStep, diffY));
            s_gyroOffset.offsetZ += fmaxf(-maxStep, fminf(maxStep, diffZ));
        }

        s_gyroOffset.stillnessTimer = requiredTime * 0.5f;
    }
}

// ==========================================================
// Processing - Gyro
// ==========================================================

static void ProcessGyro()
{
    s_gyroState.isValid = false;

    if (!s_pGamepad || !s_gyroConfig.isEnabled || !s_capabilities.hasGyro)
        return;

    float gyro[3] = { 0.0f, 0.0f, 0.0f };
    float accel[3] = { 0.0f, 0.0f, 0.0f };

    if (SDL_GetGamepadSensorData(s_pGamepad, SDL_SENSOR_GYRO, gyro, 3) && SDL_GetGamepadSensorData(s_pGamepad, SDL_SENSOR_ACCEL, accel, 3))
    {
        UpdateGyroOffset(gyro[0], gyro[1], gyro[2], accel[0], accel[1], accel[2], s_frameTiming.deltaTime);

        s_gyroState.x = gyro[0] - s_gyroOffset.offsetX;
        s_gyroState.y = gyro[1] - s_gyroOffset.offsetY;
        s_gyroState.z = gyro[2] - s_gyroOffset.offsetZ;
        s_gyroState.isValid = true;
    }
}

void GetProcessedGyroDelta(float& outYaw, float& outPitch)
{
    outYaw = 0.0f;
    outPitch = 0.0f;

    if (!s_gyroConfig.isEnabled || !s_capabilities.hasGyro || !s_gyroState.isValid)
        return;

    // Initialize timing on first call
    if (!s_gyroProcessing.isInitialized)
    {
        QueryPerformanceFrequency(&s_gyroProcessing.frequency);
        QueryPerformanceCounter(&s_gyroProcessing.lastFrameTime);
        s_gyroProcessing.isInitialized = true;
    }

    // Calculate delta time
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    float deltaTime = static_cast<float>(currentTime.QuadPart - s_gyroProcessing.lastFrameTime.QuadPart) / static_cast<float>(s_gyroProcessing.frequency.QuadPart);
    s_gyroProcessing.lastFrameTime = currentTime;
    deltaTime = std::clamp(deltaTime, 0.0001f, 0.1f);

    float rawYaw = -s_gyroState.y;
    float rawPitch = -s_gyroState.x;

    if (s_gyroConfig.invertY) rawPitch = -rawPitch;

    float smoothFactor = expf(-deltaTime / fmaxf(s_gyroConfig.smoothing, 0.001f));
    s_gyroProcessing.smoothedYaw = rawYaw + (s_gyroProcessing.smoothedYaw - rawYaw) * smoothFactor;
    s_gyroProcessing.smoothedPitch = rawPitch + (s_gyroProcessing.smoothedPitch - rawPitch) * smoothFactor;

    outYaw = s_gyroProcessing.smoothedYaw * deltaTime * s_gyroConfig.sensitivity;
    outPitch = s_gyroProcessing.smoothedPitch * deltaTime * s_gyroConfig.sensitivity;
}

// ==========================================================
// Internal Helpers
// ==========================================================

static bool IsInTransitionPeriod()
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
    for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
    {
        auto& btnState = g_Controller.gameButtons[i];
        if (!btnState.isPressed)
            continue;

        int commandId = s_buttonCommands[i];
        int simulatedKey = GetSimulatedKey(commandId);

        if (simulatedKey != 0)
        {
            PostMessage(g_State.hWnd, WM_KEYUP, simulatedKey, 0);
        }
        else if (commandId != 0)
        {
            g_Controller.commandActive[commandId] = false;
            OnCommandOff(g_State.g_pGameClientShell, commandId);
        }

        btnState = {};
    }

    for (int i = 0; i < 2; i++)
    {
        auto& btnState = g_Controller.triggerButtons[i];
        if (!btnState.isPressed)
            continue;

        int commandId = s_triggerCommands[i];
        int simulatedKey = GetSimulatedKey(commandId);

        if (simulatedKey != 0)
        {
            PostMessage(g_State.hWnd, WM_KEYUP, simulatedKey, 0);
        }
        else if (commandId != 0)
        {
            g_Controller.commandActive[commandId] = false;
            OnCommandOff(g_State.g_pGameClientShell, commandId);
        }

        btnState = {};
    }
}

static bool IsStickDirectionPressed(int direction)
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
// Input Mode Tracking
// ==========================================================

static void UpdateInputMode(bool usingController)
{
    if (g_Controller.usingControllerInput != usingController)
    {
        g_Controller.usingControllerInput = usingController;
        NotifyHUDConnectionChange();
    }
}

void OnKeyboardMouseInput()
{
    if (g_Controller.usingControllerInput && g_Controller.isConnected)
    {
        UpdateInputMode(false);
    }
}

bool ShouldShowControllerPrompts()
{
    return g_Controller.isConnected && g_Controller.usingControllerInput;
}

// ==========================================================
// Processing - Touchpad
// ==========================================================

static void ProcessTouchpadMouse()
{
    if (!s_pGamepad || !s_capabilities.hasTouchpad || !TouchpadEnabled)
        return;

    for (int touchpadIndex = 0; touchpadIndex < 2; touchpadIndex++)
    {
        bool fingerDown = false;
        float x = 0.0f, y = 0.0f, pressure = 0.0f;

        if (SDL_GetGamepadTouchpadFinger(s_pGamepad, touchpadIndex, 0, &fingerDown, &x, &y, &pressure))
        {
            if (fingerDown)
            {
                if (s_touchpadFinger[touchpadIndex].wasDown)
                {
                    float deltaX = (x - s_touchpadFinger[touchpadIndex].lastX) * g_TouchpadConfig.currentWidth;
                    float deltaY = (y - s_touchpadFinger[touchpadIndex].lastY) * g_TouchpadConfig.currentHeight;

                    if (deltaX != 0.0f || deltaY != 0.0f)
                    {
                        UpdateInputMode(true);

                        INPUT input = {};
                        input.type = INPUT_MOUSE;
                        input.mi.dwFlags = MOUSEEVENTF_MOVE;
                        input.mi.dx = static_cast<LONG>(deltaX);
                        input.mi.dy = static_cast<LONG>(deltaY);
                        SendInput(1, &input, sizeof(INPUT));
                    }
                }

                s_touchpadFinger[touchpadIndex].lastX = x;
                s_touchpadFinger[touchpadIndex].lastY = y;
                s_touchpadFinger[touchpadIndex].wasDown = true;
            }
            else
            {
                s_touchpadFinger[touchpadIndex].wasDown = false;
            }
        }
    }
}

static void ProcessTouchpadClick()
{
    if (!s_pGamepad || !s_capabilities.hasTouchpad || !TouchpadEnabled)
        return;

    bool isPressed = SDL_GetGamepadButton(s_pGamepad, SDL_GAMEPAD_BUTTON_TOUCHPAD);

    if (isPressed && !s_wasTouchpadPressed[0])
    {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
    }
    else if (!isPressed && s_wasTouchpadPressed[0])
    {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    s_wasTouchpadPressed[0] = isPressed;
}

static void ReleaseTouchpadClick()
{
    for (int i = 0; i < 2; i++)
    {
        if (s_wasTouchpadPressed[i])
        {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &input, sizeof(INPUT));
            s_wasTouchpadPressed[i] = false;
        }
    }
}

// ==========================================================
// Configuration - Mappings
// ==========================================================

void ConfigureGamepadMappings(int btnSouth, int btnEast, int btnWest, int btnNorth, int btnLeftStick, int btnRightStick, int btnLeftShoulder, int btnRightShoulder, int btnDpadUp, int btnDpadDown, int btnDpadLeft, int btnDpadRight, int btnBack, int btnStart, int axisLeftTrigger, int axisRightTrigger, int btnMisc1, int btnRightPaddle1, int btnLeftPaddle1, int btnRightPaddle2, int btnLeftPaddle2)
{
    s_buttonCommands[SDL_GAMEPAD_BUTTON_SOUTH] = btnSouth;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_EAST] = btnEast;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_WEST] = btnWest;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_NORTH] = btnNorth;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_LEFT_STICK] = btnLeftStick;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_RIGHT_STICK] = btnRightStick;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER] = btnLeftShoulder;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER] = btnRightShoulder;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_DPAD_UP] = btnDpadUp;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = btnDpadDown;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = btnDpadLeft;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = btnDpadRight;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_BACK] = btnBack;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_START] = btnStart;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_MISC1] = btnMisc1;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1] = btnRightPaddle1;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_LEFT_PADDLE1] = btnLeftPaddle1;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2] = btnRightPaddle2;
    s_buttonCommands[SDL_GAMEPAD_BUTTON_LEFT_PADDLE2] = btnLeftPaddle2;

    s_triggerCommands[0] = axisLeftTrigger;
    s_triggerCommands[1] = axisRightTrigger;
}

// ==========================================================
// Button Names
// ==========================================================

const wchar_t* GetGamepadButtonName(int commandId, bool shortName)
{
    const ButtonNameSet& names = GetButtonNameSet();

    for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
    {
        if (s_buttonCommands[i] == commandId && names.buttons[i].shortName != nullptr)
        {
            return shortName ? names.buttons[i].shortName : names.buttons[i].longName;
        }
    }

    for (int i = 0; i < 2; i++)
    {
        if (s_triggerCommands[i] == commandId)
        {
            return shortName ? names.triggers[i].shortName : names.triggers[i].longName;
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
        ReleaseTouchpadClick();
        ResetControllerState();

        // Try to connect to another available gamepad
        int count = 0;
        SDL_JoystickID* gamepads = SDL_GetGamepads(&count);
        if (gamepads)
        {
            for (int i = 0; i < count; i++)
            {
                s_pGamepad = SDL_OpenGamepad(gamepads[i]);
                if (s_pGamepad)
                {
                    LoadGamepadCapabilities(s_pGamepad);
                    break;
                }
            }

            SDL_free(gamepads);
        }
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

    int simulatedKey = GetSimulatedKey(commandId);

    // Button just pressed
    if (isPressed && !btnState.isPressed)
    {
        UpdateInputMode(true);

        if (simulatedKey != 0)
        {
            g_Controller.simulatedKeyPressCount++;
            PostMessage(g_State.hWnd, WM_KEYDOWN, simulatedKey, 0);
        }
        else
        {
            g_Controller.commandActive[commandId] = true;
            OnCommandOn(g_State.g_pGameClientShell, commandId);
        }
        btnState.wasHandled = true;
        btnState.pressStartTime = GetTickCount64();
    }
    // Button held
    else if (isPressed)
    {
        if (simulatedKey == 0)
        {
            g_Controller.commandActive[commandId] = true;
        }
    }
    // Button just released
    else if (!isPressed && btnState.isPressed)
    {
        if (simulatedKey != 0)
        {
            PostMessage(g_State.hWnd, WM_KEYUP, simulatedKey, 0);
        }
        else
        {
            g_Controller.commandActive[commandId] = false;
            OnCommandOff(g_State.g_pGameClientShell, commandId);
        }
        btnState.wasHandled = false;
    }

    btnState.isPressed = isPressed;
}

static void HandleGamepadTrigger(int triggerIndex, SDL_GamepadAxis axis)
{
    int commandId = s_triggerCommands[triggerIndex];
    auto& btnState = g_Controller.triggerButtons[triggerIndex];

    Sint16 value = SDL_GetGamepadAxis(s_pGamepad, axis);
    bool isPressed = value > TRIGGER_THRESHOLD;

    if (IsInTransitionPeriod())
    {
        btnState.isPressed = isPressed;
        return;
    }

    if (isPressed && !btnState.isPressed)
    {
        UpdateInputMode(true);
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

void ProcessGameplayInput()
{
    memset(g_Controller.commandActive, 0, sizeof(g_Controller.commandActive));

    for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
    {
        int commandId = s_buttonCommands[i];
        if (commandId != 0)
        {
            HandleGamepadButton(static_cast<SDL_GamepadButton>(i), commandId);
        }
    }

    HandleGamepadTrigger(0, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    HandleGamepadTrigger(1, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
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
                    g_Controller.simulatedKeyPressCount++;
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
            if (pressed)
                g_Controller.simulatedKeyPressCount++;

            PostMessage(g_State.hWnd, pressed ? WM_KEYDOWN : WM_KEYUP, menuNavigation[i].vkey, 0);
            btnState.isPressed = pressed;

            if (pressed)
            {
                UpdateInputMode(true);
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
    UpdateFrameTiming();
    ProcessSDLEvents();

    // Update connection state
    bool wasConnected = g_Controller.isConnected;
    g_Controller.isConnected = IsControllerConnected();

    if (wasConnected != g_Controller.isConnected)
    {
        if (!g_Controller.isConnected)
        {
            g_Controller.usingControllerInput = false;
        }

        NotifyHUDConnectionChange();
    }

    if (g_State.isConsoleOpen)
    {
        memset(g_Controller.commandActive, 0, sizeof(g_Controller.commandActive));
        return;
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