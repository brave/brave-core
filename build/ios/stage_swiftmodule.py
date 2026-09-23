# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Stages Swift module artifacts for inclusion in a framework bundle.

The Swift compiler writes its module artifacts as `<module_name>.swiftmodule`
(and friends), but a framework consumed outside of the build expects them in
the frameworks `Modules/<module_name>.swiftmodule` folder, with the actual
swiftmodule files renamed after the target triple or the architecture.
This copies and renames them, and writes a depfile so ninja tracks the
(undeclared) module artifacts as inputs.
"""

import argparse
import os
import shutil
import sys
from brave_chromium_utils import sys_path

with sys_path('//build'):
    import action_helpers

# Extensions copied for each requested name. All of these are declared as
# outputs of the `swift` tool, so they are always present.
_EXTENSIONS = ['swiftmodule', 'swiftdoc', 'abi.json']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module-dir',
                        required=True,
                        help='directory containing the compiled module')
    parser.add_argument('--module-name',
                        required=True,
                        help='name of the compiled Swift module')
    parser.add_argument('--output-dir',
                        required=True,
                        help='directory to stage the module artifacts into')
    parser.add_argument('--name',
                        action='append',
                        default=[],
                        dest='names',
                        help='name to stage the artifacts as (repeatable)')
    parser.add_argument('--depfile', required=True, help='path to the depfile')
    parser.add_argument('--depfile-target',
                        required=True,
                        help='output the depfile is keyed on')

    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)

    inputs = []
    for extension in _EXTENSIONS:
        source = os.path.join(args.module_dir,
                              f'{args.module_name}.{extension}')
        if not os.path.exists(source):
            print(f'error: missing Swift module artifact: {source}',
                  file=sys.stderr)
            return 1

        inputs.append(source)
        for name in args.names:
            destination = os.path.join(args.output_dir, f'{name}.{extension}')
            # Copy unconditionally; ninja relies on the depfile to know when
            # this step needs to re-run.
            shutil.copyfile(source, destination)

    action_helpers.write_depfile(args.depfile, args.depfile_target, inputs)

    return 0


if __name__ == '__main__':
    sys.exit(main())
