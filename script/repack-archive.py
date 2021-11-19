#!/usr/bin/env python

import optparse
import sys
import os
import subprocess
from lib.util import scoped_cwd, make_zip, tempdir


def GetLZMAExec(build_dir):
  if sys.platform == 'win32':
    lzma_exec = os.path.join(build_dir, "..", "..", "third_party",
                             "lzma_sdk", "Executable", "7za.exe")
  else:
    lzma_exec = '7zr'  # Use system 7zr.
  return lzma_exec


def main():
    parser = optparse.OptionParser()
    parser.add_option('--output', help='Path to output archive.')
    parser.add_option('--input', help='Path to input archive.')
    parser.add_option('--build_dir',
                      help='Full path to build directory(i.e. <something>/out/Release)')
    parser.add_option('--target_dir',
                      help='If provided, the paths in the archive will be '
                      'relative to this directory', default='.')
    options, _ = parser.parse_args()

    temp_dir = tempdir('brave_archive')
    lzma_exec = GetLZMAExec(options.build_dir)
    cmd = [lzma_exec, 'x', options.input, '-y', '-o' + temp_dir]
    subprocess.check_call(cmd, stdout=subprocess.PIPE, shell=False)
    with scoped_cwd(os.path.join(temp_dir, options.target_dir)):
      make_zip(options.output, [], ['.'])


if __name__ == '__main__':
    sys.exit(main())
