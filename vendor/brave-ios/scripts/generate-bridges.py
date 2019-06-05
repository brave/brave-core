import argparse
import os
import subprocess
import shutil
import sys
import string

def main():
    args = parse_args()
    xcode_path = string.strip(subprocess.check_output(['xcode-select', '-print-path']))
    build_objcgen(xcode_path, args.objc_gen_root[0], args.output[0])
    generate_bridges(xcode_path, args.ledger[0], args.ads[0], args.output[0], args.include_dirs)

def build_objcgen(xcode_path, objc_gen_root, output_dir):
    subprocess.call("DEVELOPER_DIR={} xcodebuild -project {}/objc-gen.xcodeproj -scheme objc-gen CODE_SIGNING_REQUIRED=NO CONFIGURATION_BUILD_DIR={}/build/".format(xcode_path, objc_gen_root, output_dir), shell=True)

def generate_bridges(xcode_path, ledger, ads, output_dir, include_dirs):
    macos_includes = xcode_path + "/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include"
    cpp_includes = "/Library/Developer/CommandLineTools/usr/include/c++/v1"
    system_includes = " ".join(include_dirs)
    subprocess.call("{}/build/objc-gen {} {} {} -- {} {} {}".format(output_dir, ledger, ads, output_dir, macos_includes, cpp_includes, system_includes), shell=True)

def parse_args():
    parser = argparse.ArgumentParser(description='Generate Native Bridges for ledger & ads native libs')
    parser.add_argument('--objc-gen-root', nargs=1)
    parser.add_argument('--ledger', nargs=1)
    parser.add_argument('--ads', nargs=1)
    parser.add_argument('--output', nargs=1)
    parser.add_argument('--include-dirs', nargs='+')
    return parser.parse_args()

if __name__ == '__main__':
    sys.exit(main())
