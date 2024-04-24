# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import argparse
import sys

_CWD = os.getcwd()


def get_webui_override_additional_inputs(in_folder, in_files, absolute_path=False):
  result = dict()

  # Don't add chromium_src overrides for generated files or Brave files
  if not in_folder.startswith('..') or 'brave' in in_folder: return result

  for input_file in in_files:
    in_path = os.path.join(in_folder, input_file)

    last_dot_dot = in_folder.rindex('../') + 3 # Add for the length of '../'

    relative_override_path = os.path.join('brave', 'chromium_src', in_folder[last_dot_dot:], input_file)
    override_file = os.path.normpath(os.path.join(_CWD, in_folder[:last_dot_dot], relative_override_path))
    
    # Only override files which exist
    if not os.path.exists(override_file): continue

    result[input_file] = override_file if absolute_path else '//' + relative_override_path
  return result

def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('--in-files', required=True, nargs="*")
  parser.add_argument('--in-folder', required=True)

  args = parser.parse_args(argv)

  # Return additional deps for this target as a newline separated list
  for additional_dep in get_webui_override_additional_inputs(args.in_folder, args.in_files).values():
    print(additional_dep)

if __name__ == '__main__':
  main(sys.argv[1:])
