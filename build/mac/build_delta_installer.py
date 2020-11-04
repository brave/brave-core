#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import optparse
import subprocess
import sys


def Main(argv):
  parser = optparse.OptionParser('%prog [options]')
  parser.add_option('--binary-delta', dest='binary_delta_path', action='store',
      type='string', default=None, help='The path of BinaryDelta binary.')
  parser.add_option('--sign-update', dest='sign_update_path', action='store',
      type='string', default=None, help='The path of sign_update binary')
  parser.add_option('--sign-key', dest='sign_key', action='store',
      type='string', default=None, help='The private key to sign patch file')
  parser.add_option('--old-app', dest='old_app_path', action='store',
      type='string', default=None, help='The path of old app bundle.')
  parser.add_option('--new-app', dest='new_app_path', action='store',
      type='string', default=None, help='The path of new app bundle.')
  parser.add_option('--new-dmg', dest='new_dmg_path', action='store',
      type='string', default=None, help='The path of new dmg.')
  parser.add_option('--patch-output', dest='patch_output_path', action='store',
      type='string', default=None, help='The path of generated delta file.')
  parser.add_option('--patch-eddsa-output', dest='patch_eddsa_output_path', action='store',
      type='string', default=None, help='The path of eddsa output of patch.')
  parser.add_option('--dmg-eddsa-output', dest='dmg_eddsa_output_path', action='store',
      type='string', default=None, help='The path of eddsa output of dmg.')
  (options, args) = parser.parse_args(argv)

  if len(args) > 0:
    print >> sys.stderr, parser.get_usage()
    return 1

  # generate patch file
  command = [options.binary_delta_path, 'create', options.old_app_path, options.new_app_path, options.patch_output_path]
  try:
      subprocess.check_call(command)
  except subprocess.CalledProcessError as e:
      print(e.output)
      raise e

  # sign patch file
  file = open(options.patch_eddsa_output_path, 'w')
  command = [options.sign_update_path, '-s', options.sign_key, options.patch_output_path]
  try:
      subprocess.check_call(command, stdout=file)
  except subprocess.CalledProcessError as e:
      print(e.output)
      raise e
  file.close()

  # sign dmg
  file = open(options.dmg_eddsa_output_path, 'w')
  command = [options.sign_update_path, '-s', options.sign_key, options.new_dmg_path]
  try:
      subprocess.check_call(command, stdout=file)
  except subprocess.CalledProcessError as e:
      print(e.output)
      raise e
  file.close()

  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
