#!/usr/bin/env python3

import argparse
import os
import sys
from lib.util import execute_stdout, scoped_cwd


NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
    NPM += '.cmd'


def main():
    args = parse_args()
    root_gen_dir = args.root_gen_dir[0]
    generate_tsconfig(root_gen_dir)


def parse_args():
    parser = argparse.ArgumentParser(description='Generate tsconfig')
    parser.add_argument('--root_gen_dir', nargs=1)
    args = parser.parse_args()
    # validate args
    if (args.root_gen_dir is None or
            len(args.root_gen_dir) != 1 or
            len(args.root_gen_dir[0]) == 0):
        raise Exception("root_gen_dir argument was not specified correctly")
    # args are valid
    return args


def generate_tsconfig(root_gen_dir, env=None):
    if env is None:
        env = os.environ.copy()

    args = [NPM, 'run', 'web-ui-gen-tsconfig']

    env["ROOT_GEN_DIR"] = root_gen_dir

    dirname = os.path.abspath(os.path.join(__file__, '..', '..'))
    with scoped_cwd(dirname):
        execute_stdout(args, env)


if __name__ == '__main__':
    sys.exit(main())
