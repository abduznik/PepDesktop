#include <windows.h>
#include <stdio.h>

typedef struct {
    int width;
    int height;
    unsigned char pixels[0]; 
} SharedMem;

int main() {
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    size_t pixelSize = w * h * 4;
    size_t totalSize = sizeof(int) * 2 + pixelSize;
    
    HANDLE hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)totalSize, "PepsimanDesktopCapture");
    if (!hMap) return 1;

    SharedMem* pMem = (SharedMem*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, totalSize);
    if (!pMem) return 1;

    pMem->width = w;
    pMem->height = h;

    HDC hScreen = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hScreen, w, h);
    SelectObject(hMemDC, hBmp);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    printf("Pepsiman ScreenCapServer Running (%dx%d)...\n", w, h);

    while (1) {
        // Capture desktop from the "clean" server-side context
        BitBlt(hMemDC, 0, 0, w, h, hScreen, 0, 0, SRCCOPY);
        GetDIBits(hMemDC, hBmp, 0, h, pMem->pixels, &bmi, DIB_RGB_COLORS);
        Sleep(16); // ~60 FPS
    }

    return 0;
}
