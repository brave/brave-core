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

from lib.config import SOURCE_ROOT


EXCLUDE_PATHS = [
    os.path.join(SOURCE_ROOT, 'build'),
    os.path.join(SOURCE_ROOT, 'components', 'brave_sync', 'extension', 'brave-sync', 'node_modules'),
    os.path.join(SOURCE_ROOT, 'node_modules'),
    os.path.join(SOURCE_ROOT, 'vendor', 'brave-extension', 'node_modules'),
]

whitelisted_advisories = [
]


def main():
    args = parse_args()
    errors = 0

    if args.input_dir:
        return npm_audit(os.path.abspath(args.input_dir), args)

    for path in [os.path.dirname(os.path.dirname(SOURCE_ROOT)), SOURCE_ROOT]:
        errors += npm_audit(path, args)

    for dir_path, dirs, dummy in os.walk(SOURCE_ROOT):
        for dir_name in dirs:
            full_path = os.path.join(dir_path, dir_name)
            skip_dir = False
            for exclusion in EXCLUDE_PATHS:
                if full_path.startswith(exclusion):
                    skip_dir = True
                    break

            if not skip_dir:
                errors += npm_audit(full_path, args)

    return errors > 0


def npm_audit(path, args):
    if os.path.isfile(os.path.join(path, 'package.json')) and \
       os.path.isfile(os.path.join(path, 'package-lock.json')) and \
       os.path.isdir(os.path.join(path, 'node_modules')):
        print('Auditing %s' % path)
        return audit_deps(path, args)
    else:
        print('Skipping audit of "%s" (no package.json or node_modules '
              'directory found)' % path)
        return 0


def audit_deps(path, args):
    npm_cmd = 'npm'
    if sys.platform.startswith('win'):
        npm_cmd = 'npm.cmd'

    npm_args = [npm_cmd, 'audit']

    # Just run audit regularly if --audit_dev_deps is passed
    if args.audit_dev_deps:
        return subprocess.call(npm_args, cwd=path)

    npm_args.append('--json')
    audit_process = subprocess.Popen(npm_args, stdout=subprocess.PIPE, cwd=path)
    output, error_data = audit_process.communicate()

    try:
        # results from audit
        result = json.loads(output.decode('UTF-8'))
    except ValueError:
        # This can happen in the case of an NPM network error
        print('Audit failed to return valid json')
        return 1

    # remove the results which match the exceptions
    for i, val in enumerate(result['actions']):
        result['actions'][i]['resolves'] = \
            [d for d in result['actions'][i]['resolves'] if
                d['id'] not in whitelisted_advisories]

    resolutions, non_dev_exceptions = extract_resolutions(result)

    # Trigger a failure if there are non-dev exceptions
    if non_dev_exceptions:
        print('Result: Audit finished, vulnerabilities found')
        print(json.dumps(non_dev_exceptions, indent=4))
        return 1

    # Still pass if there are dev exceptions, but let the user know about them
    if resolutions:
        print('Result: Audit finished, there are dev package warnings')
        print(json.dumps(resolutions, indent=4))
    else:
        print('Result: Audit finished, no vulnerabilities found')
    return 0


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
    parser.add_argument('--audit_dev_deps',
                        action='store_true',
                        help='Audit dev dependencies')
    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())
