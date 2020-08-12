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


def build(args):
    rustup_path = args.rustup_path
    cargo_path = args.cargo_path
    manifest_path = args.manifest_path
    build_path = args.build_path
    target = args.target
    is_debug = args.is_debug
    clang_prefix = os.path.join(args.clang_prefix, "bin")
    clang_c_compiler = os.path.join(clang_prefix, "clang")
    sysroot = args.sysroot
    rustflags = []

    # Set environment variables for rustup
    env = os.environ.copy()

    rustup_home = os.path.join(rustup_path, RUST_DEPS_PACKAGE_VERSION)
    env['RUSTUP_HOME'] = rustup_home

    cargo_home = os.path.join(cargo_path, RUST_DEPS_PACKAGE_VERSION)
    env['CARGO_HOME'] = cargo_home

    rustup_bin = os.path.abspath(os.path.join(rustup_home, 'bin'))
    rustup_bin_exe = os.path.join(rustup_bin, 'cargo.exe')
    env['PATH'] = rustup_bin + os.pathsep + env['PATH']

    if args.toolchain:
        toolchains_path = os.path.abspath(
            os.path.join(rustup_path, 'toolchains', args.toolchain, "bin"))
        env['PATH'] = toolchains_path + os.pathsep + env['PATH']

    if args.mac_deployment_target is not None:
        env['MACOSX_DEPLOYMENT_TARGET'] = args.mac_deployment_target

    if is_debug == "false":
        env['NDEBUG'] = "1"

    if args.target.endswith("linux-gnu"):
        rustflags += ["-Clink-arg=-Wl,--build-id"]
    if not args.target.endswith("darwin"):
        rustflags += ["-Clink-arg=-Wl,--threads"]

    env["CARGO_TARGET_LINKER"] = clang_c_compiler
    env["CARGO_TARGET_X86_64_APPLE_DARWIN_LINKER"] = clang_c_compiler
    env["CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER"] = clang_c_compiler
    env["CARGO_TARGET_%s_LINKER" % target.replace("-", "_").upper()] = clang_c_compiler
    rustflags += ["-Clink-arg=--target=" + target]

    env["CARGO_TARGET_%s_RUSTFLAGS" % target.replace("-", "_").upper()] = (
        ' '.join(rustflags)
    )

    env["CC"] = clang_c_compiler
    cflags = []
    if sysroot:
        cflags += ["--sysroot=" + sysroot]
    env["CFLAGS"] = " ".join(cflags)

    env["CARGO_BUILD_DEP_INFO_BASEDIR"] = build_path
    env["RUST_BACKTRACE"] = "1"
    env["CC"] = os.path.join(clang_prefix, "clang")
    env["CXX"] = os.path.join(clang_prefix, "clang++")
    # TODO(bridiver) - these don't exist on mac and cause an error when they are added
    # env["AR"] = os.path.join(clang_prefix, "llvm-ar")
    # env["RANLIB"] = os.path.join(clang_prefix, "llvm-ranlib")

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
    parser.add_argument('--toolchain')
    parser.add_argument('--is_debug', required=True)
    parser.add_argument('--mac_deployment_target')
    parser.add_argument('--rust_flags')
    parser.add_argument('--clang_prefix')
    parser.add_argument('--sysroot')

    args = parser.parse_args()

    if (args.is_debug != "false" and args.is_debug != "true"):
        raise Exception("is_debug argument was not specified correctly")

    return args


def main():
    build(parse_args())

    return 0


if __name__ == '__main__':
    sys.exit(main())
