#!/usr/bin/env python3
# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Script to download rust_deps."""

import argparse
import os
import subprocess
import sys

import deps
from deps_config import DEPS_PACKAGES_URL

from lib.config import BRAVE_CORE_ROOT
from lib.util import get_host_os, get_host_arch, str2bool

RUSTUP_PATH = os.path.join(BRAVE_CORE_ROOT, 'build', 'rustup')


def host_rust_toolchain(rust_version):
    if get_host_os() == 'linux':
        return f'{rust_version}-x86_64-unknown-linux-gnu'
    if get_host_os() == 'mac':
        if get_host_arch() == 'arm64':
            return f'{rust_version}-aarch64-apple-darwin'
        return f'{rust_version}-x86_64-apple-darwin'
    if get_host_os() == 'win':
        return f'{rust_version}-x86_64-pc-windows-msvc'
    raise RuntimeError(f'Unsupported host_os: {get_host_os()}')


def get_url(rust_version):
    ext = 'zip' if get_host_os() == 'win' else 'gz'
    filename = f'rust_deps_{host_rust_toolchain(rust_version)}.{ext}'
    return DEPS_PACKAGES_URL + "/rust-deps/" + filename


def should_download(rustup_home):
    if os.path.exists(os.path.join(rustup_home, 'bin')):
        return False

    return True


def download_and_unpack_rust_deps(rust_version, rustup_home):
    if not should_download(rustup_home):
        return

    url = get_url(rust_version)

    deps.DownloadAndUnpack(url, RUSTUP_PATH)


def parse_args():
    parser = argparse.ArgumentParser(description='Download rust deps')

    parser.add_argument('--rust_version')
    parser.add_argument('--checkout_android', default=False, type=str2bool)
    parser.add_argument('--checkout_ios', default=False, type=str2bool)
    parser.add_argument('--checkout_linux',
                        default=get_host_os() == 'linux',
                        type=str2bool)
    parser.add_argument('--checkout_mac',
                        default=get_host_os() == 'mac',
                        type=str2bool)
    parser.add_argument('--checkout_win',
                        default=get_host_os() == 'win',
                        type=str2bool)

    args = parser.parse_args()
    return args


def cargo_install(tool, rustup_home):
    cargo_args = ["install", tool["name"]]
    if "path" in tool:
        cargo_args.append("--path")
        cargo_args.append(tool["path"])
        cargo_args.append("--target-dir")
        cargo_args.append(os.path.join(rustup_home, ".build"))
    if "version" in tool:
        cargo_args.append("--version")
        cargo_args.append(tool["version"])
    if "locked" in tool:
        cargo_args.append("--locked")
    if "features" in tool:
        cargo_args.append("--features")
        cargo_args.append(tool["features"])

    run_rust_tool('cargo', cargo_args, rustup_home)


def rustup_add_target(target, rustup_home):
    run_rust_tool('rustup', ['target', 'add', target], rustup_home)


def run_rust_tool(tool, tool_args, rustup_home):
    env = os.environ.copy()
    env['RUSTUP_HOME'] = rustup_home
    env['CARGO_HOME'] = rustup_home

    if get_host_os() == 'win':
        tool += '.exe'
    tool_path = os.path.abspath(os.path.join(rustup_home, 'bin', tool))

    try:
        subprocess.check_call([tool_path] + tool_args, env=env)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def main():
    args = parse_args()

    rust_version = args.rust_version
    rustup_home = os.path.join(RUSTUP_PATH, rust_version)

    download_and_unpack_rust_deps(rust_version, rustup_home)

    rustup_targets = set()

    if args.checkout_android:
        rustup_targets.update([
            'aarch64-linux-android',
            'arm-linux-androideabi',
            'armv7-linux-androideabi',
            'aarch64-linux-android',
            'i686-linux-android',
            'x86_64-linux-android',
        ])

    if args.checkout_ios:
        rustup_targets.update([
            'aarch64-apple-ios',
            'aarch64-apple-ios-sim',
            'x86_64-apple-ios',
        ])

    if args.checkout_linux:
        rustup_targets.update([
            'aarch64-unknown-linux-gnu',
            'x86_64-unknown-linux-gnu',
        ])

    if args.checkout_mac:
        rustup_targets.update([
            'aarch64-apple-darwin',
            'x86_64-apple-darwin',
        ])

    if args.checkout_win:
        rustup_targets.update([
            'aarch64-pc-windows-msvc',
            'i686-pc-windows-msvc',
            'x86_64-pc-windows-msvc',
        ])

    for rustup_target in rustup_targets:
        rustup_add_target(rustup_target, rustup_home)

    if args.checkout_mac:
        toolchain = host_rust_toolchain(rust_version)
        run_rust_tool(
            'rustup',
            ['component', 'add', 'rust-src', '--toolchain', toolchain],
            rustup_home)

    tools = [{
        "name": "cbindgen",
        "version": "0.14.2",
        "locked": True,
    }, {
        "name": "cargo-audit",
        "version": "0.17.5",
        "locked": True,
        "features": "vendored-openssl",
    }]
    for tool in tools:
        cargo_install(tool, rustup_home)

    return 0


if __name__ == '__main__':
    sys.exit(main())
