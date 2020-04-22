#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import os
import sys
import subprocess
import shutil

from rust_deps_config import RUST_DEPS_PACKAGE_VERSION


def get_target_arch(target):
    return {
        'arm-linux-androideabi': 'arm',
        'aarch64-linux-android': 'arm64',
        'i686-linux-android': 'x86',
        'x86_64-linux-android': 'x86_64',
    }[target]


def build(args):
    rustup_path = args.rustup_path
    cargo_path = args.cargo_path
    manifest_path = args.manifest_path
    build_path = args.build_path
    target = args.target
    is_debug = args.is_debug

    # Set environment variables for rustup
    env = os.environ.copy()

    rustup_home = os.path.join(rustup_path, RUST_DEPS_PACKAGE_VERSION)
    env['RUSTUP_HOME'] = rustup_home

    cargo_home = os.path.join(cargo_path, RUST_DEPS_PACKAGE_VERSION)
    env['CARGO_HOME'] = cargo_home

    rustup_bin = os.path.abspath(os.path.join(rustup_home, 'bin'))
    rustup_bin_exe = os.path.join(rustup_bin, 'cargo.exe')
    env['PATH'] = rustup_bin + os.pathsep + env['PATH']

    if (target == 'arm-linux-androideabi' or
        target == 'aarch64-linux-android' or
        target == 'i686-linux-android' or
            target == 'x86_64-linux-android'):
        target_arch = get_target_arch(target)

        toolchains_path = os.path.abspath(
            os.path.join(rustup_path, 'toolchains', target_arch, "bin"))

        env['PATH'] = toolchains_path + os.pathsep + env['PATH']

    if args.mac_deployment_target is not None:
        env['MACOSX_DEPLOYMENT_TARGET'] = args.mac_deployment_target

    # Set environment variables for Challenge Bypass Ristretto FFI
    if is_debug == "false":
        env['NDEBUG'] = "1"

    if args.rust_flags is not None:
        env['RUSTFLAGS'] = args.rust_flags

    # Clean first because we want GN to decide when to rebuild and cargo doesn't
    # rebuild when env changes
    cargo_args = []
    cargo_args.append("cargo" if sys.platform != "win32" else rustup_bin_exe)
    cargo_args.append("clean")
    cargo_args.append("--manifest-path=" + manifest_path)
    cargo_args.append("--target-dir=" + build_path)
    cargo_args.append("--target=" + target)

    try:
        subprocess.check_call(cargo_args, env=env)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e

    # Build target
    cargo_args = []
    cargo_args.append("cargo" if sys.platform != "win32" else rustup_bin_exe)
    cargo_args.append("build")
    if is_debug == "false":
        cargo_args.append("--release")
    cargo_args.append("--manifest-path=" + manifest_path)
    cargo_args.append("--target-dir=" + build_path)
    cargo_args.append("--target=" + target)
    print("cargo args", cargo_args)
    print("rust flags", env['RUSTFLAGS'])
    try:
        subprocess.check_call(cargo_args, env=env)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def parse_args():
    parser = argparse.ArgumentParser(description='Cargo')

    parser.add_argument('--rustup_path', required=True)
    parser.add_argument('--cargo_path', required=True)
    parser.add_argument('--manifest_path', required=True)
    parser.add_argument('--build_path', required=True)
    parser.add_argument('--target', required=True)
    parser.add_argument('--is_debug', required=True)
    parser.add_argument('--mac_deployment_target')
    parser.add_argument('--rust_flags')

    args = parser.parse_args()

    if (args.is_debug != "false" and args.is_debug != "true"):
        raise Exception("is_debug argument was not specified correctly")

    return args


def main():
    build(parse_args())

    return 0


if __name__ == '__main__':
    sys.exit(main())
