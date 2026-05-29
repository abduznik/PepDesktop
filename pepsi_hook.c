#include <windows.h>
#include "minhook/include/MinHook.h"

// Pointers to the original functions
typedef HDC (WINAPI *GETDC_T)(HWND);
typedef int (WINAPI *RELEASEDC_T)(HWND, HDC);

GETDC_T fpGetDC = NULL;
RELEASEDC_T fpReleaseDC = NULL;

// Global tracking for our custom background DC to prevent memory leaks
HDC g_hdcCustom = NULL;
HBITMAP g_hbmpCustom = NULL;
HBITMAP g_hbmpOld = NULL;

HDC WINAPI Hooked_GetDC(HWND hWnd) {
    if (hWnd == NULL) {
        // Instead of capturing the broken DWM desktop, we return a solid green DC!
        // This allows our C launcher to use SetLayeredWindowAttributes to make green transparent.
        
        if (g_hdcCustom == NULL) {
            HDC hdcScreen = fpGetDC(NULL);
            g_hdcCustom = CreateCompatibleDC(hdcScreen);
            g_hbmpCustom = CreateCompatibleBitmap(hdcScreen, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            g_hbmpOld = (HBITMAP)SelectObject(g_hdcCustom, g_hbmpCustom);
            fpReleaseDC(NULL, hdcScreen);
        }

        // Fill it with pure green (Color Key: 0x0000FF00)
        RECT rect = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 0));
        FillRect(g_hdcCustom, &rect, hBrush);
        DeleteObject(hBrush);

        return g_hdcCustom;
    }
    return fpGetDC(hWnd);
}

int WINAPI Hooked_ReleaseDC(HWND hWnd, HDC hDC) {
    if (hWnd == NULL && hDC == g_hdcCustom) {
        // Do not actually release it, we reuse it for performance.
        return 1;
    }
    return fpReleaseDC(hWnd, hDC);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            MH_Initialize();
            MH_CreateHook(&GetDC, &Hooked_GetDC, (LPVOID*)&fpGetDC);
            MH_CreateHook(&ReleaseDC, &Hooked_ReleaseDC, (LPVOID*)&fpReleaseDC);
            MH_EnableHook(MH_ALL_HOOKS);
            break;

        case DLL_PROCESS_DETACH:
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
            if (g_hdcCustom != NULL) {
                SelectObject(g_hdcCustom, g_hbmpOld);
                DeleteObject(g_hbmpCustom);
                DeleteDC(g_hdcCustom);
            }
            break;
    }
    return TRUE;
}
