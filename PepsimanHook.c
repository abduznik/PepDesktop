#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef BOOL (WINAPI *BITBLT_T)(HDC, int, int, int, int, HDC, int, int, DWORD);
BITBLT_T OriginalBitBlt = NULL;

static HDC g_hCompDC = NULL;
static HBITMAP g_hCompBmp = NULL;
static RECT g_pepsiRect = {0, 0, 0, 0};
static __thread BOOL g_inHook = FALSE;
static BOOL g_loggedRect = FALSE;

BOOL CALLBACK FindAfxWndEnum(HWND hwnd, LPARAM lParam) {
    if (!IsWindow(hwnd)) return TRUE;
    char cls[256];
    if (GetClassName(hwnd, cls, sizeof(cls))) {
        // Primary: Afx signature, Fallback: Pepsiman4
        if (strncmp(cls, "Afx:400000", 10) == 0 || strcmp(cls, "Pepsiman4") == 0) {
            RECT r;
            if (GetWindowRect(hwnd, &r)) {
                int w = r.right - r.left;
                int h = r.bottom - r.top;
                if (w >= 10 && w <= 300 && h >= 10 && h <= 300) {
                    if (r.left >= 0 && r.left < 3000 && r.top >= 0 && r.top < 3000) {
                        if (r.left != 8192 && r.top != 8192) {
                            g_pepsiRect = r;
                            if (!g_loggedRect) {
                                FILE* f = fopen("dll_loaded.txt", "a");
                                if (f) {
                                    fprintf(f, "FIRST RECT LOCK: L:%ld T:%ld R:%ld B:%ld (Class: %s)\n", r.left, r.top, r.right, r.bottom, cls);
                                    fclose(f);
                                }
                                g_loggedRect = TRUE;
                            }
                            return FALSE;
                        }
                    }
                }
            }
        }
    }
    return TRUE;
}

DWORD WINAPI TrackingThread(LPVOID lpParam) {
    while (TRUE) {
        EnumWindows(FindAfxWndEnum, 0);
        Sleep(100);
    }
    return 0;
}

void SetupCompDC(HDC hTargetDC) {
    if (g_hCompDC) return;
    g_hCompDC = CreateCompatibleDC(hTargetDC);
    if (!g_hCompDC) return;
    g_hCompBmp = CreateCompatibleBitmap(hTargetDC, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    if (g_hCompBmp) {
        SelectObject(g_hCompDC, g_hCompBmp);
    } else {
        DeleteDC(g_hCompDC);
        g_hCompDC = NULL;
    }
}

void PatchIAT(HMODULE hModule, const char* dllName, const char* funcName, PVOID hookFunc, PVOID originalFunc) {
    if (!hModule || !originalFunc) return;
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hModule + 
        ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    if (!importDesc) return;
    while (importDesc->Name) {
        if (lstrcmpiA((char*)((BYTE*)hModule + importDesc->Name), dllName) == 0) {
            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((BYTE*)hModule + importDesc->FirstThunk);
            while (thunk->u1.Function) {
                PROC* procAddr = (PROC*)&thunk->u1.Function;
                if ((PVOID)*procAddr == originalFunc) {
                    DWORD oldProtect;
                    if (VirtualProtect(procAddr, sizeof(PVOID), PAGE_READWRITE, &oldProtect)) {
                        *procAddr = (PROC)hookFunc;
                        VirtualProtect(procAddr, sizeof(PVOID), oldProtect, &oldProtect);
                    }
                }
                thunk++;
            }
        }
        importDesc++;
    }
}

BOOL WINAPI HookedBitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop) {
    if (g_inHook || !OriginalBitBlt) return OriginalBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);

    // Skip hook until we have a valid window position to prevent drawing at 0,0
    if (g_pepsiRect.left == 0 && g_pepsiRect.top == 0) {
        return OriginalBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
    }

    int realX = g_pepsiRect.left + nXDest;
    int realY = g_pepsiRect.top + nYDest;

    if (dwRop == 0x008800C6) { // SRCAND
        g_inHook = TRUE;
        SetupCompDC(hdcDest);
        if (g_hCompDC) {
            HDC hdcDisplay = CreateDC("DISPLAY", NULL, NULL, NULL);
            if (hdcDisplay) {
                OriginalBitBlt(g_hCompDC, 0, 0, nWidth, nHeight, hdcDisplay, realX, realY, SRCCOPY);
                DeleteDC(hdcDisplay);
            }
            OriginalBitBlt(g_hCompDC, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, SRCAND);
        }
        g_inHook = FALSE;
        return TRUE; // Suppress original mascot blit
    }

    if (dwRop == 0x00EE0086) { // SRCPAINT
        if (g_hCompDC != NULL) {
            g_inHook = TRUE;
            OriginalBitBlt(g_hCompDC, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, SRCPAINT);
            OriginalBitBlt(hdcDest, realX, realY, nWidth, nHeight, g_hCompDC, 0, 0, SRCCOPY);
            g_inHook = FALSE;
            return TRUE; // Suppress original mascot blit
        }
    }

    return OriginalBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
}

DWORD WINAPI InitializationThread(LPVOID lpParam) {
    Sleep(3000);
    OriginalBitBlt = (BITBLT_T)GetProcAddress(GetModuleHandle("gdi32.dll"), "BitBlt");
    PatchIAT(GetModuleHandle(NULL), "gdi32.dll", "BitBlt", (PVOID)HookedBitBlt, (PVOID)OriginalBitBlt);
    HANDLE hTrack = CreateThread(NULL, 0, TrackingThread, NULL, 0, NULL);
    if (hTrack) CloseHandle(hTrack);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        HMODULE hOrig = LoadLibraryA("winmm_orig.dll");
        FILE* f = fopen("dll_loaded.txt", "w");
        if (f) {
            fprintf(f, "DLL LOADED. winmm_orig handle: %p\n", hOrig);
            fclose(f);
        }
        HANDLE hInit = CreateThread(NULL, 0, InitializationThread, NULL, 0, NULL);
        if (hInit) CloseHandle(hInit);
    }
    return TRUE;
}
