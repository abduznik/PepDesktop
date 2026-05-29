/* PepsimanHook.c — Chroma-key + SRCAND dest rect clear + slow backup */
#define _WIN32_WINNT 0x0600
#include <windows.h>

static DWORD g_ourPid = 0;
static HWND g_targetHwnd = NULL;
typedef BOOL (WINAPI *BITBLT_T)(HDC,int,int,int,int,HDC,int,int,DWORD);
static BITBLT_T g_origBitBlt = NULL;
static DWORD g_lastClear = 0;

/* ---- IAT patching ---- */
static void patch_iat(const char *dll, const char *func, BITBLT_T hook, BITBLT_T *orig)
{
    HMODULE mod = GetModuleHandle(NULL);
    if (!mod) return;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)mod;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)mod + dos->e_lfanew);
    PIMAGE_IMPORT_DESCRIPTOR imp = (PIMAGE_IMPORT_DESCRIPTOR)
        ((BYTE*)mod + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    if (!imp) return;
    while (imp->Name) {
        if (lstrcmpiA((char*)((BYTE*)mod + imp->Name), dll) == 0) {
            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((BYTE*)mod + imp->FirstThunk);
            while (thunk->u1.Function) {
                FARPROC target = GetProcAddress(GetModuleHandleA(dll), func);
                if (target && (PVOID)(thunk->u1.Function) == (PVOID)target) {
                    *orig = (BITBLT_T)thunk->u1.Function;
                    DWORD old;
                    VirtualProtect(&thunk->u1.Function, sizeof(PVOID), PAGE_READWRITE, &old);
                    thunk->u1.Function = (ULONG_PTR)hook;
                    VirtualProtect(&thunk->u1.Function, sizeof(PVOID), old, &old);
                    return;
                }
                thunk++;
            }
        }
        imp++;
    }
}

/* ---- Hooked BitBlt: clear draw rect on SRCAND (all DCs), slow backup ---- */
static BOOL WINAPI Hooked_BitBlt(HDC hdc, int x, int y, int w, int h,
    HDC hsrc, int sx, int sy, DWORD rop)
{
    if (g_origBitBlt) {
        DWORD now = GetTickCount();
        
        /* SRCAND = start of frame draw. Clear this area to prevent ghosting. */
        if (rop == 0x008800C6) {
            g_lastClear = now;
            RECT r = { x, y, x + w, y + h };
            HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &r, brush);
            DeleteObject(brush);
        }
        /* Backup: if no SRCAND in 500ms, clear the entire visible window */
        else if (g_targetHwnd && now - g_lastClear > 500) {
            g_lastClear = now;
            RECT cr;
            GetClientRect(g_targetHwnd, &cr);
            HDC wdc = GetDC(g_targetHwnd);
            if (wdc) {
                HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
                FillRect(wdc, &cr, brush);
                DeleteObject(brush);
                ReleaseDC(g_targetHwnd, wdc);
            }
        }
        
        return g_origBitBlt(hdc, x, y, w, h, hsrc, sx, sy, rop);
    }
    return FALSE;
}

/* ---- Chroma-key setup ---- */
static BOOL CALLBACK find_win(HWND h, LPARAM)
{
    char cls[64];
    if (!GetClassNameA(h, cls, sizeof(cls))) return TRUE;
    int ok = 0;
    for (int i = 0; cls[i]; i++) {
        if (cls[i]=='A'&&cls[i+1]=='f'&&cls[i+2]=='x'&&cls[i+3]==':'
         && cls[i+4]=='4'&&cls[i+5]=='0'&&cls[i+6]=='0'&&cls[i+7]=='0'
         && cls[i+8]=='0') { ok=1; break; }
        if (cls[i]=='P'&&cls[i+1]=='e'&&cls[i+2]=='p'&&cls[i+3]=='s'
         && cls[i+4]=='i'&&cls[i+5]=='m'&&cls[i+6]=='a'&&cls[i+7]=='n') { ok=1; break; }
    }
    if (!ok) return TRUE;
    RECT r;
    if (!GetWindowRect(h, &r)) return TRUE;
    if (r.left==8192 && r.top==8192) return TRUE;
    if ((r.right-r.left) < 20) return TRUE;
    DWORD pid;
    GetWindowThreadProcessId(h, &pid);
    if (pid != g_ourPid) return TRUE;
    /* Apply chroma-key: black becomes transparent */
    SetWindowLongPtr(h, GWL_EXSTYLE,
        GetWindowLongPtr(h, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(h, RGB(0,0,0), 0, LWA_COLORKEY);
    g_targetHwnd = h;
    return FALSE;
}

static DWORD WINAPI thread_fn(LPVOID)
{
    LoadLibraryA("winmm_orig.dll");
    /* Install BitBlt hook */
    patch_iat("gdi32.dll", "BitBlt", Hooked_BitBlt, &g_origBitBlt);
    /* Apply chroma-key */
    for (int i = 0; i < 200; i++) {
        EnumWindows(find_win, 0);
        if (g_targetHwnd) break;
        Sleep(100);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE m, DWORD r, LPVOID)
{
    if (r == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(m);
        g_ourPid = GetCurrentProcessId();
        HANDLE h = CreateThread(NULL, 0, thread_fn, NULL, 0, NULL);
        if (h) CloseHandle(h);
    }
    return TRUE;
}
