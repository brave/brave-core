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
import re

import git_cl
import git_common

def HasFormatErrors():
    print('Running git cl format and gn format')
    # For more options, see vendor/depot_tools/git_cl.py
    cmd = ['cl', 'format', '--diff']
    diff = git_cl.RunGit(cmd)
    print(diff)
    return bool(diff)

def main(args):
  """Runs cpplint on the current changelist."""
  """Adapted from git_cl.py CMDlint """
  parser = git_cl.OptionParser()
  parser.add_option('--filter', action='append', metavar='-x,+y',
                    help='Comma-separated list of cpplint\'s category-filters')
  parser.add_option('--project_root')
  parser.add_option('--base_branch')
  options, args = parser.parse_args(args)

  # Access to a protected member _XX of a client class
  # pylint: disable=protected-access
  try:
    import cpplint
    import cpplint_chromium
  except ImportError:
    print('Your depot_tools is missing cpplint.py and/or cpplint_chromium.py.')
    return 1

  # Change the current working directory before calling lint so that it
  # shows the correct base.
  settings = git_cl.settings
  previous_cwd = os.getcwd()
  os.chdir(settings.GetRoot())

  # Check for clang/gn format errors.
  cl = git_cl.Changelist()
  try:
    if HasFormatErrors():
      upstream_branch = cl.GetUpstreamBranch()
      upstream_commit = git_cl.RunGit(['merge-base', 'HEAD', upstream_branch])
      print('Format check failed against commit %s. Run npm format to fix.' % upstream_commit)
      return 1
  except:
    e = sys.exc_info()[1]
    print('Error running format check: %s' % e.info)
    return 1
  print('Format check succeeded.')

  try:
    files = cl.GetAffectedFiles(git_common.get_or_create_merge_base(cl.GetBranch(), options.base_branch))
    if not files:
      print('Cannot lint an empty CL')
      return 0

    # Process cpplints arguments if any.
    command = args + files
    if options.filter:
      command = ['--filter=' + ','.join(options.filter)] + command
    if options.project_root:
      command = ['--project_root=' + options.project_root] + command
    filenames = cpplint.ParseArguments(command)

    white_regex = re.compile(settings.GetLintRegex())
    black_regex = re.compile(settings.GetLintIgnoreRegex())
    extra_check_functions = [cpplint_chromium.CheckPointerDeclarationWhitespace]
    for filename in filenames:
      if white_regex.match(filename):
        if black_regex.match(filename):
          print('Ignoring file %s' % filename)
        else:
          cpplint.ProcessFile(filename, cpplint._cpplint_state.verbose_level,
                              extra_check_functions)
      else:
        print('Skipping file %s' % filename)
  finally:
    os.chdir(previous_cwd)
  print('Total errors found: %d\n' % cpplint._cpplint_state.error_count)
  if cpplint._cpplint_state.error_count != 0:
    return 1
  return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
