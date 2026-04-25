# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Generates Obj-C++ source files from a mojom.Module."""

import argparse

import importlib
import os
import sys
import json

_current_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(
    1,
    os.path.join(_current_dir,
                 *([os.pardir] * 4 + ['mojo/public/tools/mojom'])))

# pylint: disable=import-error,wrong-import-position
from mojom.generate.module import Module

def parse_args():
    parser = argparse.ArgumentParser(
        description='Generate Obj-C files from mojo definitions')
    parser.add_argument('--mojom-module', nargs=1)
    parser.add_argument('--output-dir', nargs=1)
    parser.add_argument('--bytecode-path', nargs=1)
    parser.add_argument('--no-namespace-generation',
                        default=False,
                        action='store_true')
    parser.add_argument('--exclude', nargs=1, required=False)
    return parser.parse_args()


def load_cpp_typemap_info(module_dir):
    # Attempt to load base typemap info if available
    base_typemap_path = os.path.join(os.path.dirname(module_dir),
                                     'mojom__type_mappings')
    if os.path.isfile(base_typemap_path):
        with open(base_typemap_path, 'rb') as f:
            return json.load(f)['c++'] if not None else {}
    return {}


def main():
    args = parse_args()
    mojom_module = args.mojom_module[0]
    output_dir = args.output_dir[0]
    bytecode_path = args.bytecode_path[0]
    generate_namespace = not args.no_namespace_generation
    excluded = args.exclude[0] if args.exclude else ""

    generator_module = importlib.import_module('mojom_objc_generator')

    generator = generator_module.Generator(None)
    generator.bytecode_path = bytecode_path
    generator.excludedTypes = excluded.split(',')
    generator.generateNamespace = generate_namespace
    generator.typemap = load_cpp_typemap_info(mojom_module)
    with open(mojom_module, 'rb') as f:
        generator.module = Module.Load(f)
    generator.GenerateFiles(output_dir)

if __name__ == "__main__":
    sys.exit(main())
