import os
import sys

def patch_binary(file_path):
    if not os.path.exists(file_path):
        print(f"Error: {file_path} not found.")
        return

    print(f"Patching {file_path}...")
    with open(file_path, "rb") as f:
        data = bytearray(f.read())

    # 1. Patch Null Pointer Access Violation at 0x1160
    # Original: 8B 01 8B 50 08 FF D2 (mov eax, [ecx]; mov edx, [eax+8]; call edx)
    # Patch: Precision 16-byte assembly check injection
    offset_crash = 0x1160
    patch_crash = bytes.fromhex("85 C9 74 09 8B 01 8B 50 08 FF D2 C3 90 90 90 90")
    data[offset_crash:offset_crash+len(patch_crash)] = patch_crash
    print(" - Patched: Access Violation at 0x1160")

    # 2. Patch Error Popup JZ to JMP at 0xEFD7
    # This silences the "sorry, some errors occured" dialog
    offset_popup = 0xEFD7
    data[offset_popup] = 0xEB # JMP instead of JZ
    print(" - Patched: Error popup silenced at 0xEFD7")

    # 3. Patch Configuration Path to Local Mode at 0x52B0C
    # Original: "dc_pepsi.tbd"
    # Patch: ".\dc_pep.tbd" (Forces local path resolution, bypassing C:\Windows)
    offset_path = 0x52B0C
    path_patch = b".\\dc_pep.tbd\x00"
    data[offset_path:offset_path+len(path_patch)] = path_patch
    print(" - Patched: Local configuration mode enabled (dc_pepsi.tbd -> .\\dc_pep.tbd)")

    with open(file_path, "wb") as f:
        f.write(data)
    print("Finished patching!")

if __name__ == "__main__":
    target = "dc_pepsi.exe"
    if len(sys.argv) > 1:
        target = sys.argv[1]
    patch_binary(target)
