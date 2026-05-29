/* PlayPepsiman.c - Launcher with delayed injection */
#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

int main(void) {
    char exePath[MAX_PATH], dllPath[MAX_PATH], exeDir[MAX_PATH];
    
    GetModuleFileNameA(NULL, exeDir, MAX_PATH);
    char* lastSlash = strrchr(exeDir, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0';
    
    snprintf(exePath, MAX_PATH, "%sdc_pepsi.exe", exeDir);
    snprintf(dllPath, MAX_PATH, "%sPepsimanHook.dll", exeDir);
    
    /* Launch dc_pepsi.exe normally */
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    if (!CreateProcessA(exePath, NULL, NULL, NULL, FALSE,
                        0, NULL, exeDir, &si, &pi)) {
        printf("Failed to launch dc_pepsi.exe (err %lu)\n", GetLastError());
        return 1;
    }
    
    /* Wait for process to initialize */
    WaitForInputIdle(pi.hProcess, 3000);
    Sleep(500);
    
    /* Inject our DLL */
    SIZE_T pathLen = strlen(dllPath) + 1;
    LPVOID remoteMem = VirtualAllocEx(pi.hProcess, NULL, pathLen, 
                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        printf("VirtualAllocEx failed (err %lu)\n", GetLastError());
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 1;
    }
    
    WriteProcessMemory(pi.hProcess, remoteMem, dllPath, pathLen, NULL);
    
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    LPTHREAD_START_ROUTINE pLoadLib = (LPTHREAD_START_ROUTINE)
        GetProcAddress(hKernel32, "LoadLibraryA");
    
    HANDLE hThread = CreateRemoteThread(pi.hProcess, NULL, 0, 
                                         pLoadLib, remoteMem, 0, NULL);
    if (hThread) {
        WaitForSingleObject(hThread, 5000);
        CloseHandle(hThread);
    }
    
    VirtualFreeEx(pi.hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
