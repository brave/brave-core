#!/usr/bin/env python
# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Script to download rust_deps."""

import argparse
import ast
import os
import shutil
import subprocess
import sys

import deps
from deps_config import DEPS_PACKAGES_URL

try:
    from urllib2 import URLError
except ImportError:  # For Py3 compatibility
    from urllib.error import URLError  # pylint: disable=no-name-in-module,import-error

from lib.config import BRAVE_CORE_ROOT, CHROMIUM_ROOT

RUSTUP_PATH = os.path.join(BRAVE_CORE_ROOT, 'build', 'rustup')


def get_url(platform, rust_version):
    if platform in ("win32", "cygwin"):
        filename = "rust_deps_win_" + rust_version + ".zip"
    elif platform == 'darwin':
        filename = "rust_deps_mac_" + rust_version + ".gz"
    elif platform == 'ios':
        filename = "rust_deps_ios_" + rust_version + ".gz"
    elif platform.startswith('linux'):
        filename = "rust_deps_linux_" + rust_version + ".gz"
    else:
        print('Invalid platform for Rust deps: %s' % platform)
        print('Exiting.')
        sys.exit(1)

    return DEPS_PACKAGES_URL + "/rust-deps/" + filename


def should_download(rustup_home):
    if os.path.exists(os.path.join(rustup_home, 'bin')):
        return False

    return True


def download_and_unpack_rust_deps(platform, rust_version, rustup_home):
    if not should_download(rustup_home):
        return

    url = get_url('ios' if platform == 'ios' else sys.platform, rust_version)

    try:
        deps.DownloadAndUnpack(url, RUSTUP_PATH)
    except URLError:
        print('Failed to download Rust deps: %s' % url)
        print('Exiting.')
        sys.exit(1)


def get_android_api_level(target_arch):
    return {
        'arm': '19',
        'arm64': '24',
        'x86': '19',
        'x86_64': '24',
    }[target_arch]


def get_android_target(target_arch):
    return {
        'arm': 'arm-linux-androideabi',
        'arm64': 'aarch64-linux-android',
        'x86': 'i686-linux-android',
        'x86_64': 'x86_64-linux-android',
    }[target_arch]


def get_android_linker(target_arch):
    return get_android_target(target_arch) + "-clang"


def parse_args():
    parser = argparse.ArgumentParser(description='Download rust deps')

    parser.add_argument('rust_version')
    parser.add_argument('platform')

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

    rustup_bin = os.path.abspath(os.path.join(rustup_home, 'bin'))
    tool_path = os.path.join(
        rustup_bin, tool + (".exe" if sys.platform == "win32" else ""))

    try:
        subprocess.check_call([tool_path] + tool_args, env=env)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def main():
    args = parse_args()

    rust_version = ast.literal_eval(args.rust_version)
    rustup_home = os.path.join(RUSTUP_PATH, rust_version)

    if args.platform == 'android':
        download_and_unpack_rust_deps(sys.platform, rust_version, rustup_home)
    else:
        download_and_unpack_rust_deps(args.platform, rust_version, rustup_home)

    if sys.platform == 'darwin':
        rustup_add_target('x86_64-apple-darwin', rustup_home)
        rustup_add_target('aarch64-apple-darwin', rustup_home)
        # a separate directory is not created for aarch64-apple-darwin
        toolchain = rust_version + '-x86_64-apple-darwin'
        run_rust_tool(
            'rustup',
            ['component', 'add', 'rust-src', '--toolchain', toolchain],
            rustup_home)
        # patch gcc.rs.patch
        # https://github.com/rust-lang/rust/issues/102754#issuecomment-1421438735
        patched_file = os.path.join(BRAVE_CORE_ROOT, 'build', 'rust',
                                    'gcc.rs.patched')
        orig_file = os.path.join(rustup_home, 'toolchains',
                                 rust_version + '-x86_64-apple-darwin', 'lib',
                                 'rustlib', 'src', 'rust', 'library', 'std',
                                 'src', 'personality', 'gcc.rs')
        shutil.copyfile(patched_file, orig_file)

    cxx_path = os.path.abspath(
        os.path.join(CHROMIUM_ROOT, 'third_party', 'rust', 'cxx', 'v1'))

    with open(os.path.join(cxx_path, "README.chromium"), "r",
              encoding="utf8") as readme_file:
        _VERSION_PREFIX = "Version: "
        for line in readme_file:
            if not line.startswith(_VERSION_PREFIX):
                continue
            cxx_version = line[len(_VERSION_PREFIX):].strip()

    tools = [{
        "name": "cbindgen",
        "version": "0.14.2",
        "locked": True,
    }, {
        "name": "cxxbridge-cmd",
        "locked": True,
        "version": cxx_version,
    }, {
        "name": "cargo-audit",
        "version": "0.17.4",
        "locked": True,
        "features": "vendored-openssl",
    }]
    for tool in tools:
        cargo_install(tool, rustup_home)

    return 0


if __name__ == '__main__':
    sys.exit(main())
