#!/usr/bin/env python

import argparse
import sys
import os
import subprocess
from lib.util import scoped_cwd, make_zip, tempdir


def GetLZMAExec():
    if sys.platform == 'win32':
        root_src_dir = os.path.abspath(os.path.join(
            os.path.dirname(__file__), *[os.pardir] * 2))
        lzma_exec = os.path.join(root_src_dir, "third_party",
                                 "lzma_sdk", "Executable", "7za.exe")
    else:
        lzma_exec = '7zr'  # Use system 7zr.
    return lzma_exec


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', help='Path to output archive.')
    parser.add_argument('--input', help='Path to input archive.')
    parser.add_argument('--target_dir',
                        help='If provided, the paths in the archive will be '
                        'relative to this directory', default='.')
    args = parser.parse_args()

    temp_dir = tempdir('brave_archive')
    lzma_exec = GetLZMAExec()
    cmd = [lzma_exec, 'x', args.input, '-y', '-o' + temp_dir]
    subprocess.check_call(cmd, stdout=subprocess.PIPE, shell=False)
    with scoped_cwd(os.path.join(temp_dir, args.target_dir)):
        make_zip(args.output, [], ['.'])


if __name__ == '__main__':
    sys.exit(main())
