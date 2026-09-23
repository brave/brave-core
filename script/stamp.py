#!/usr/bin/env python
#
# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""
Creates a stamp file to compare against inputs for targets that do not generate
outputs.

Usage:
    stamp.py --stamp /stamp/path
"""

import argparse
import sys


def main(args):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stamp',
                        required=True,
                        help='Path to stamp to mark when finished.')
    options = parser.parse_args(args)

    if options.stamp:
        open(options.stamp, 'w').close()
    else:
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
