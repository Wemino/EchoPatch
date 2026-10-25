#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "EngineInterfaces.cpp"
#include "ConsoleMgr.hpp"

void(__cdecl* UpdateServerObject)(DWORD) = nullptr;

// =======================
// ConsoleClassTicks
// =======================

namespace ClassTicks
{
    // The panel shows the last second and changes once per second, so it can be read
    constexpr double kRefreshSeconds = 1.0;

    struct ClassTime
    {
        std::string name;
        double frameTime = 0.0;
        uint32_t frameObjects = 0;
        double windowTime = 0.0;
        double windowPeak = 0.0;
        uint32_t windowObjects = 0;
        double totalTime = 0.0;
    };

    // Values shown by the panel: objects is the most in one frame, time is per frame
    struct Row
    {
        std::string name;
        uint32_t objects = 0;
        double time = 0.0;
        double peak = 0.0;
        double average = 0.0;
    };

    static bool isTiming = false;
    static double ticksToMs = 0.0;
    static std::unordered_map<DWORD, ClassTime> classes;

    // Totals of the frame, of the current second and since the timing started
    static double frameTime = 0.0;
    static uint32_t frameObjects = 0;
    static double windowTime = 0.0;
    static double windowPeak = 0.0;
    static uint32_t windowObjects = 0;
    static uint32_t windowFrames = 0;
    static LARGE_INTEGER windowStart = {};
    static double totalTime = 0.0;
    static double peakTime = 0.0;
    static uint32_t frames = 0;

    // Objects of the class given by ShowClassTicksSpecific, time of each one in the current second
    static std::string specificName;
    static DWORD specificClass = 0;
    static std::unordered_map<DWORD, double> specificTimes;

    // Last refresh
    static bool hasSnapshot = false;
    static Row shownTotal;
    static std::vector<Row> shownClasses;
    static std::vector<std::pair<std::string, double>> shownSpecific;

    static int showClassTicks = 0;
    static int showTickCounts = 0;

    // Engine variable: +0x04 float, +0x08 int, +0x10 string
    inline int ReadEngineInt(const char* name)
    {
        DWORD state = GetClientManager();
        DWORD var = state ? FindEngineVar(state, name) : 0;
        if (!var)
            return 0;

        if (int* pInt = *(int**)(var + 0x08))
            return *pInt;

        if (float* pFloat = *(float**)(var + 0x04))
            return (int)*pFloat;

        return 0;
    }

    inline std::string ReadEngineString(const char* name)
    {
        DWORD state = GetClientManager();
        DWORD var = state ? FindEngineVar(state, name) : 0;
        char** pString = var ? *(char***)(var + 0x10) : nullptr;
        return pString && *pString ? *pString : "";
    }

    inline void ResetWindow()
    {
        for (auto& [hClass, entry] : classes)
        {
            entry.windowTime = entry.windowPeak = 0.0;
            entry.windowObjects = 0;
        }

        windowTime = windowPeak = 0.0;
        windowObjects = windowFrames = 0;
        specificTimes.clear();
        QueryPerformanceCounter(&windowStart);
    }

    inline void ResetTotals()
    {
        classes.clear();
        frameTime = totalTime = peakTime = 0.0;
        frameObjects = frames = 0;
        hasSnapshot = false;
        shownClasses.clear();
        shownSpecific.clear();
        ResetWindow();
    }

    inline void Record(DWORD hObject, double time)
    {
        frameTime += time;
        frameObjects++;

        DWORD hClass = EngineInterface::CallServerMember<DWORD>(EngineInterface::Server_GetObjectClass, hObject);
        if (!hClass)
            return;

        ClassTime& entry = classes[hClass];
        if (entry.name.empty())
        {
            entry.name = EngineInterface::GetClassName(hClass);
            if (entry.name.empty())
            {
                entry.name = "?";
            }
        }

        entry.frameTime += time;
        entry.frameObjects++;

        if (specificClass && hClass == specificClass)
        {
            specificTimes[hObject] += time;
        }
    }

    // Values of the second that ended, sorted by time as shown so equal values keep their order
    inline void TakeSnapshot()
    {
        const double frameCount = (double)std::max(windowFrames, 1u);

        shownTotal = { "", windowObjects, windowTime / frameCount, windowPeak, frames ? totalTime / frames : 0.0 };

        shownClasses.clear();
        for (const auto& [hClass, entry] : classes)
        {
            if (entry.windowObjects > 0)
            {
                shownClasses.push_back({ entry.name, entry.windowObjects, entry.windowTime / frameCount, entry.windowPeak, frames ? entry.totalTime / frames : 0.0 });
            }
        }

        std::sort(shownClasses.begin(), shownClasses.end(), [](const Row& a, const Row& b)
        {
            long long timeA = llround(a.time * 1000.0);
            long long timeB = llround(b.time * 1000.0);
            return timeA != timeB ? timeA > timeB : _stricmp(a.name.c_str(), b.name.c_str()) < 0;
        });

        // Names of the slowest objects of the class
        std::vector<std::pair<DWORD, double>> objects(specificTimes.begin(), specificTimes.end());
        std::sort(objects.begin(), objects.end(), [](const auto& a, const auto& b)
        {
            long long timeA = llround(a.second * 1000.0);
            long long timeB = llround(b.second * 1000.0);
            return timeA != timeB ? timeA > timeB : a.first < b.first;
        });

        shownSpecific.clear();
        for (size_t i = 0; i < objects.size() && i < 20; i++)
        {
            std::string name = EngineInterface::GetObjectName(objects[i].first);
            shownSpecific.push_back({ name.empty() ? "(no name)" : name, objects[i].second / frameCount });
        }

        hasSnapshot = true;
    }

