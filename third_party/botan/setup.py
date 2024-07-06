import os
import subprocess
import sys
import platform

def run_cmake(build_dir, source_dir):
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    
    cmake_command = [
        "cmake",
        "-S", source_dir, 
        "-B", build_dir
    ]

    # Adjust commands based on the platform
    if platform.system() == "Windows":
        cmake_command.extend([
            "-G", "MinGW Makefiles"
        ])

    subprocess.check_call(cmake_command, shell=(platform.system() == "Windows"))

    # Run make or equivalent based on the platform
    if platform.system() == "Windows":
        subprocess.check_call(["cmake", "--build", build_dir], shell=True)
    else:
        subprocess.check_call(["make", "-C", build_dir])

def main():
    if len(sys.argv) != 3:
        print("Usage: build_botan.py <root_gen_dir> <out_dir>")
        sys.exit(1)

    root_gen_dir = sys.argv[1]
    out_dir = sys.argv[2]

    source_dir = root_gen_dir

    run_cmake(out_dir, source_dir)

if __name__ == "__main__":
    main()