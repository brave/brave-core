#!/usr/bin/env python3
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import json
import optparse # pylint: disable=deprecated-module
import os
import sys
import subprocess

from rust_deps_config import RUST_DEPS_PACKAGE_VERSION


def run_cargo(command, args):
    rustup_path = args.rustup_path

    # Set environment variables for rustup
    env = os.environ.copy()

    rustup_home = os.path.join(rustup_path, RUST_DEPS_PACKAGE_VERSION)
    env['RUSTUP_HOME'] = rustup_home
    env['CARGO_HOME'] = rustup_home

    rustup_bin = os.path.abspath(os.path.join(rustup_home, 'bin'))
    cargo_exe = os.path.join(rustup_bin, 'cargo')
    if sys.platform == "win32":
        cargo_exe += ".exe"

    env['PATH'] = rustup_bin + os.pathsep + env['PATH']

    if args.toolchain:
        toolchains_path = os.path.abspath(
            os.path.join(rustup_path, 'toolchains', args.toolchain, "bin"))
        env['PATH'] = toolchains_path + os.pathsep + env['PATH']
        llvm_ar = os.path.join(toolchains_path, 'llvm-ar')
        env['AR'] = llvm_ar

    if args.mac_deployment_target is not None:
        env['MACOSX_DEPLOYMENT_TARGET'] = args.mac_deployment_target

    if args.ios_deployment_target is not None:
        env['IPHONEOS_DEPLOYMENT_TARGET'] = args.ios_deployment_target

    if args.is_debug == "false":
        env['NDEBUG'] = "1"

    if args.rust_flags is not None:
        env['RUSTFLAGS'] = " ".join(args.rust_flags)

    env["RUST_BACKTRACE"] = "1"

    try:
        cargo_args = []
        cargo_args.append(cargo_exe)
        cargo_args.append(command)
        if args.profile == "release":
            cargo_args.append("--release")
        cargo_args.append("--manifest-path=" + args.manifest_path)
        cargo_args.append("--target-dir=" + args.build_path)
        cargo_args.append("--target=" + args.target)
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
    if os.path.exists(build_args_cache_file):
        with open(build_args_cache_file, "r", encoding="utf8") as f:
            previous_args = json.load(f)

    if (previous_args != args.__dict__ or
            os.path.getmtime(build_args_cache_file) <
            os.path.getmtime(__file__)):
        run_cargo('clean', args)

    try:
        run_cargo('build', args)
        with open(build_args_cache_file, "w", encoding="utf8") as f:
            json.dump(args.__dict__, f)

    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def parse_args():
    parser = optparse.OptionParser(description='Cargo')

    parser.add_option('--rustup_path')
    parser.add_option('--manifest_path')
    parser.add_option('--build_path')
    parser.add_option('--target')
    parser.add_option('--toolchain')
    parser.add_option('--is_debug')
    parser.add_option('--profile')
    parser.add_option('--mac_deployment_target')
    parser.add_option('--ios_deployment_target')
    parser.add_option("--rust_flag", action="append",
                                     dest="rust_flags",
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