    // Called once per frame, the server updated its objects since the last call
    inline void Update()
    {
        showClassTicks = ReadEngineInt("ShowClassTicks");
        showTickCounts = ReadEngineInt("ShowTickCounts");

        bool timing = (showClassTicks > 0 || showTickCounts > 0) && EngineInterface::IsServerRunning();
        if (timing != isTiming)
        {
            isTiming = timing;
            ResetTotals();
        }

        if (!isTiming)
            return;

        // The class of ShowClassTicksSpecific is found again when the name changes or the server restarts
        std::string specific = showClassTicks == 2 ? ReadEngineString("ShowClassTicksSpecific") : "";
        if (specific != specificName || (!specific.empty() && !specificClass))
        {
            specificName = specific;
            specificClass = specific.empty() ? 0 : EngineInterface::CallServerMember<DWORD>(EngineInterface::Server_GetClass, specific.c_str());
            specificTimes.clear();
        }

        if (frameObjects > 0)
        {
            totalTime += frameTime;
            peakTime = std::max(peakTime, frameTime);
            frames++;

            windowTime += frameTime;
            windowPeak = std::max(windowPeak, frameTime);
            windowObjects = std::max(windowObjects, frameObjects);
            windowFrames++;

            for (auto& [hClass, entry] : classes)
            {
                if (entry.frameObjects > 0)
                {
                    entry.totalTime += entry.frameTime;
                    entry.windowTime += entry.frameTime;
                    entry.windowPeak = std::max(entry.windowPeak, entry.frameTime);
                    entry.windowObjects = std::max(entry.windowObjects, entry.frameObjects);
                }
                entry.frameTime = 0.0;
                entry.frameObjects = 0;
            }
        }

        frameTime = 0.0;
        frameObjects = 0;

        LARGE_INTEGER now, frequency;
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&frequency);
        if (windowFrames > 0 && (double)(now.QuadPart - windowStart.QuadPart) / (double)frequency.QuadPart >= kRefreshSeconds)
        {
            TakeSnapshot();
            ResetWindow();
        }
    }

    // Overlay
    inline bool IsActive()
    {
        return isTiming && hasSnapshot;
    }

    inline void Render()
    {
        if (!BeginOverlayPanel("##ClassTicksPanel", 1, showClassTicks > 0 ? "Object updates by class" : "Object updates"))
            return;

        ImGui::Text("%u objects  %.3f ms", shownTotal.objects, shownTotal.time);
        ImGui::TextDisabled("Per frame in the last second, peak %.3f ms", shownTotal.peak);
        ImGui::TextDisabled("Average %.3f ms since the start", shownTotal.average);

        if (showClassTicks == 2 && !specificName.empty())
        {
            ImGui::Spacing();
            if (!specificClass)
            {
                ImGui::TextColored(g_warningLineColor, "No class named %s", specificName.c_str());
            }
            else if (shownSpecific.empty())
            {
                ImGui::TextDisabled("No %s updated in the last second", specificName.c_str());
            }
            else if (ImGui::BeginTable("##Objects", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn(specificName.c_str());
                ImGui::TableSetupColumn("ms", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();
                for (const auto& [name, time] : shownSpecific)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(name.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", time);
                }
                ImGui::EndTable();
            }
        }
        else if (showClassTicks > 0)
        {
            ImGui::Spacing();
            if (ImGui::BeginTable("##Classes", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("Class");
                ImGui::TableSetupColumn("Objects");
                ImGui::TableSetupColumn("ms");
                ImGui::TableSetupColumn("Peak");
                ImGui::TableSetupColumn("Average");
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < shownClasses.size() && i < 25; i++)
                {
                    const Row& row = shownClasses[i];
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(row.name.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%u", row.objects);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.3f", row.time);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.3f", row.peak);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%.3f", row.average);
                }
                ImGui::EndTable();
            }
        }

        EndOverlayPanel();
    }
}

// Times one object update of the server
static void __cdecl UpdateServerObject_Hook(DWORD hObject)
{
    if (!ClassTicks::isTiming)
    {
        UpdateServerObject(hObject);
        return;
    }

    LARGE_INTEGER start, end;
    QueryPerformanceCounter(&start);
    UpdateServerObject(hObject);
    QueryPerformanceCounter(&end);

    ClassTicks::Record(hObject, (double)(end.QuadPart - start.QuadPart) * ClassTicks::ticksToMs);
}

static void ApplyClassTicksHook()
{
    DWORD addr = GetAddress(Addr::Console_UpdateObject);
    if (addr == 0) return;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    ClassTicks::ticksToMs = 1000.0 / (double)frequency.QuadPart;

    HookHelper::ApplyHook((void*)addr, &UpdateServerObject_Hook, (LPVOID*)&UpdateServerObject, g_State.CurrentFEARGame == FEAR);
    Console::AddUpdateCallback(&ClassTicks::Update);
    Console::AddOverlay(&ClassTicks::IsActive, &ClassTicks::Render);
}
