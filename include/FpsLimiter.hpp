class FpsLimiter
{
public:
    explicit FpsLimiter(double targetFps = 240.0)
    {
        timeBeginPeriod(1);

        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        frequency = freq.QuadPart;

        ticksTo100ns = 10000000.0 / static_cast<double>(frequency);

        SetTargetFps(targetFps);

        QueryPerformanceCounter(&freq);
        lastTargetTick = freq.QuadPart;

        // Default to legacy Sleep implementation (2ms threshold)
        sleepThreshold = (frequency * 2) / 1000;
        limitFunc = &FpsLimiter::LimitLegacy;

        // Try to upgrade to waitable timer (Vista+)
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
                    // High-res timer is precise, 1ms threshold is safe
                    sleepThreshold = frequency / 1000;
                    limitFunc = &FpsLimiter::LimitWaitableTimer;
                }
                else
                {
                    // Try standard waitable timer (Vista/7/8)
                    hTimer = pCreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS);

                    if (hTimer)
                    {
                        // Standard timer has 1ms resolution, keep 2ms threshold
                        sleepThreshold = (frequency * 2) / 1000;
                        limitFunc = &FpsLimiter::LimitWaitableTimer;
                    }
                }
            }
        }
    }

    ~FpsLimiter()
    {
        if (hTimer)
        {
            CloseHandle(hTimer);
        }

        timeEndPeriod(1);
    }

    void SetTargetFps(double targetFps)
    {
        targetFrameTicks = static_cast<LONGLONG>(static_cast<double>(frequency) / targetFps);
    }

    void Limit()
    {
        (this->*limitFunc)();
    }

private:
    // Waitable timer implementation (Vista+, best precision on Windows 10 1803+)
    void LimitWaitableTimer()
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        const LONGLONG targetTick = lastTargetTick + targetFrameTicks;
        const LONGLONG remaining = targetTick - currentTime.QuadPart;

        if (remaining > 0)
        {
            // Use waitable timer for bulk of wait
            if (remaining > sleepThreshold)
            {
                LARGE_INTEGER dueTime;
                dueTime.QuadPart = -static_cast<LONGLONG>((remaining - sleepThreshold) * ticksTo100ns);

                if (SetWaitableTimer(hTimer, &dueTime, 0, nullptr, nullptr, FALSE))
                {
                    WaitForSingleObject(hTimer, INFINITE);
                }
            }

            // Spin for final precision
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
    void LimitLegacy()
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        const LONGLONG targetTick = lastTargetTick + targetFrameTicks;
        const LONGLONG remaining = targetTick - currentTime.QuadPart;

        if (remaining > 0)
        {
            // Sleep the bulk of the wait
            if (remaining > sleepThreshold)
            {
                DWORD sleepMs = static_cast<DWORD>((remaining - sleepThreshold) * 1000 / frequency);
                if (sleepMs > 0)
                {
                    Sleep(sleepMs);
                }
            }

            // Spin for final precision
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

private:
    HANDLE hTimer = nullptr;
    LONGLONG frequency;
    LONGLONG lastTargetTick;
    LONGLONG targetFrameTicks;
    LONGLONG sleepThreshold;
    double ticksTo100ns;
    void (FpsLimiter::* limitFunc)();
};