#!/usr/bin/env python3

# Copyright (c) 2020 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import json
import os
import subprocess
import sys

from rust_deps_config import RUST_DEPS_PACKAGE_VERSION


EXCLUDE_PATHS = [
    'build',
    os.path.join('components', 'brave_sync', 'extension', 'brave-sync', 'node_modules'),
    os.path.join('node_modules'),
    os.path.join('vendor', 'brave-extension', 'node_modules'),
]

# Ping security team before adding to ignored_npm_advisories
ignored_npm_advisories = [
    1556  # low-sev DoS vector that isn't fixed in some upstream packages
]


def main():
    args = parse_args()
    errors = 0

    if args.input_dir:
        return audit_path(os.path.abspath(args.input_dir), args)

    for path in [os.path.dirname(os.path.dirname(args.source_root)), args.source_root]:
        errors += audit_path(path, args)

    for dir_path, dirs, dummy in os.walk(args.source_root):
        for dir_name in dirs:
            full_path = os.path.join(dir_path, dir_name)
            skip_dir = False
            for exclusion in EXCLUDE_PATHS:
                if full_path.startswith(os.path.join(args.source_root, exclusion)):
                    skip_dir = True
                    break

            if not skip_dir:
                errors += audit_path(full_path, args)

    return errors > 0


def audit_path(path, args):
    if os.path.isfile(os.path.join(path, 'package.json')) and \
       os.path.isfile(os.path.join(path, 'package-lock.json')) and \
       os.path.isdir(os.path.join(path, 'node_modules')):
        print('Auditing (npm) %s' % path)
        return npm_audit_deps(path, args)
    elif os.path.isfile(os.path.join(path, 'Cargo.toml')) and os.path.isfile(os.path.join(path, 'Cargo.lock')):
        print('Auditing (cargo) %s' % path)
        return cargo_audit_deps(path, args)

    return 0


def npm_audit_deps(path, args):
    npm_cmd = 'npm'
    if sys.platform.startswith('win'):
        npm_cmd = 'npm.cmd'

    npm_args = [npm_cmd, 'audit', '--json']
    audit_process = subprocess.Popen(npm_args, stdout=subprocess.PIPE, cwd=path)
    output, error_data = audit_process.communicate()
    if not args.audit_dev_deps:
        print('WARNING: Ignoring npm devDependencies')

    try:
        # results from audit
        result = json.loads(output.decode('UTF-8'))
        assert 'actions' in result
    except (ValueError, AssertionError):
        # This can happen in the case of an NPM network error
        print('Audit failed to return valid json')
        return 1

    # remove the results which match the exceptions
    if len(ignored_npm_advisories):
        print('Ignoring NPM advisories ' + ','.join(map(str, ignored_npm_advisories)))
    for i, val in enumerate(result['actions']):
        result['actions'][i]['resolves'] = \
            [d for d in result['actions'][i]['resolves'] if
                d['id'] not in ignored_npm_advisories]

    resolutions, non_dev_exceptions = extract_resolutions(result)

    if resolutions:
        print('Result: Audit finished, vulnerabilities found')

    # Trigger a failure if there are non-dev exceptions
    if non_dev_exceptions and not args.audit_dev_deps:
        print('Result: Audit finished, dev vulnerabilities ignored')
        print(json.dumps(non_dev_exceptions, indent=4))
        return 1

    if resolutions:
        print(json.dumps(resolutions, indent=4))
        return 1 if args.audit_dev_deps else 0
    else:
        print('Result: Audit finished, no vulnerabilities found')
        return 0


def cargo_audit_deps(path, args):
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

    return subprocess.call(cargo_args, env=env)


def extract_resolutions(result):
    if 'actions' not in result:
        return [], []

    if len(result['actions']) == 0:
        return [], []

    if 'resolves' not in result['actions'][0]:
        return [], []

    resolutions = result['actions'][0]['resolves']
    return resolutions, [r for r in resolutions if not r['dev']]


def parse_args():
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
