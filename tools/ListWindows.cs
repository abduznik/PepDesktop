using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

class ListWindows {
    delegate bool EnumDelegate(IntPtr hWnd, int lParam);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumDelegate lpEnumFunc, int lParam);
    [DllImport("user32.dll")] static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    static void Main() {
        Console.WriteLine("Starting 10-second Window Position Trace...");
        for (int i = 0; i < 6; i++) {
            Console.WriteLine(string.Format("\n--- Sample {0} at {1} ---", i, DateTime.Now.ToLongTimeString()));
            EnumWindows((hWnd, lParam) => {
                StringBuilder title = new StringBuilder(256);
                StringBuilder cls = new StringBuilder(256);
                GetWindowText(hWnd, title, 256);
                GetClassName(hWnd, cls, 256);

                if (title.ToString().Contains("Pepsiman") || cls.ToString().StartsWith("Afx")) {
                    RECT r; GetWindowRect(hWnd, out r);
                    Console.WriteLine(string.Format("HWND: 0x{0:X} | Class: {1} | Title: {2} | Rect: {3},{4}-{5},{6}", 
                        hWnd.ToInt32(), cls, title, r.Left, r.Top, r.Right, r.Bottom));
                }
                return true;
            }, 0);
            if (i < 5) Thread.Sleep(2000);
        }
        Console.WriteLine("\nTrace Complete.");
    }
}
