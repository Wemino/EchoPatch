class FpsLimiter
{
public:
    FpsLimiter(float targetFps = 240.0f) : initialized(false)
    {
        timeBeginPeriod(1);
        QueryPerformanceFrequency(&frequency);
        SetTargetFps(targetFps);

        sleepThreshold = (frequency.QuadPart * 2) / 1000;

        lastTime.QuadPart = 0;
    }

    ~FpsLimiter()
    {
        timeEndPeriod(1);
    }

    void SetTargetFps(float targetFps)
    {
        targetFrameTicks = static_cast<LONGLONG>(static_cast<double>(frequency.QuadPart) / targetFps);
    }

    void Limit()
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        if (!initialized)
        {
            lastTime = currentTime;
            initialized = true;
            return;
        }

        LONGLONG targetTick = lastTime.QuadPart + targetFrameTicks;

        if (currentTime.QuadPart < targetTick)
        {
            LONGLONG remaining = targetTick - currentTime.QuadPart;

            if (remaining > sleepThreshold)
            {
                DWORD sleepMs = static_cast<DWORD>(((remaining - sleepThreshold) * 1000) / frequency.QuadPart);
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

            lastTime.QuadPart = targetTick;
        }
        else
        {
            LONGLONG overshoot = currentTime.QuadPart - targetTick;

            if (overshoot < targetFrameTicks)
            {
                lastTime.QuadPart = targetTick;
            }
            else
            {
                lastTime = currentTime;
            }
        }
    }

private:
    LARGE_INTEGER frequency;
    LARGE_INTEGER lastTime;
    LONGLONG targetFrameTicks;
    LONGLONG sleepThreshold;
    bool initialized;
};