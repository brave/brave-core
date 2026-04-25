#!/usr/bin/env python

# Copyright (c) 2018 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import optparse
import sys
import os
import gn_helpers
from lib.util import scoped_cwd, make_zip, make_7z


sys.path.append(os.path.join(os.path.dirname(__file__),
                             os.pardir, os.pardir, "build"))


def main():
    parser = optparse.OptionParser()

    parser.add_option('--inputs',
                      help='GN format list of files to archive.')
    parser.add_option('--dir-inputs',
                      help='GN format list of files to archive.')
    parser.add_option('--output', help='Path to output archive.')
    parser.add_option('--base-dir',
                      help='If provided, the paths in the archive will be '
                      'relative to this directory', default='.')

    options, _ = parser.parse_args()

    inputs = []
    if (options.inputs):
        parser = gn_helpers.GNValueParser(options.inputs)
        inputs = parser.ParseList()

    dir_inputs = []
    if options.dir_inputs:
        parser = gn_helpers.GNValueParser(options.dir_inputs)
        dir_inputs = parser.ParseList()

    output = options.output
    base_dir = options.base_dir

    with scoped_cwd(base_dir):
        if output.endswith('.zip'):
            make_zip(output, inputs, dir_inputs)
        elif output.endswith('.7z'):
            make_7z(output, inputs, dir_inputs)
        else:
            assert False, "Invalid archive type: " + output


if __name__ == '__main__':
    sys.exit(main())
