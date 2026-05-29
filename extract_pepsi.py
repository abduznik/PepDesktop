import subprocess
import os
import sys

def main():
    # Paths
    data_z_path = os.path.join("Testing", "DC_PEPSI", "Data.z")
    output_dir = os.path.join("Testing", "Pepsiman_Extracted")
    idecomp_script = os.path.join("thirdparty", "idecomp", "idecomp.py")

    # Check if files exist
    if not os.path.exists(data_z_path):
        print(f"Error: {data_z_path} not found.")
        return

    if not os.path.exists(idecomp_script):
        print(f"Error: {idecomp_script} not found. Did you clone the submodule?")
        return

    # Ensure output directory exists
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    print(f"Extracting {data_z_path} to {output_dir}...")

    # Run idecomp
    # Command: python idecomp.py Data.z -C output_dir
    try:
        subprocess.run(
            [sys.executable, idecomp_script, data_z_path, "-C", output_dir],
            check=True
        )
        print(f"Successfully extracted to {output_dir}")
    except subprocess.CalledProcessError as e:
        print(f"Error during extraction: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    main()
