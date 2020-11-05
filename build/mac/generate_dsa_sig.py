#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import optparse
import subprocess
import sys


def Main(argv):
  parser = optparse.OptionParser('%prog [options]')
  parser.add_option('--sign-update', dest='sign_update_path', action='store',
      type='string', default=None, help='The path of sign_update binary')
  parser.add_option('--sign-key-file', dest='sign_key_file', action='store',
      type='string', default=None, help='The private key to sign patch file')
  parser.add_option('--new-dmg', dest='new_dmg_path', action='store',
      type='string', default=None, help='The path of new dmg.')
  parser.add_option('--dmg-dsa-output', dest='dmg_dsa_output_path', action='store',
      type='string', default=None, help='The path of dsa output of dmg.')
  (options, args) = parser.parse_args(argv)

  if len(args) > 0:
    print >> sys.stderr, parser.get_usage()
    return 1

  # sign eddsa dmg
  file = open(options.dmg_dsa_output_path, 'w')
  command = [options.sign_update_path, options.new_dmg_path, options.sign_key_file]
  try:
      subprocess.check_call(command, stdout=file)
  except subprocess.CalledProcessError as e:
      print(e.output)
      raise e
  file.close()

  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
