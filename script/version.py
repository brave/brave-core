#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import CHROMIUM_ROOT, get_brave_version


def main():
  args = parse_args()
  create_version(args.target_gen_dir[0], args.brave_version_patch[0])

def parse_args():
  parser = argparse.ArgumentParser(description='Transpile web-uis')
  parser.add_argument('-p', '--production',
                      action='store_true',
                      help='Uses production config')
  parser.add_argument('--target_gen_dir',
                      nargs=1)
  parser.add_argument('--brave_version_patch',
                      nargs=1)
  return parser.parse_args()


def create_version(target_gen_dir, brave_version_patch):
  version_path = os.path.abspath(os.path.join(CHROMIUM_ROOT, target_gen_dir, '..', 'version'))
  with open(version_path, 'w') as version_file:
    version_file.write(get_brave_version())

  # Strip off the v prefix.
  version = get_brave_version()[1:]
  version_four_values = version.split('.')
  version_four_values.append(brave_version_patch)

  version_h_path = os.path.join(CHROMIUM_ROOT, target_gen_dir, 'brave', 'version.h')
  with open(version_h_path, 'w') as version_h_file:
    version_h_file.write(''.join(['#define BRAVE_BROWSER_VERSION "', version, '"']))
    version_h_file.write('\n')
    version_h_file.write(''.join(['#define BRAVE_BROWSER_VERSION_FOUR_COMPONENTS "', '.'.join(version_four_values), '"']))

  another_version_h_path = os.path.join(CHROMIUM_ROOT, target_gen_dir, 'brave', 'VERSION')
  with open(another_version_h_path, 'w') as another_version_h_file:
    version_keys = ['MAJOR', 'MINOR', 'BUILD', 'PATCH']
    version_pairs = zip(version_keys, version_four_values)
    for version_pair in version_pairs:
      another_version_h_file.write('='.join(version_pair))
      another_version_h_file.write('\n')


if __name__ == '__main__':
  sys.exit(main())
