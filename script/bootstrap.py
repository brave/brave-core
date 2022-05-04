#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import PLATFORM, SOURCE_ROOT, \
    enable_verbose_mode, is_verbose_mode
from lib.util import execute_stdout, scoped_cwd


VENDOR_DIR = os.path.join(SOURCE_ROOT, 'vendor')
PYTHON_26_URL = 'https://chromium.googlesource.com/chromium/deps/python_26'

NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
    NPM += '.cmd'


def main():
    os.chdir(SOURCE_ROOT)

    args = parse_args()
    if not args.yes and PLATFORM != 'win32':
        check_root()
    if args.verbose:
        enable_verbose_mode()
    if sys.platform == 'cygwin':
        update_win32_python()

    setup_python_libs()
    update_node_modules('.')


def parse_args():
    parser = argparse.ArgumentParser(description='Bootstrap this project')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Prints the output of the subprocesses')
    parser.add_argument('-y', '--yes', '--assume-yes',
                        action='store_true',
                        help='Run non-interactively by assuming "yes" to all '
                        'prompts.')

    return parser.parse_args()


def check_root():
    if os.geteuid() == 0:
        print "We suggest not running this as root, unless you're really sure."
        choice = raw_input("Do you want to continue? [y/N]: ")
        if choice not in ('y', 'Y'):
            sys.exit(0)


def setup_python_libs():
    for lib in ('requests', 'boto'):
        with scoped_cwd(os.path.join(VENDOR_DIR, lib)):
            execute_stdout([sys.executable, 'setup.py', 'build'])


def update_node_modules(dirname, env=None):
    if env is None:
        env = os.environ.copy()
    with scoped_cwd(dirname):
        args = [NPM, 'install', '--no-save', '--yes']
        if is_verbose_mode():
            args += ['--verbose']
        execute_stdout(args, env)


def update_win32_python():
    with scoped_cwd(VENDOR_DIR):
        if not os.path.exists('python_26'):
            execute_stdout(['git', 'clone', PYTHON_26_URL])


if __name__ == '__main__':
    sys.exit(main())
