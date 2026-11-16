#pragma once

#include <ctime>
#include "../../Globals.cpp"

tm*(__cdecl* LocalTime)(const int*) = nullptr;

static tm saveLocalTime;

// ===========
// FixYear2038
// ===========

static tm* __cdecl SaveLocalTime(const int* pTime)
{
    // Read the save time as unsigned to keep it valid until 2106
    if (*pTime >= 0)
        return LocalTime(pTime);

    __time64_t time = static_cast<uint32_t>(*pTime);
    if (_localtime64_s(&saveLocalTime, &time) != 0)
        return nullptr;

    return &saveLocalTime;
}
