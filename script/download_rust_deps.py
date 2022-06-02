#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

"""Script to download rust_deps."""

import argparse
import os
import subprocess
import sys

import deps

try:
    from urllib2 import URLError
except ImportError:  # For Py3 compatibility
    from urllib.error import URLError  # pylint: disable=no-name-in-module,import-error

from deps_config import DEPS_PACKAGES_URL, RUST_DEPS_PACKAGE_VERSION
from lib.config import BRAVE_CORE_ROOT

RUSTUP_PATH = os.path.join(BRAVE_CORE_ROOT, 'build', 'rustup')
RUSTUP_HOME = os.path.join(RUSTUP_PATH, RUST_DEPS_PACKAGE_VERSION)


def get_url(platform):
    if platform in ("win32", "cygwin"):
        filename = "rust_deps_win_" + RUST_DEPS_PACKAGE_VERSION + ".zip"
    elif platform == 'darwin':
        filename = "rust_deps_mac_" + RUST_DEPS_PACKAGE_VERSION + ".gz"
    elif platform == 'ios':
        filename = "rust_deps_ios_" + RUST_DEPS_PACKAGE_VERSION + ".gz"
    elif platform.startswith('linux'):
        filename = "rust_deps_linux_" + RUST_DEPS_PACKAGE_VERSION + ".gz"
    else:
        print('Invalid platform for Rust deps: %s' % platform)
        print('Exiting.')
        sys.exit(1)

    return DEPS_PACKAGES_URL + "/rust-deps/" + filename


def should_download():
    if os.path.exists(RUSTUP_HOME):
        return False

    return True


def download_and_unpack_rust_deps(platform):
    if not should_download():
        return

    url = get_url('ios' if platform == 'ios' else sys.platform)

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


def make_standalone_toolchain_for_android():
    ANDROID_NDK_PATH = os.path.join(os.path.dirname(
        __file__), '..', '..', 'third_party', 'android_ndk')

    make_standalone_toolchain = os.path.join(
        ANDROID_NDK_PATH, 'build', 'tools', 'make_standalone_toolchain.py')

    config_path = os.path.join(RUSTUP_HOME, 'config')
    fp = open(config_path, "w+")

    for target_arch in ['arm', 'arm64', 'x86', 'x86_64']:
        toolchain_path = os.path.join(RUSTUP_PATH, 'toolchains', target_arch)

        api_level = get_android_api_level(target_arch)

        # Make standalone Android toolchain for target_arch
        toolchain_args = []
        toolchain_args.append(make_standalone_toolchain)
        toolchain_args.append("--force")
        toolchain_args.append("--install-dir=" + toolchain_path)
        toolchain_args.append("--api=" + api_level)
        toolchain_args.append("--arch=" + target_arch)

        try:
            subprocess.check_call(toolchain_args, env=None)
        except subprocess.CalledProcessError as e:
            print(e.output)
            raise e

        # Add target to rustup config
        fp.write("[target." + get_android_target(target_arch) + "]\r\n")
        fp.write("linker = \"" + get_android_linker(target_arch) + "\"\r\n")

    fp.close()


def parse_args():
    parser = argparse.ArgumentParser(description='Download rust deps')

    parser.add_argument('platform')

    args = parser.parse_args()
    return args


def cargo_install(tool):
    cargo_args = ["install", tool["name"]]
    if "path" in tool:
        cargo_args.append("--path")
        cargo_args.append(tool["path"])
        cargo_args.append("--target-dir")
        cargo_args.append(os.path.join(RUSTUP_HOME, ".build"))
    if "version" in tool:
        cargo_args.append("--version")
        cargo_args.append(tool["version"])
    if "features" in tool:
        cargo_args.append("--features")
        cargo_args.append(tool["features"])

    run_rust_tool('cargo', cargo_args)


def rustup_add_target(target):
    run_rust_tool('rustup', ['target', 'add', target])


def run_rust_tool(tool, args):
    env = os.environ.copy()
    env['RUSTUP_HOME'] = RUSTUP_HOME
    env['CARGO_HOME'] = RUSTUP_HOME

    rustup_bin = os.path.abspath(os.path.join(RUSTUP_HOME, 'bin'))
    tool_path = os.path.join(
        rustup_bin, tool + (".exe" if sys.platform == "win32" else ""))

    try:
        subprocess.check_call([tool_path] + args, env=env)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def main():
    args = parse_args()

    if args.platform == 'android':
        download_and_unpack_rust_deps(sys.platform)
        make_standalone_toolchain_for_android()
    else:
        download_and_unpack_rust_deps(args.platform)

    if args.platform == 'darwin':
        # It would be nice to conditionally only add the target we actually
        # need for building. However, DEPS (which we are called from) does not
        # seem to support making this distinction.
        rustup_add_target('x86_64-apple-darwin')
        rustup_add_target('aarch64-apple-darwin')

    cxx_path = os.path.abspath(
        os.path.join(BRAVE_CORE_ROOT, '..', 'third_party', 'rust', 'cxx', 'v1'))

    with open(os.path.join(cxx_path, "README.chromium")) as readme_file:
        _VERSION_PREFIX = "Version: "
        for line in readme_file:
            if not line.startswith(_VERSION_PREFIX):
                continue
            cxx_version = line[len(_VERSION_PREFIX):].strip()

    tools = [
        {
            "name": "cbindgen",
            "version": "0.14.2",
        },
        {
            "name": "cxxbridge-cmd",
            "version": cxx_version,
        },
        {
            "name": "cargo-audit",
            "features": "vendored-openssl",
        }
    ]
    for tool in tools:
        cargo_install(tool)

    return 0


if __name__ == '__main__':
    sys.exit(main())
