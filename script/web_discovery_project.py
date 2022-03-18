#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import PLATFORM, SOURCE_ROOT, enable_verbose_mode
from lib.util import execute_stdout, scoped_cwd

WEB_DISCOVERY_DIR = os.path.join(
    SOURCE_ROOT, 'vendor', 'web-discovery-project')

NPM = 'npm'
if PLATFORM in ['win32', 'cygwin']:
    NPM += '.cmd'


def main():
    args = parse_args()
    env = os.environ.copy()

    with scoped_cwd(WEB_DISCOVERY_DIR):
        if args.verbose:
            enable_verbose_mode()
        if args.install:
            execute_stdout([NPM, 'install', '--no-save', '--yes'], env=env)
        if args.build:
            env["OUTPUT_PATH"] = args.output_path
            execute_stdout([NPM, 'run', 'build-module'], env=env)


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
