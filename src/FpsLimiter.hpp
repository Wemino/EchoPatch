#include <windows.h>
#include <mmsystem.h>

class FpsLimiter
{
public:
    FpsLimiter(float targetFps = 120.0f) : initialized(false)
    {
        timeBeginPeriod(1);

        SetTargetFps(targetFps);
        QueryPerformanceFrequency(&frequency);
        ticksToMillis = 1000.0 / static_cast<double>(frequency.QuadPart);
        lastTime.QuadPart = 0;
    }

    ~FpsLimiter()
    {
        timeEndPeriod(1);
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
            return;
        }

        // Calculate elapsed time in ticks and milliseconds
        LONGLONG elapsedTicks = currentTime.QuadPart - lastTime.QuadPart;
        double elapsedTimeMillis = static_cast<double>(elapsedTicks) * ticksToMillis;

        if (elapsedTimeMillis < targetFrameTimeMillis)
        {
            // Calculate remaining time in milliseconds
            double remainingTimeMillis = targetFrameTimeMillis - elapsedTimeMillis;

            // Sleep for almost all of the remaining time (if >1ms)
            DWORD sleepMillis = (remainingTimeMillis > 1.0) ? static_cast<DWORD>(remainingTimeMillis - 1.0) : 0;
            if (sleepMillis > 0)
            {
                Sleep(sleepMillis);
            }

            // Determine target tick count corresponding to the frame time
            LONGLONG targetTicks = lastTime.QuadPart + static_cast<LONGLONG>((targetFrameTimeMillis / 1000.0) * frequency.QuadPart);

            // Spin-yield until we reach the target time.
            do
            {
                Sleep(0);
                QueryPerformanceCounter(&currentTime);
            } 
            while (currentTime.QuadPart < targetTicks);
        }

        // Update lastTime for the next frame.
        lastTime = currentTime;
    }

private:
    LARGE_INTEGER frequency;
    LARGE_INTEGER lastTime;
    double targetFrameTimeMillis;
    double ticksToMillis;
    bool initialized;
};