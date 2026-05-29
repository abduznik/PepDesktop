#include <windows.h>
#include <stdio.h>

void log_msg(const char* msg) {
    FILE* f = fopen("inject_log.txt", "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

int main(int argc, char* argv[]) {
    remove("inject_log.txt");
    if (argc < 2) {
        log_msg("Error: No target EXE specified.");
        return 1;
    }

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    char dllPath[MAX_PATH];
    GetFullPathName("PepsimanHook.dll", MAX_PATH, dllPath, NULL);

    char logBuf[1024];
    sprintf(logBuf, "Injecting DLL: %s into %s", dllPath, argv[1]);
    log_msg(logBuf);

    if (!CreateProcess(NULL, argv[1], NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        sprintf(logBuf, "CreateProcess failed: %d", GetLastError());
        log_msg(logBuf);
        return 1;
    }

    void* remoteMem = VirtualAllocEx(pi.hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    sprintf(logBuf, "VirtualAllocEx: %p", remoteMem);
    log_msg(logBuf);

    if (!remoteMem) {
        log_msg("VirtualAllocEx failed.");
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }

    if (!WriteProcessMemory(pi.hProcess, remoteMem, dllPath, strlen(dllPath) + 1, NULL)) {
        sprintf(logBuf, "WriteProcessMemory failed: %d", GetLastError());
        log_msg(logBuf);
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    log_msg("WriteProcessMemory: Success");

    HANDLE hThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"), remoteMem, 0, NULL);
    sprintf(logBuf, "CreateRemoteThread: %p", hThread);
    log_msg(logBuf);

    if (hThread) {
        WaitForSingleObject(hThread, 10000); // 10s timeout
        DWORD exitCode;
        GetExitCodeThread(hThread, &exitCode);
        sprintf(logBuf, "LoadLibrary Thread Exit Code: 0x%X", exitCode);
        log_msg(logBuf);
        CloseHandle(hThread);
    } else {
        sprintf(logBuf, "CreateRemoteThread failed: %d", GetLastError());
        log_msg(logBuf);
    }

    ResumeThread(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    log_msg("Process Resumed.");
    return 0;
}
