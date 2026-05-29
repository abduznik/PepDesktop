#include <windows.h>
#include <stdio.h>
#include <string.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    char currentDir[MAX_PATH];
    GetModuleFileName(NULL, currentDir, MAX_PATH);
    
    // Remove the executable name to get the directory
    char *lastSlash = strrchr(currentDir, '\\');
    if (lastSlash != NULL) {
        *(lastSlash + 1) = '\0'; // Keep the trailing backslash
    }

    char exePath[MAX_PATH];
    snprintf(exePath, sizeof(exePath), "%sdc_pepsi.exe", currentDir);

    char tbdPath[MAX_PATH];
    snprintf(tbdPath, sizeof(tbdPath), "%sdc_pep.tbd", currentDir);

    char pepsimanDir[MAX_PATH];
    snprintf(pepsimanDir, sizeof(pepsimanDir), "%spepsiman\\", currentDir);

    // 1. Check if dc_pepsi.exe exists
    if (GetFileAttributes(exePath) == INVALID_FILE_ATTRIBUTES) {
        MessageBox(NULL, "Could not find dc_pepsi.exe in the current directory.\nMake sure this launcher is placed in the same folder as dc_pepsi.exe.", "Launcher Error", MB_ICONERROR);
        return 1;
    }

    // 2. Write the proper contents into dc_pep.tbd dynamically
    char tbdContent[1024];
    snprintf(tbdContent, sizeof(tbdContent), 
        "[Settings]\r\n"
        "Path=%s\r\n"
        "Body=pepsiman.bac\r\n"
        "Texture=pepsi,pepsi1\r\n"
        "BaseSpeed=1.00\r\n"
        "MotorFile=figure.mot\r\n"
        "YOffset=-60\r\n", pepsimanDir);

    FILE *f = fopen(tbdPath, "w");
    if (f) {
        fputs(tbdContent, f);
        fclose(f);
    }

    // 3. Set Windows Compatibility Flags automatically for this executable
    // This forces 16-bit color mode to fix "smearing" and "deforming" bugs
    HKEY hKey;
    const char *compatKeyPath = "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
    if (RegCreateKeyEx(HKEY_CURRENT_USER, compatKeyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        const char *flagValue = "~ 16BITCOLOR";
        RegSetValueEx(hKey, exePath, 0, REG_SZ, (const BYTE *)flagValue, strlen(flagValue) + 1);
        RegCloseKey(hKey);
    }

    // 4. Launch the game
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(exePath, NULL, NULL, NULL, FALSE, 0, NULL, currentDir, &si, &pi)) {
        MessageBox(NULL, "Failed to launch dc_pepsi.exe.", "Launcher Error", MB_ICONERROR);
        return 1;
    }

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}
