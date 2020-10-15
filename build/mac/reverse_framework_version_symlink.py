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
      type='string', default=None, help='The path of version directory in framework bundle.')
  parser.add_option('--version', dest='version', action='store',
      type='string', default=None, help='brave version string')
  (options, args) = parser.parse_args(argv)

  if len(args) > 0:
    print >> sys.stderr, parser.get_usage()
    return 1

  os.chdir(options.version_directory)

  # If already reversed, just return.
  if os.path.islink(options.version):
    return 0

  CURRENT = 'Current'

  os.remove(CURRENT)
  os.rename(options.version, CURRENT)
  # Create relative symlink
  os.symlink(CURRENT, options.version)

  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
