import argparse
import os
import sys
import shutil


def extract_data_z(data_z_path, output_dir):
    if not os.path.exists(data_z_path):
        print(f"Error: {data_z_path} not found.")
        return

    os.makedirs(output_dir, exist_ok=True)

    print(f"Extracting {data_z_path} to {output_dir}...")

    SETUP_INS = os.path.join(os.path.dirname(data_z_path), "SETUP.INS")

    if not os.path.exists(SETUP_INS):
        print(f"Warning: {SETUP_INS} not found. Extraction may be incomplete.")

    import struct

    try:
        with open(data_z_path, "rb") as f:
            magic = f.read(4)
            if magic != b"Data":
                print(f"Error: {data_z_path} does not appear to be a valid Data.z file (magic={magic!r})")
                return

            f.read(4)
            entry_count = struct.unpack("<I", f.read(4))[0]
            print(f"Entries: {entry_count}")

            for i in range(entry_count):
                name_len = struct.unpack("<I", f.read(4))[0]
                name = f.read(name_len).decode("ascii", errors="replace").rstrip("\x00")
                offset, size = struct.unpack("<II", f.read(8))

                pos = f.tell()
                f.seek(offset)
                data = f.read(size)
                f.seek(pos)

                out_path = os.path.join(output_dir, name)
                os.makedirs(os.path.dirname(out_path), exist_ok=True)
                with open(out_path, "wb") as out:
                    out.write(data)
                print(f"  [{i+1}/{entry_count}] {name} ({size} bytes)")

        print(f"Successfully extracted {entry_count} files to {output_dir}")

    except Exception as e:
        print(f"Error during extraction: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="Extract assets from the Pepsiman 1997 Data.z archive"
    )
    parser.add_argument(
        "data_z",
        help="Path to Data.z (e.g., from the CD installer or installed game)"
    )
    parser.add_argument(
        "output_dir",
        nargs="?",
        default="Pepsiman_Extracted",
        help="Output directory for extracted files (default: Pepsiman_Extracted)"
    )
    args = parser.parse_args()
    extract_data_z(args.data_z, args.output_dir)


if __name__ == "__main__":
    main()
