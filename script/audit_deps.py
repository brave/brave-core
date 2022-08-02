#!/usr/bin/env python3
# pylint: disable=line-too-long

"""This script runs `npm audit' and `cargo audit' on relevant paths in the
repo."""

# Copyright (c) 2020 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import json
import os
import subprocess
import sys

import requests

from deps_config import RUST_DEPS_PACKAGE_VERSION

def get_remote_audit_config(
        url = "https://raw.githubusercontent.com/brave/audit-config/main/config.json",
        retry = 3):
    """Fetch additional audit configuration"""
    s = requests.Session()
    s.mount(url, requests.adapters.HTTPAdapter(max_retries=retry))
    return s.get(url).json()

REMOTE_AUDIT_CONFIG = get_remote_audit_config()
IGNORED_CARGO_ADVISORIES = [e["advisory"] for e in REMOTE_AUDIT_CONFIG["ignore"]["cargo"]]
IGNORED_NPM_ADVISORIES = [e["advisory"] for e in REMOTE_AUDIT_CONFIG["ignore"]["npm"]]

# Use all (sub)paths except these for npm audit.
NPM_EXCLUDE_PATHS = [
    'build',
    os.path.join('node_modules')
]

# Use only these (sub)paths for cargo audit.
CARGO_INCLUDE_PATHS = [
    os.path.join('build', 'rust'),
]


def main():
    """Audit a specified path, or the whole project."""

    if len(IGNORED_NPM_ADVISORIES) > 0:
        print(
            f"Ignoring NPM advisories "
            f"{', '.join(map(str, IGNORED_NPM_ADVISORIES))}"
        )
    if len(IGNORED_CARGO_ADVISORIES) > 0:
        print(
            f"Ignoring Cargo advisories "
            f"{', '.join(map(str, IGNORED_CARGO_ADVISORIES))}"
        )

    args = parse_args()
    errors = 0

    if args.input_dir:
        return audit_path(os.path.abspath(args.input_dir), args)

    for path in [os.path.dirname(os.path.dirname(args.source_root)),
                 args.source_root]:
        errors += audit_path(path, args)

    for dir_path, dirs, _ in os.walk(args.source_root):
        for dir_name in dirs:
            full_path = os.path.join(dir_path, dir_name)
            errors += audit_path(full_path, args)

    return errors > 0


def audit_path(path, args):
    """Audit the specified path (relative, or absolute)."""

    full_path = os.path.join(os.path.abspath(path), "")
    if os.path.isfile(os.path.join(path, 'package.json')) and \
       os.path.isfile(os.path.join(path, 'package-lock.json')) and \
       not any(full_path.startswith(os.path.join(args.source_root, p, ""))
               for p in NPM_EXCLUDE_PATHS):
        print(f'Auditing (npm) {path}')
        return npm_audit_deps(path, args)

    if os.path.isfile(os.path.join(path, 'Cargo.toml')) and \
        os.path.isfile(os.path.join(path, 'Cargo.lock')) and \
        any(full_path.startswith(os.path.join(args.source_root, p, ""))
            for p in CARGO_INCLUDE_PATHS):
        print(f'Auditing (cargo) {path}')
        return cargo_audit_deps(path, args)

    return 0


def npm_audit_deps(path, args):
    """Run `npm audit' in the specified path."""

    npm_cmd = 'npm'
    if sys.platform.startswith('win'):
        npm_cmd = 'npm.cmd'

    npm_args = [npm_cmd, 'audit', '--json']
    if not args.audit_dev_deps:
        # Don't support npm audit --production until dev dependencies are
        # correctly identified in package.json
        print('npm audit --production not supported; auditing dev dependencies')
    audit_process = subprocess.Popen(
        npm_args, stdout=subprocess.PIPE, cwd=path)
    output, _ = audit_process.communicate()

    try:
        # results from audit
        result = json.loads(output.decode('UTF-8'))
        # npm7 uses a different format from earlier versions
        assert 'vulnerabilities' in result or 'advisories' in result
    except (ValueError, AssertionError):
        # This can happen in the case of an NPM network error
        print('Audit failed to return valid json')
        return 1

    resolutions = extract_resolutions(result)

    if len(resolutions) > 0:
        print('Result: Audit failed due to vulnerabilities')
        print(json.dumps(resolutions, indent=2))
        return 1

    print('Result: Audit finished, no vulnerabilities found')
    return 0


def cargo_audit_deps(path, args):
    """Run `cargo audit' in the specified path."""

    rustup_path = args.rustup_path
    cargo_path = args.cargo_path

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

    cargo_args = []
    cargo_args.append("cargo" if sys.platform != "win32" else rustup_bin_exe)
    cargo_args.append("audit")
    cargo_args.append("--file")
    cargo_args.append(os.path.join(path, "Cargo.lock"))
    for advisory in IGNORED_CARGO_ADVISORIES:
        cargo_args.append("--ignore")
        cargo_args.append(advisory)

    return subprocess.call(cargo_args, env=env)


def extract_resolutions(result):
    """Extract resolutions from advisories present in the result."""

    resolutions = []
    # npm 7+
    if 'vulnerabilities' in result:
        advisories = result['vulnerabilities']
        if len(advisories) == 0:
            return resolutions
        for _, v in advisories.items():
            via = v['via']
            for item in via:
                if isinstance(item, dict) and \
                   item['url'] not in IGNORED_NPM_ADVISORIES:
                    resolutions.append(item['url'])
    # npm 6 and earlier
    if 'advisories' in result:
        advisories = result['advisories']
        if len(advisories) == 0:
            return resolutions
        for _, v in advisories.items():
            url = v['url']
            if url not in IGNORED_NPM_ADVISORIES:
                resolutions.append(url)
    return resolutions


def parse_args():
    """Parse command line arguments."""

    parser = argparse.ArgumentParser(description='Audit brave-core npm deps')
    parser.add_argument('input_dir', nargs='?', help='Directory to check')
    parser.add_argument('--source_root', required=True,
                        help='Full path of the src/brave directory')
    parser.add_argument('--rustup_path', required=True)
    parser.add_argument('--cargo_path', required=True)
    parser.add_argument('--toolchain')
    parser.add_argument('--audit_dev_deps',
                        action='store_true',
                        help='Audit dev dependencies')
    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())
