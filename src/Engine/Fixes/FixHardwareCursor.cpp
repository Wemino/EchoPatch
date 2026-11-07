#pragma once

#include <algorithm>
#include <vector>
#include "../../Globals.cpp"
#include "../../Addresses.cpp"

// ===========================
// FixHardwareCursor
// ===========================

int(__stdcall* LoadCursorBitmapResource)(const char*, HCURSOR**) = nullptr;

static HCURSOR FixCursorTransparency(HCURSOR hCursor)
{
    ICONINFO iconInfo{};
    if (!GetIconInfo(hCursor, &iconInfo)) return hCursor;

    HCURSOR result = hCursor;
    BITMAP maskBitmap{};

    if (GetObject(iconInfo.hbmMask, sizeof(maskBitmap), &maskBitmap))
    {
        int width = maskBitmap.bmWidth;
        int maskHeight = maskBitmap.bmHeight;
        int height = iconInfo.hbmColor ? maskHeight : maskHeight / 2;

        struct { BITMAPINFOHEADER header; RGBQUAD colors[2]; } maskInfo{};
        maskInfo.header = { sizeof(BITMAPINFOHEADER), width, -maskHeight, 1, 1, BI_RGB };
        int maskStride = (width + 31) / 32 * 4;
        std::vector<BYTE> maskBits(maskStride * maskHeight);

        BITMAPINFO colorInfo{};
        colorInfo.bmiHeader = { sizeof(BITMAPINFOHEADER), width, -height, 1, 32, BI_RGB };
        std::vector<DWORD> pixels(width * height);

        HDC hdc = GetDC(nullptr);
        bool hasBits = GetDIBits(hdc, iconInfo.hbmMask, 0, maskHeight, maskBits.data(), reinterpret_cast<BITMAPINFO*>(&maskInfo), DIB_RGB_COLORS) == maskHeight && (!iconInfo.hbmColor || GetDIBits(hdc, iconInfo.hbmColor, 0, height, pixels.data(), &colorInfo, DIB_RGB_COLORS) == height);
        ReleaseDC(nullptr, hdc);

        if (hasBits)
        {
            auto isMaskSet = [&](int x, int y) { return (maskBits[y * maskStride + x / 8] & (0x80 >> (x % 8))) != 0; };
            bool hasAlpha = std::any_of(pixels.begin(), pixels.end(), [](DWORD pixel) { return (pixel >> 24) != 0; });

            int fixedMaskStride = (width + 15) / 16 * 2;
            std::vector<BYTE> fixedMaskBits(fixedMaskStride * height, 0);

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    DWORD& pixel = pixels[y * width + x];

                    if (!iconInfo.hbmColor)
                    {
                        pixel = isMaskSet(x, y + height) ? 0xFFFFFF : 0;
                    }

                    if (hasAlpha ? (pixel >> 24) == 0 : isMaskSet(x, y))
                    {
                        pixel = 0;
                        fixedMaskBits[y * fixedMaskStride + x / 8] |= 0x80 >> (x % 8);
                    }
                    else if (!hasAlpha)
                    {
                        pixel |= 0xFF000000;
                    }
                }
            }

            void* colorBits = nullptr;
            HBITMAP hbmColor = CreateDIBSection(nullptr, &colorInfo, DIB_RGB_COLORS, &colorBits, nullptr, 0);
            HBITMAP hbmMask = CreateBitmap(width, height, 1, 1, fixedMaskBits.data());

            if (hbmColor && hbmMask)
            {
                memcpy(colorBits, pixels.data(), pixels.size() * sizeof(DWORD));

                ICONINFO fixedIconInfo = { FALSE, iconInfo.xHotspot, iconInfo.yHotspot, hbmMask, hbmColor };
                HCURSOR hFixedCursor = CreateIconIndirect(&fixedIconInfo);

                if (hFixedCursor)
                {
                    DestroyCursor(hCursor);
                    result = hFixedCursor;
                }
            }

            if (hbmColor) DeleteObject(hbmColor);
            if (hbmMask) DeleteObject(hbmMask);
        }
    }

    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);

    return result;
}

static int __stdcall LoadCursorBitmapResource_Hook(const char* pName, HCURSOR** ppCursor)
{
    int result = LoadCursorBitmapResource(pName, ppCursor);

    if (result == 0 && *ppCursor && pName && _stricmp(PathFindExtensionA(pName), ".ani") != 0)
    {
        **ppCursor = FixCursorTransparency(**ppCursor);
    }

    return result;
}

static void ApplyFixHardwareCursor()
{
    if (!FixHardwareCursor) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::LoadCursorBitmapResource), &LoadCursorBitmapResource_Hook, (LPVOID*)&LoadCursorBitmapResource, g_State.CurrentFEARGame == FEAR);
}