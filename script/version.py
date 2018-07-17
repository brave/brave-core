#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import CHROMIUM_ROOT, get_brave_version, get_chromium_version


def main():
  args = parse_args()
  create_version(args.target_gen_dir[0])

def parse_args():
  parser = argparse.ArgumentParser(description='Transpile web-uis')
  parser.add_argument('-p', '--production',
                      action='store_true',
                      help='Uses production config')
  parser.add_argument('--target_gen_dir',
                      nargs=1)
  return parser.parse_args()


def create_version(target_gen_dir):
  version_path = os.path.abspath(os.path.join(CHROMIUM_ROOT, target_gen_dir, '..', 'version'))
  with open(version_path, 'w') as version_file:
    version_file.write(get_brave_version())

  version_h_path = os.path.join(CHROMIUM_ROOT, target_gen_dir, 'brave', 'version.h')
  with open(version_h_path, 'w') as version_h_file:
    # Strip off the v prefix.
    version = get_brave_version()[1:]
    version_h_file.write(''.join(['#define BRAVE_BROWSER_VERSION "', version, '"']))
    version_h_file.write('\n')
    version_h_file.write(''.join(['#define BRAVE_CHROMIUM_VERSION "', get_chromium_version(), '"']))


if __name__ == '__main__':
  sys.exit(main())
