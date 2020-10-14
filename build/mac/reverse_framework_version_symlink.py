#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import errno
import os
import os.path
import optparse
import subprocess
import shutil
import sys


def Main(argv):
  parser = optparse.OptionParser('%prog [options]')
  parser.add_option('--version-directory-path', dest='version_directory', action='store',
      type='string', default=None, help='The path of BinaryDelta binary.')
  parser.add_option('--version', dest='version', action='store',
      type='string', default=None, help='The path of sign_update binary')
  (options, args) = parser.parse_args(argv)

  if len(args) > 0:
    print >> sys.stderr, parser.get_usage()
    return 1

  CURRENT = 'Current'

  full_current_path = os.path.join(options.version_directory, CURRENT)
  full_versioned_path = os.path.join(options.version_directory, options.version)

  # If already reversed, just return.
  if os.path.islink(full_versioned_path):
    return 0

  os.remove(full_current_path)
  os.rename(full_versioned_path, full_current_path)
  os.symlink(full_current_path, full_versioned_path)

  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
