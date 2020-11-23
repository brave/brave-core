#!/usr/bin/env vpython
# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

#!/usr/bin/env vpython
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Copyright (C) 2008 Evan Martin <martine@danga.com>

import os
import sys

import git_cl

def main(args):
  """Runs clang-format and gn format on the current changelist."""
  parser = git_cl.OptionParser()
  options, args = parser.parse_args(args)

  # Change the current working directory before calling so that it
  # shows the correct base.
  settings = git_cl.settings
  previous_cwd = os.getcwd()
  os.chdir(settings.GetRoot())
  try:
      cmd = ['git', 'cl', 'format', '--full'] + args
      git_cl.RunCommand(cmd)
  except:
    e = sys.exc_info()[1]
    print('Could not run format: %s' % e.message)
    return 1
  finally:
    os.chdir(previous_cwd)
  print('Formatting done.')
  return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
