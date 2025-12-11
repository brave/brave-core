# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import re
import sys

filter_values = ['glic', 'ash', 'chromeos']
start_pattern = [r'^{}\/'.format(val) for val in filter_values]
other_pattern = [r'\/{}\/'.format(val) for val in filter_values]
patterns = '|'.join(start_pattern + other_pattern)

exclusions = [
    # This GRD is built on all Desktop platforms for some reason
    # From upstream comment:
    # TODO(b/207518736): Input overlay resources will be changed
    # to proto soon, thus not going to move this resource pak to
    #  under ash.
    'chromeos/ash/experiences/arc/input_overlay/resources/input_overlay_resources.grd'
]


def read_file(filename):
  '''Reads and returns the entire contents of the given file.

  Args:
    filename: The path to the file.
  '''
  with open(filename, 'r', encoding='utf-8') as f:
    return f.read()


def filter_ids(in_file, out_file, is_default_toolchain):
    '''Removes entries whose keys match the |patterns| defined above.

    Args:
      in_file: The path to the input file.
      out_file: The path to the output file.
      is_default_toolchain: If the target is being built for default
      toolchain. If the toolchain is not the default one, then there's another
      directory level added to the path (for example,
      'out/android_Debug_x86/clang_x64/gen/...', as opposed to
      'out/android_Debug_x86/gen/...' for the default toolchain).

    Note:
      The output file and the output of the upstream's 'default_resource_ids"
      action in //tools/gritsettings/BUILD.gn will lose the comments and the
      pretty formatting from the input file.
    '''
    ids_dict = eval(read_file(in_file))
    # Update SRCDIR entry because the input to the "default_resource_ids"
    # action in //tools/gritsettings/BUILD.gn will now be the output of this
    # script (//out/<BUILD_TYPE>/[TOOLCHAIN/]gen/tools/gritsettings) instead of
    # //tools/gritsettings/resource_ids.spec.
    # SRCDIR is used by the scripts that consume the file get the path to the
    # checkout's src directory.
    if is_default_toolchain:
        ids_dict['SRCDIR'] = '../../../../..'
    else:
        ids_dict['SRCDIR'] = '../../../../../..'
    for grd_filename in list(ids_dict.keys()):
        # If the key matches our removal pattern but not in exclusions, then
        # remove the entry
        if re.search(patterns, grd_filename) and not grd_filename in exclusions:
            ids_dict.pop(grd_filename)
    with open(out_file, 'w', encoding='utf-8') as f:
        f.write(repr(ids_dict))


def parse_args():
    '''Parses arguments passed to the script.'''
    parser = argparse.ArgumentParser(
        description='Filter out unused resource ids')
    parser.add_argument('--input', nargs=1, required=True)
    parser.add_argument('--output', nargs=1, required=True)
    parser.add_argument('--is_default_toolchain', action='store_true')
    return parser.parse_args()


def main():
    args = parse_args()
    filter_ids(args.input[0], args.output[0], args.is_default_toolchain)


if __name__ == '__main__':
    sys.exit(main())
