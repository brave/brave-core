#!/usr/bin/env python

import argparse
import sys
import os
import subprocess
from lib.util import scoped_cwd, make_zip, tempdir, get_lzma_exec


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', help='Path to output archive.')
    parser.add_argument('--input', help='Path to input archive.')
    parser.add_argument('--target_dir',
                        help='If provided, the paths in the archive will be '
                        'relative to this directory', default='.')
    args = parser.parse_args()

    temp_dir = tempdir('brave_archive')
    cmd = [get_lzma_exec(), 'x', args.input, '-y', '-o' + temp_dir]
    subprocess.check_call(cmd, stdout=subprocess.PIPE, shell=False)
    with scoped_cwd(os.path.join(temp_dir, args.target_dir)):
        make_zip(args.output, [], ['.'])


if __name__ == '__main__':
    sys.exit(main())
