#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import PLATFORM, SOURCE_ROOT, enable_verbose_mode
from lib.util import execute_stdout, scoped_cwd

SOLANA_WEB3_DIR = os.path.join(
    SOURCE_ROOT, 'vendor', 'solana-web3-js')

NPM = 'npm'
if PLATFORM in ['win32', 'cygwin']:
    NPM += '.cmd'


def main():
    args = parse_args()

    with scoped_cwd(SOLANA_WEB3_DIR):
        if args.verbose:
            enable_verbose_mode()
        if args.install:
            execute_stdout([NPM, 'install', '--no-save'])
        if args.build:
            execute_stdout([NPM, 'run', 'build'])
            execute_stdout(['cp', os.path.join('lib', 'index.iife.min.js'),
                os.path.join(args.output_path, 'solana_web3.min.js')])


def parse_args():
    parser = argparse.ArgumentParser(description='solana-web3.js setup')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Prints the output of the subprocesses')
    parser.add_argument('-i', '--install',
                        action='store_true',
                        help='Install solana-web3 dependencies')
    parser.add_argument('--output_path')
    parser.add_argument('-b', '--build',
                        action='store_true',
                        help='Build solana-web3 and copy minified js file to output_path')

    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())

