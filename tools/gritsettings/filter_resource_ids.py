#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
'''This script takes the input resource ids file
   (//tools/gritsettings/resource_ids.spec) and filters out all entries that
   Brave doesn't use (e.g. chromeos, ash, glic). These entries are defined by
   the patterns below. This is done to reduce the resource ids range taken by
   upstream and free up space for Brave resources.
'''

import argparse
import re

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


def filter_ids(in_file, out_file, srcdir):
    '''Removes entries whose keys match the |patterns| defined above.

    Args:
      in_file: The path to the input file.
      out_file: The path to the output file.
      srcdir: The relative path from the output file to the repo root. This
      value is needed for updating the SRCDIR value in the output file (see
      below).

    Note:
      The output file and the output of the upstream's 'default_resource_ids"
      action in //tools/gritsettings/BUILD.gn will lose the comments and the
      pretty formatting from the input file.
    '''
    ids_dict = eval(read_file(in_file))
    # Update SRCDIR entry because the input to the "default_resource_ids"
    # action in //tools/gritsettings/BUILD.gn will now be the output of this
    # script ('//out/<BUILD_TYPE>/[TOOLCHAIN]/gen/tools/gritsettings/
    # filtered_default_resource_ids.spec') instead of '//tools/gritsettings/
    # resource_ids.spec'. SRCDIR is used by the scripts that consume the file
    # to get the path to the checkout's src directory.
    ids_dict['SRCDIR'] = srcdir

    for grd_filename in list(ids_dict.keys()):
        # If the key matches our removal pattern but not in exclusions, then
        # remove the entry
        if re.search(patterns,
                     grd_filename) and not grd_filename in exclusions:
            ids_dict.pop(grd_filename)
    with open(out_file, 'w', encoding='utf-8', newline='\n') as f:
        f.write(repr(ids_dict))


def parse_args():
    '''Parses arguments passed to the script.'''
    parser = argparse.ArgumentParser(
        description='Filter out unused resource ids')
    parser.add_argument('--input', nargs=1, required=True)
    parser.add_argument('--output', nargs=1, required=True)
    parser.add_argument('--srcdir', nargs=1, required=True)
    return parser.parse_args()


def main():
    args = parse_args()
    filter_ids(args.input[0], args.output[0], args.srcdir[0])


if __name__ == '__main__':
    main()
