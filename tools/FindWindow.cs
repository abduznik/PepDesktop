using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Collections.Generic;

class FindWindowTool {
    delegate bool EnumDelegate(IntPtr hWnd, int lParam);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumDelegate lpEnumFunc, int lParam);
    [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr hWndParent, EnumDelegate lpEnumFunc, int lParam);
    [DllImport("user32.dll")] static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hWnd);

    static void Main() {
        EnumWindows((hWnd, lParam) => {
            if (IsWindowVisible(hWnd)) {
                StringBuilder title = new StringBuilder(256);
                StringBuilder className = new StringBuilder(256);
                GetWindowText(hWnd, title, 256);
                GetClassName(hWnd, className, 256);
                
                if (title.ToString().Contains("Pepsiman")) {
                    Console.WriteLine("FOUND PEPSIMAN TOP-LEVEL: HWND: 0x{0:X} | Class: {1} | Title: {2}", hWnd.ToInt32(), className, title);
                    
                    // Enumerate children
                    EnumChildWindows(hWnd, (childHwnd, l) => {
                        StringBuilder childTitle = new StringBuilder(256);
                        StringBuilder childClass = new StringBuilder(256);
                        GetWindowText(childHwnd, childTitle, 256);
                        GetClassName(childHwnd, childClass, 256);
                        Console.WriteLine("  -> CHILD: HWND: 0x{0:X} | Class: {1} | Title: {2}", childHwnd.ToInt32(), childClass, childTitle);
                        return true;
                    }, 0);
                }
            }
            return true;
        }, 0);
    }
}
