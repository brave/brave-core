# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Precompiles Obj-C generator template modules files"""

import argparse
import importlib
import os
import sys

_current_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(
    1,
    os.path.join(_current_dir,
                 *([os.pardir] * 4 + ['mojo/public/tools/mojom'])))

# pylint: disable=import-error,wrong-import-position
from mojom.generate import template_expander


def parse_args():
    parser = argparse.ArgumentParser(
        description='Precompiles Obj-C generator template modules files')
    parser.add_argument('--output-dir', nargs=1)
    return parser.parse_args()


def main():
    args = parse_args()
    output_dir = args.output_dir[0]

    generator_module = importlib.import_module('mojom_objc_generator')
    template_expander.PrecompileTemplates({"objc": generator_module},
                                          output_dir)


if __name__ == "__main__":
    sys.exit(main())
