#!/usr/bin/env python3

# Copyright (c) 2020 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
import optparse # pylint: disable=deprecated-module
import os
import sys
import subprocess

def run_cargo(command, args):
    # Set environment variables for rustup
    env = os.environ.copy()

    rustup_home = args.rustup_home
    env['RUSTUP_HOME'] = rustup_home
    env['CARGO_HOME'] = rustup_home
    # Enable experimental features in non-nightly builds
    env['RUSTC_BOOTSTRAP'] = '1'

    rustup_bin_dir = os.path.abspath(os.path.join(rustup_home, 'bin'))
    cargo_exe = args.exe

    env['PATH'] = rustup_bin_dir + os.pathsep + env['PATH']

    if args.toolchain:
        toolchains_path = os.path.abspath(
            os.path.join(rustup_home, 'toolchains', args.toolchain, "bin"))
        env['PATH'] = toolchains_path + os.pathsep + env['PATH']

    if args.clang_bin_path is not None and not sys.platform.startswith('win'):
        env['AR'] = os.path.join(args.clang_bin_path, 'llvm-ar')
        env['CC'] = os.path.join(args.clang_bin_path, 'clang')
        env['CXX'] = os.path.join(args.clang_bin_path, 'clang++')

    if args.mac_deployment_target is not None:
        env['MACOSX_DEPLOYMENT_TARGET'] = args.mac_deployment_target

    if args.ios_deployment_target is not None:
        env['IPHONEOS_DEPLOYMENT_TARGET'] = args.ios_deployment_target

    if args.is_debug == "false":
        env['NDEBUG'] = "1"

    rust_flags = args.rust_flags.copy()
    rust_flags.append("-Cpanic=" + args.panic)

    if rust_flags is not None:
        env['RUSTFLAGS'] = " ".join(rust_flags)

    try:
        cargo_args = []
        cargo_args.append(cargo_exe)
        cargo_args.append(command)
        if args.profile == "release":
            cargo_args.append("--release")
        cargo_args.append("--manifest-path=" + args.manifest_path)
        cargo_args.append("--target-dir=" + args.build_path)
        cargo_args.append("--target=" + args.target)
        if command == "rustc" and args.features is not None:
            cargo_args.append("--features=" + args.features)
        # use deployment target as a proxy for mac/ios target_os
        if (args.mac_deployment_target is not None
                or args.ios_deployment_target is not None):
            cargo_args.append("-Z")
            cargo_args.append("build-std=panic_" + args.panic + ",std")
        if command == "rustc":
            cargo_args.append('--lib')
            cargo_args.append('--crate-type=staticlib')
            cargo_args.append('--')
            cargo_args += rust_flags
        subprocess.check_call(cargo_args, env=env)

    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def build(args):
    # Check the previous args against the current args because cargo doesn't
    # rebuild when env vars change and rerun-if-env-changed doesn't force the
    # dependencies to rebuild
    build_args_cache_file = os.path.join(args.build_path, ".cargo_args")
    previous_args = {}
    clean = False

    if os.path.exists(build_args_cache_file):
        with open(build_args_cache_file, "r", encoding="utf8") as f:
            previous_args = json.load(f)

        if previous_args != args.__dict__:
            clean = True

        for filename in args.other_inputs:
            if (os.path.getmtime(build_args_cache_file) <
                    os.path.getmtime(filename)):
                clean = True

    if clean:
        run_cargo('clean', args)

    try:
        run_cargo('rustc', args)
        with open(build_args_cache_file, "w", encoding="utf8") as f:
            json.dump(args.__dict__, f)

    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def parse_args():
    parser = optparse.OptionParser(description='Cargo')

    parser.add_option('--exe')
    parser.add_option('--rustup_home')
    parser.add_option('--manifest_path')
    parser.add_option('--build_path')
    parser.add_option('--target')
    parser.add_option('--toolchain')
    parser.add_option('--is_debug')
    parser.add_option('--profile')
    parser.add_option('--panic')
    parser.add_option('--clang_bin_path')
    parser.add_option('--rust_version')
    parser.add_option('--mac_deployment_target')
    parser.add_option('--ios_deployment_target')
    parser.add_option("--rust_flag", action="append",
                                     dest="rust_flags",
                                     default=[])
    parser.add_option('--features')
    parser.add_option('--inputs',
                      action="append",
                      dest="other_inputs",
                      default=[])

    options, _ = parser.parse_args()

    if options.is_debug not in ('false', 'true'):
        raise Exception("is_debug argument was not specified correctly")

    if options.profile not in ('release', 'dev'):
        raise Exception("profile argument was not specified correctly")

    return options


def main():
    build(parse_args())

    return 0


if __name__ == '__main__':
    sys.exit(main())
