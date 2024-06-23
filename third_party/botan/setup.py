
import os
import shutil
import subprocess
import sys

def run_cmake(build_dir, source_dir):
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    subprocess.check_call([
        "cmake",
        source_dir + "/CMakeLists.txt"
    ])
    subprocess.call("make")

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