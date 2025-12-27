class FpsLimiter
{
public:
    explicit FpsLimiter(double targetFps = 240.0)
    {
        timeBeginPeriod(1);
        QueryPerformanceFrequency(&frequency);
        sleepThreshold = (frequency.QuadPart * 2) / 1000;
        SetTargetFps(targetFps);
        lastTargetTick = 0;
    }

    ~FpsLimiter()
    {
        timeEndPeriod(1);
    }

    void SetTargetFps(double targetFps)
    {
        targetFrameTicks = static_cast<LONGLONG>(static_cast<double>(frequency.QuadPart) / targetFps);
    }

    void Limit()
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        if (lastTargetTick == 0)
        {
            lastTargetTick = currentTime.QuadPart;
            return;
        }

        LONGLONG targetTick = lastTargetTick + targetFrameTicks;
        LONGLONG remaining = targetTick - currentTime.QuadPart;

        if (remaining > 0)
        {
            // Sleep the bulk of the wait (preserves precision: multiply before divide)
            if (remaining > sleepThreshold)
            {
                DWORD sleepMs = static_cast<DWORD>(((remaining - sleepThreshold) * 1000) / frequency.QuadPart);
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
    LARGE_INTEGER frequency;
    LONGLONG lastTargetTick;
    LONGLONG targetFrameTicks;
    LONGLONG sleepThreshold;
};