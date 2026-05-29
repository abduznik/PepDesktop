using System;
using System.Diagnostics;
using System.IO;

class PepsimanLauncher {
    static void Main() {
        try {
            string baseDir = AppDomain.CurrentDomain.BaseDirectory;
            string exePath = Path.Combine(baseDir, "dc_pepsi.exe");
            string injectorPath = Path.Combine(baseDir, "Injector.exe");

            // Simple launch. All magic happens in PepsimanHook.dll via CreateDC("DISPLAY").
            ProcessStartInfo startInfo = new ProcessStartInfo(injectorPath, string.Format("\"{0}\"", exePath));
            startInfo.WorkingDirectory = baseDir;
            Process.Start(startInfo);
            
            Console.WriteLine("Pepsiman Direct Display Engine Started.");
        } catch (Exception ex) {
            Console.WriteLine("Error: " + ex.Message);
        }
    }
}
