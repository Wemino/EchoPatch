class FpsLimiter
{
public:
    FpsLimiter(float targetFps = 120.0f) : initialized(false)
    {
        SetTargetFps(targetFps);
        QueryPerformanceFrequency(&frequency);
        ticksToMillis = 1000.0 / static_cast<double>(frequency.QuadPart);
        lastTime.QuadPart = 0;
    }

    void SetTargetFps(float targetFps)
    {
        targetFrameTimeMillis = 1000.0 / targetFps;
    }

    void Limit()
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        if (!initialized)
        {
            lastTime = currentTime;
            initialized = true;
            return; // Skip limiting on the first call
        }

        // Calculate elapsed time in ticks and milliseconds
        LONGLONG elapsedTicks = currentTime.QuadPart - lastTime.QuadPart;
        double elapsedTimeMillis = static_cast<double>(elapsedTicks) * ticksToMillis;

        if (elapsedTimeMillis < targetFrameTimeMillis)
        {
            // Calculate remaining time and handle sleep
            double remainingTimeMillis = targetFrameTimeMillis - elapsedTimeMillis;
            DWORD sleepMillis = static_cast<DWORD>(remainingTimeMillis - 1.0);

            if (sleepMillis > 1)
            {
                Sleep(sleepMillis);
            }

            // Calculate target tick count for spin-yield
            LONGLONG targetTicks = lastTime.QuadPart + static_cast<LONGLONG>((targetFrameTimeMillis / 1000.0) * frequency.QuadPart);

            // Spin-yield until target time is reached
            do
            {
                Sleep(0);
                QueryPerformanceCounter(&currentTime);
            } 
            while (currentTime.QuadPart < targetTicks);
        }

        lastTime = currentTime;
    }

private:
    LARGE_INTEGER frequency;
    LARGE_INTEGER lastTime;
    double targetFrameTimeMillis;
    double ticksToMillis;
    bool initialized;
};