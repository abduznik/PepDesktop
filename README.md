# Pepsiman 1997 Desktop Mascot Resurrection

This repository contains the tools and patches required to run the original 1997 *Desktop Character Pepsiman* mascot on modern 64-bit Windows systems (Windows 10/11) in a fully portable manner.

## Components

### 1. The Patcher (`make_portable.py`)
A Python script that applies critical binary patches to `dc_pepsi.exe`:
- **Crash Fix:** Bypasses a null-pointer access violation at `0x1160`.
- **Popup Suppression:** Silences the "sorry, some errors occured" dialog at `0xEFD7`.
- **Portability Hack:** Redirects the configuration lookup from `C:\Windows\dc_pepsi.ini` to a local file `.\dc_pep.tbd` at `0x52B0C`.

### 2. The Launcher (`launcher.c` / `launcher.cs`)
A standalone C/C# tool that:
- Dynamically generates the animation database (`dc_pep.tbd`) with current absolute paths.
- Automatically applies the `~ 16BITCOLOR` Windows compatibility flag to the registry to fix rendering deformation.
- (Experimental) Injects `WS_EX_LAYERED` transparency to handle modern DWM background capture issues.

### 3. The Asset Extractor (`extract_pepsi.py`)
Extracts the original game assets from the legacy `Data.z` and `SETUP.INS` files found on the 1997 CD/Installer.

## How to Build the Setup

1. **Extract Assets:** Run `python extract_pepsi.py` on your original game files.
2. **Patch Binary:** Place `dc_pepsi.exe` in the root and run `python make_portable.py`.
3. **Compile Launcher:** 
   - Using GCC: `gcc launcher.c -o PlayPepsiman.exe -luser32 -lgdi32`
   - Using CSC: `csc launcher.cs`
4. **Deploy:** Copy `dc_pepsi.exe`, `PlayPepsiman.exe`, and the `pepsiman/` asset folder into a single directory. 

Launch via **`PlayPepsiman.exe`** for the full portable experience.

## Credits
- Resurrected by Antigravity (Google DeepMind)
- Original Application by HIcorp (1997)
- [MinHook](https://github.com/TsudaKageyu/minhook) library used for API interception.