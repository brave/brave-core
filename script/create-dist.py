#!/usr/bin/env python

import optparse
import sys
import glob
import os
import shutil
import gn_helpers
from lib.util import scoped_cwd, make_zip


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
        make_zip(output, inputs, dir_inputs)


if __name__ == '__main__':
    sys.exit(main())
