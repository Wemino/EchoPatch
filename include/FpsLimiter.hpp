#include <Windows.h>

class FpsLimiter
{
public:
    explicit FpsLimiter(double targetFps = 300.0) noexcept
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        frequency = freq.QuadPart;
        ticksTo100ns = 10000000.0 / static_cast<double>(frequency);

        SetTargetFps(targetFps);

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        lastTargetTick = now.QuadPart;

        // Default to legacy Sleep implementation
        limitFunc = &FpsLimiter::LimitLegacy;

        // Try to upgrade to waitable timer
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        if (kernel32)
        {
            auto pCreateWaitableTimerExW = reinterpret_cast<decltype(&CreateWaitableTimerExW)>(GetProcAddress(kernel32, "CreateWaitableTimerExW"));

            if (pCreateWaitableTimerExW)
            {
                // Try high-resolution timer (Windows 10 1803+)
                hTimer = pCreateWaitableTimerExW(nullptr, nullptr, 0x00000002, TIMER_ALL_ACCESS);

                if (hTimer)
                {
                    sleepThreshold = frequency / 2000;
                    limitFunc = &FpsLimiter::LimitWaitableTimer;
                }
                else
                {
                    // Try standard waitable timer (Vista+)
                    hTimer = pCreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS);
                    if (hTimer)
                    {
                        sleepThreshold = (frequency * 2) / 1000;
                        limitFunc = &FpsLimiter::LimitWaitableTimer;
                    }
                }
            }
        }

        // Only change global system timer resolution if we need legacy Sleep
        if (limitFunc == &FpsLimiter::LimitLegacy)
        {
            timeBeginPeriod(1);
            usedTimeBeginPeriod = true;
            sleepThreshold = (frequency * 2) / 1000;
        }
    }

    ~FpsLimiter() noexcept
    {
        if (hTimer)
            CloseHandle(hTimer);

        if (usedTimeBeginPeriod)
            timeEndPeriod(1);
    }

    FpsLimiter(const FpsLimiter&) = delete;
    FpsLimiter& operator=(const FpsLimiter&) = delete;

    void SetTargetFps(double targetFps) noexcept
    {
        if (targetFps <= 0.0)
            targetFps = 1.0;

        targetFrameTicks = static_cast<LONGLONG>(static_cast<double>(frequency) / targetFps);
    }

    void Limit() noexcept
    {
        (this->*limitFunc)();
    }

private:
    // Waitable timer implementation (Vista+, best precision on Windows 10 1803+)
    void LimitWaitableTimer() noexcept
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        const LONGLONG targetTick = lastTargetTick + targetFrameTicks;
        const LONGLONG remaining = targetTick - currentTime.QuadPart;

        if (remaining > 0)
        {
            // Use waitable timer for bulk of wait, spin for final precision
            if (remaining > sleepThreshold)
            {
                LARGE_INTEGER dueTime;
                dueTime.QuadPart = -static_cast<LONGLONG>((remaining - sleepThreshold) * ticksTo100ns);

                if (SetWaitableTimer(hTimer, &dueTime, 0, nullptr, nullptr, FALSE))
                {
                    WaitForSingleObject(hTimer, INFINITE);
                }
            }

            do
            {
                YieldProcessor();
                QueryPerformanceCounter(&currentTime);
            } 
            while (currentTime.QuadPart < targetTick);
        }

        // Advance by fixed amount to maintain average FPS
        lastTargetTick = targetTick;

        // If more than 1 frame behind, reset to prevent catch-up spiral
        if (currentTime.QuadPart - targetTick > targetFrameTicks)
        {
            lastTargetTick = currentTime.QuadPart;
        }
    }

    // Legacy Sleep implementation (Windows XP+)
    void LimitLegacy() noexcept
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        const LONGLONG targetTick = lastTargetTick + targetFrameTicks;
        const LONGLONG remaining = targetTick - currentTime.QuadPart;

        if (remaining > 0)
        {
            if (remaining > sleepThreshold)
            {
                DWORD sleepMs = static_cast<DWORD>((remaining - sleepThreshold) * 1000 / frequency);
                if (sleepMs > 0)
                {
                    Sleep(sleepMs);
                }
            }

            do
            {
                YieldProcessor();
                QueryPerformanceCounter(&currentTime);
            } 
            while (currentTime.QuadPart < targetTick);
        }

        lastTargetTick = targetTick;

        if (currentTime.QuadPart - targetTick > targetFrameTicks)
        {
            lastTargetTick = currentTime.QuadPart;
        }
    }

    HANDLE hTimer = nullptr;
    LONGLONG frequency = 0;
    LONGLONG lastTargetTick = 0;
    LONGLONG targetFrameTicks = 0;
    LONGLONG sleepThreshold = 0;
    double ticksTo100ns = 0.0;
    bool usedTimeBeginPeriod = false;
    void (FpsLimiter::* limitFunc)() noexcept = nullptr;
};