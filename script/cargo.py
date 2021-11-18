#!/usr/bin/env python3
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import optparse
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

    if args.ios_deployment_target is not None:
        env['IPHONEOS_DEPLOYMENT_TARGET'] = args.ios_deployment_target

    if is_debug == "false":
        env['NDEBUG'] = "1"

    if args.rust_flags is not None:
        env['RUSTFLAGS'] = " ".join(args.rust_flags)

    env["RUST_BACKTRACE"] = "1"

    # Clean first because we want GN to decide when to rebuild and cargo doesn't
    # rebuild when env changes
    shutil.rmtree(build_path)

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
    parser = optparse.OptionParser(description='Cargo')

    parser.add_option('--rustup_path')
    parser.add_option('--cargo_path')
    parser.add_option('--manifest_path')
    parser.add_option('--build_path')
    parser.add_option('--target')
    parser.add_option('--toolchain')
    parser.add_option('--is_debug')
    parser.add_option('--mac_deployment_target')
    parser.add_option('--ios_deployment_target')
    parser.add_option("--rust_flag", action="append", dest="rust_flags", default=[])

    options, args = parser.parse_args()

    if (options.is_debug != "false" and options.is_debug != "true"):
        raise Exception("is_debug argument was not specified correctly")

    return options


def main():
    build(parse_args())

    return 0


if __name__ == '__main__':
    sys.exit(main())
