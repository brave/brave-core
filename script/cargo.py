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


def main():
    args = parse_args()

    rustup_home = args.rustup_home[0]
    cargo_home = args.cargo_home[0]
    manifest_path = args.manifest_path[0]
    build_path = args.build_path[0]
    target = args.target[0]
    is_debug = args.is_debug[0]

    build(rustup_home, cargo_home, manifest_path, build_path, target, is_debug)


def parse_args():
    parser = argparse.ArgumentParser(description='Cargo')

    parser.add_argument('--rustup_home', nargs=1)
    parser.add_argument('--cargo_home', nargs=1)
    parser.add_argument('--manifest_path', nargs=1)
    parser.add_argument('--build_path', nargs=1)
    parser.add_argument('--target', nargs=1)
    parser.add_argument('--is_debug', nargs=1)

    args = parser.parse_args()

    # Validate rustup_home args
    if (args.rustup_home is None or
        len(args.rustup_home) is not 1 or
            len(args.rustup_home[0]) is 0):
        raise Exception("rustup_home argument was not specified correctly")

    # Validate cargo_home args
    if (args.cargo_home is None or
        len(args.cargo_home) is not 1 or
            len(args.cargo_home[0]) is 0):
        raise Exception("cargo_home argument was not specified correctly")

    # Validate manifest_path args
    if (args.manifest_path is None or
        len(args.manifest_path) is not 1 or
            len(args.manifest_path[0]) is 0):
        raise Exception("manifest_path argument was not specified correctly")

    # Validate build_path args
    if (args.build_path is None or
        len(args.build_path) is not 1 or
            len(args.build_path[0]) is 0):
        raise Exception("build_path argument was not specified correctly")

    if "out" not in args.build_path[0]:
        raise Exception("build_path did not contain 'out'")

    # Validate target args
    if (args.target is None or
        len(args.target) is not 1 or
            len(args.target[0]) is 0):
        raise Exception("target argument was not specified correctly")

    # Validate is_debug args
    if (args.is_debug is None or
        len(args.is_debug) is not 1 or
            len(args.is_debug[0]) is 0):
        raise Exception("is_debug argument was not specified correctly")

    if (args.is_debug[0] != "false" and args.is_debug[0] != "true"):
        raise Exception("is_debug argument was not specified correctly")

    # args are valid
    return args


def build(rustup_home, cargo_home, manifest_path, build_path, target, is_debug):
    # Set environment variables for rustup
    env = os.environ.copy()

    rustup_home = os.path.join(rustup_home, RUST_DEPS_PACKAGE_VERSION)
    env['RUSTUP_HOME'] = rustup_home

    cargo_home = os.path.join(cargo_home, RUST_DEPS_PACKAGE_VERSION)
    env['CARGO_HOME'] = cargo_home

    rustup_bin = os.path.join(rustup_home, 'bin')
    env['PATH'] = rustup_bin + os.pathsep + env['PATH']

    # Set environment variables for Challenge Bypass Ristretto FFI
    env['NO_CXXEXCEPTIONS'] = "1"
    if is_debug == "false":
        env['NDEBUG'] = "1"

    # Build targets
    args = []
    args.append("cargo")
    args.append("build")
    if is_debug == "false":
        args.append("--release")
    args.append("--manifest-path=" + manifest_path)
    args.append("--target-dir=" + build_path)
    args.append("--target=" + target)

    try:
        subprocess.check_call(args, env=env)
    except subprocess.CalledProcessError as e:
        print e.output
        raise e


if __name__ == '__main__':
    sys.exit(main())
