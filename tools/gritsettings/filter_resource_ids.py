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


def filter_ids(in_file, out_file):
    with open(in_file, 'r', encoding='utf-8') as f:
        output_lines = []
        skipping = False
        for line in f:
            # If we are skipping the current entry, then look for the closing
            # pattern.
            if skipping:
                if re.search(r'^\s*},\s*$', line):
                    skipping = False
                continue
            # Pass comments and empty lines through
            if re.search(r'^\s*#|^\s*$', line):
                output_lines.append(line)
                continue
            # Check if this is the SRCDIR entry and if so adjust the path
            # because the input to the "default_resource_ids" action will
            # now be the output of this script instead of the
            # src/tools/gritsettings/resource_ids.spec
            if re.search(r'^\s*\"SRCDIR\": \"\.\.\/\.\.\",\s*$', line):
                output_lines.append('  "SRCDIR": "../../../../..",\n')
                continue
            # Check if there's a key in this line
            match = re.search(r'^\s*\"([^\"]+)\"\s*:\s*{$', line)
            if match:
                key = match.group(1)
                # If the key matches our exclusion pattern then start skipping
                if re.search(patterns, key):
                    # This GRD is built on all Desktop platforms for some reason
                    # From upstream comment:
                    # TODO(b/207518736): Input overlay resources will be changed
                    # to proto soon, thus not going to move this resource pak to
                    #  under ash.
                    if key != 'chromeos/ash/experiences/arc/input_overlay/resources/input_overlay_resources.grd':
                        skipping = True
                        continue
                # Non-matching key, let it through
                output_lines.append(line)
                continue
            # Not a key-containing line, let it through
            output_lines.append(line)
    with open(out_file, 'w', encoding='utf-8') as f:
        f.write(''.join(output_lines))


def parse_args():
    parser = argparse.ArgumentParser(
        description='Filter out unused resource ids')
    parser.add_argument('--input', nargs=1, required=True)
    parser.add_argument('--output', nargs=1, required=True)
    return parser.parse_args()


def main():
    args = parse_args()
    filter_ids(args.input[0], args.output[0])


if __name__ == '__main__':
    sys.exit(main())
