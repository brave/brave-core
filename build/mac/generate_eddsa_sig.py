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
  parser.add_option('--sign-key', dest='sign_key', action='store',
      type='string', default=None, help='The private key to sign patch file')
  parser.add_option('--target', dest='target', action='store',
      type='string', default=None, help='Target file path for signing.')
  parser.add_option('--output', dest='output', action='store',
      type='string', default=None, help='The path of eddsa output.')
  (options, args) = parser.parse_args(argv)

  if len(args) > 0:
    print >> sys.stderr, parser.get_usage()
    return 1

  # sign file with eddsa
  file = open(options.output, 'w')
  command = [options.sign_update_path, '-s', options.sign_key, options.target]
  try:
      subprocess.check_call(command, stdout=file)
  except subprocess.CalledProcessError as e:
      print(e.output)
      raise e
  file.close()

  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
