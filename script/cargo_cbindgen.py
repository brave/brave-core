#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import os
import sys
import subprocess
import shutil

from deps_config import RUST_DEPS_PACKAGE_VERSION


def run(args):
    # Set environment variables for rustup
    env = os.environ.copy()

    rustup_home = os.path.join(args.rustup_path, RUST_DEPS_PACKAGE_VERSION)
    env["RUSTUP_HOME"] = rustup_home

    cargo_home = os.path.join(args.cargo_path, RUST_DEPS_PACKAGE_VERSION)
    env["CARGO_HOME"] = cargo_home

    rustup_bin = os.path.abspath(os.path.join(rustup_home, "bin"))
    cbindgen_bin = os.path.join(
        rustup_bin, "cbindgen" if sys.platform != "win32" else "cbindgen.exe"
    )
    env["PATH"] = rustup_bin + os.pathsep + env["PATH"]

    # Install the tool
    cargo_args = []
    cargo_args.append(cbindgen_bin)
    cargo_args.append("--config")
    cargo_args.append(args.config)
    cargo_args.append("--output")
    cargo_args.append(args.output)
    cargo_args.append(args.path)

    try:
        subprocess.check_call(cargo_args, env=env)
    except subprocess.CalledProcessError as e:
        raise e


def parse_args():
    parser = argparse.ArgumentParser(description="Cargo cbindgen")

    parser.add_argument("--rustup_path", required=True)
    parser.add_argument("--cargo_path", required=True)
    parser.add_argument("--path", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--output", required=True)

    args = parser.parse_args()

    return args


def main():
    run(parse_args())

    return 0


if __name__ == "__main__":
    sys.exit(main())
