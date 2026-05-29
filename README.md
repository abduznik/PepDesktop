# PepDesktop

PepDesktop revives the original 1997 *Desktop Character Pepsiman* mascot for modern Windows 10/11 (64-bit).

## Legal Notice

You **must own the original CD-ROM** to use this project. No game files or copyrighted assets are included in this repository. This project provides only patching tools and launcher code — you supply the original `dc_pepsi.exe` from your legally purchased copy.

## Prerequisites

- **MinGW-w64 (GCC)** — to compile the C launcher and hook DLL
- **Python 3** — to run the binary patcher and asset extractor
- The **original Pepsiman CD-ROM** (or its extracted installer files)

## Quick Start

1. **Obtain Pepsiman** — install from the original CD-ROM (1997), or extract `Data.z` using `extract_pepsi.py`.
2. **Copy `dc_pepsi.exe`** from the installation directory into this repository root.
3. **Patch the binary:**
   ```bash
   python make_portable.py
   ```
4. **Extract assets** (if needed):
   ```bash
   python extract_pepsi.py <path_to_Data.z> [output_dir]
   ```
5. **Build the launcher:**
   ```bash
   gcc launcher.c -o PlayPepsiman.exe -luser32 -lgdi32
   ```
6. **Build the hook DLL:**
   ```bash
   gcc PepsimanHook.c -shared -o PepsimanHook.dll -luser32 -lgdi32 -static-libgcc
   ```
7. **Run `PlayPepsiman.exe`** in the directory containing `dc_pepsi.exe`, `PepsimanHook.dll`, and the `pepsiman/` asset folder.

## Alternative: Pre-built Portable Package

The `Portable_Pepsiman/` directory contains a fully pre-patched, ready-to-run deployment:

| File | Purpose |
|------|---------|
| `dc_pepsi.exe` | Patched original binary |
| `PlayPepsiman.exe` | C launcher with DLL injection |
| `PepsimanHook.dll` | BitBlt hook DLL |
| `winmm.dll` | WinMM proxy for DLL injection |
| `winmm_orig.dll` | Original system `winmm.dll` |
| `dc_pep.tbd` | Animation database config |
| `pepsiman/` | Extracted game assets (`.bac`, `.mot`, `.TRA`, textures) |

**To use:** run `PlayPepsiman.exe` from inside `Portable_Pepsiman/`.

## How It Works

### The BitBlt Hook (`PepsimanHook.dll`)

The original Pepsiman mascot renders itself on Windows 9x using two BitBlt operations per frame — `SRCAND` (mask) followed by `SRCPAINT` (sprite) — to produce transparency via the classic bitmask-over-bitmap technique. On modern Windows with DWM compositing, `GetDC(NULL)` no longer captures a reliable desktop background, so the mascot renders over a black (or garbage) backdrop.

The hook intercepts these two BitBlt raster operations inside `dc_pepsi.exe`:

1. **`SRCAND` intercept** — creates an off-screen compositing DC, captures the real desktop behind the mascot window via `CreateDC("DISPLAY")`, then applies the mask.
2. **`SRCPAINT` intercept** — paints the sprite onto the composited buffer, then copies the final result back to the original destination using `SRCCOPY`.

A tracking thread continuously locates the Pepsiman window (by class `Afx:400000` or `Pepsiman4`) to follow it as it moves around the screen.

### The Binary Patcher (`make_portable.py`)

Three byte-level patches to `dc_pepsi.exe`:

- **Crash fix** at `0x1160` — adds a null-pointer guard before an indirect call.
- **Popup suppression** at `0xEFD7` — changes a conditional jump to an unconditional jump, silencing the "sorry, some errors occured" dialog.
- **Portability hack** at `0x52B0C` — redirects the config path from `C:\Windows\dc_pepsi.ini` to `.\dc_pep.tbd`.

### The Launcher (`launcher.c`)

Resolves its own directory, writes a `dc_pep.tbd` config file with absolute paths to the `pepsiman/` folder, sets the `~ 16BITCOLOR` Windows compatibility flag to fix rendering artifacts, and launches `dc_pepsi.exe`.

## Project Structure

| Path | Purpose |
|------|---------|
| `launcher.c` | C launcher — writes config, sets compat flags, launches the game |
| `PepsimanHook.c` | BitBlt hook DLL — composites mascot over modern desktop |
| `PlayPepsiman.c` | Launcher with CreateRemoteThread DLL injection |
| `pepsi_hook.c` | Alternative hook using MinHook (GetDC/ReleaseDC + green screen key) |
| `Injector.c` | Standalone DLL injector (CreateProcess suspended) |
| `ScreenCapServer.c` | Shared-memory desktop capture server |
| `make_portable.py` | Binary patcher for `dc_pepsi.exe` |
| `extract_pepsi.py` | Asset extractor from original CD installer |
| `launcher.cs` | C# launcher (older approach) |
| `winmm.def` | WinMM DLL proxy module definition |
| `winmm_names.txt` | WinMM export name list (documentation) |
| `winmm_exports.txt` | WinMM export ordinal map (documentation) |
| `tools/` | Debug utilities (`FindWindow.cs`, `ListWindows.cs`) |
| `minhook/` | Vendored MinHook library (submodule) |
| `Portable_Pepsiman/` | Pre-built portable deployment |

## Build Instructions

### C Launcher
```bash
gcc launcher.c -o PlayPepsiman.exe -luser32 -lgdi32
```

### Pepsiman Hook DLL
```bash
gcc PepsimanHook.c -shared -o PepsimanHook.dll -luser32 -lgdi32 -static-libgcc
```

### PlayPepsiman (CreateRemoteThread injector)
```bash
gcc PlayPepsiman.c -o PlayPepsiman.exe -luser32 -lkernel32
```

### Injector (standalone DLL injector)
```bash
gcc Injector.c -o Injector.exe -luser32 -lkernel32
```

### pepsi_hook (MinHook alternative)
```bash
gcc pepsi_hook.c -shared -o pepsi_hook.dll -Iminhook/include -Lminhook/build -lminhook -static-libgcc
```

### ScreenCapServer
```bash
gcc ScreenCapServer.c -o ScreenCapServer.exe -lgdi32 -luser32
```

## Credits

- Resurrected by **Antigravity** (Google DeepMind)
- Original application by **HIcorp** (1997)
- [MinHook](https://github.com/TsudaKageyu/minhook) library by TsudaKageyu
