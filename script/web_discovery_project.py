#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import PLATFORM, SOURCE_ROOT, enable_verbose_mode
from lib.util import execute_stdout, scoped_cwd

WEB_DISCOVERY_DIR = os.path.join(
    SOURCE_ROOT, 'vendor', 'web-discovery-project')

PNPM = 'pnpm'
NPM = 'npm'
if PLATFORM in ['win32', 'cygwin']:
    PNPM += '.cmd'
    NPM += '.cmd'


def main():
    args = parse_args()
    env = os.environ.copy()

    with scoped_cwd(WEB_DISCOVERY_DIR):
        if args.verbose:
            enable_verbose_mode()
        if args.install:
            # Check if pnpm-lock.yaml exists - vendor directories may use npm
            pnpm_lock = os.path.join(WEB_DISCOVERY_DIR, 'pnpm-lock.yaml')
            if os.path.exists(pnpm_lock):
                execute_stdout([PNPM, 'install', '--frozen-lockfile'], env=env)
            else:
                # Fall back to npm with package-lock.json
                execute_stdout([NPM, 'ci'], env=env)
        if args.build:
            env["OUTPUT_PATH"] = args.output_path
            # Use npm for build-module script as well
            pnpm_lock = os.path.join(WEB_DISCOVERY_DIR, 'pnpm-lock.yaml')
            pkg_manager = PNPM if os.path.exists(pnpm_lock) else NPM
            execute_stdout([pkg_manager, 'run', 'build-module'], env=env)


def parse_args():
    parser = argparse.ArgumentParser(description='Web Discovery Project setup')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Prints the output of the subprocesses')
    parser.add_argument('-i', '--install',
                        action='store_true',
                        help='Install Web Discovery Project dependencies')
    parser.add_argument('--output_path')
    parser.add_argument('-b', '--build',
                        action='store_true',
                        help='Build Web Discovery Project')

    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())
