/* PlayPepsiman.c — 32-bit launcher + injector */
#include <windows.h>
#include <stdio.h>

int main(void)
{
    char cur[MAX_PATH], exe[MAX_PATH], dll[MAX_PATH];
    GetModuleFileNameA(NULL, cur, MAX_PATH);
    char *p = strrchr(cur, '\\');
    if (p) *(p+1) = 0;
    snprintf(exe, MAX_PATH, "%sdc_pepsi.exe", cur);
    snprintf(dll, MAX_PATH, "%sPepsimanHook.dll", cur);
    
    STARTUPINFOA si = {0}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessA(exe, NULL, NULL, NULL, FALSE, 0, NULL, cur, &si, &pi))
        return 1;
    CloseHandle(pi.hThread);
    
    WaitForInputIdle(pi.hProcess, 5000);
    Sleep(500);
    
    SIZE_T len = strlen(dll) + 1;
    LPVOID mem = VirtualAllocEx(pi.hProcess, NULL, len, MEM_COMMIT, PAGE_READWRITE);
    if (!mem) { CloseHandle(pi.hProcess); return 1; }
    
    WriteProcessMemory(pi.hProcess, mem, dll, len, NULL);
    
    HANDLE ht = CreateRemoteThread(pi.hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
        mem, 0, NULL);
    if (ht) { WaitForSingleObject(ht, 5000); CloseHandle(ht); }
    
    VirtualFreeEx(pi.hProcess, mem, 0, MEM_RELEASE);
    CloseHandle(pi.hProcess);
    return 0;
}
