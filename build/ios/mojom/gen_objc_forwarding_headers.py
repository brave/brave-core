# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Generates forwarding headers for mojom dep modules.

When generating a C++ variant (e.g., --variant objc), the generated headers
include dep headers with the variant suffix (e.g., url.mojom-objc.h). If the
deps don't have that variant generated, we create simple forwarding headers
that include the default variant instead.

This works because dep types used across module boundaries are typically
cpp-typemapped to global C++ types (GURL, base::Time, etc.) that don't depend
on variant namespaces.
"""

import argparse
import os
import sys


_FORWARDING_SUFFIXES = [
    '.h',
    '-forward.h',
    '-import-headers.h',
    '-test-utils.h',
]


_HEADER_TEMPLATE = (
    '// Auto-generated forwarding header for objc variant.\n'
    '// See brave/build/ios/mojom/gen_objc_forwarding_headers.py\n'
    '#include "{default_path}"\n'
)


def main():
    parser = argparse.ArgumentParser(
        description='Generate forwarding headers for mojom dep modules')
    parser.add_argument(
        '--output-dir',
        required=True,
        help='Root output directory (typically root_gen_dir)')
    parser.add_argument(
        '--variant',
        required=True,
        help='Variant name (e.g., "objc")')
    parser.add_argument(
        '--dep-mojom',
        action='append',
        default=[],
        help='Dep mojom file path relative to // (may be repeated)')
    args = parser.parse_args()

    for dep_path in args.dep_mojom:
        for suffix in _FORWARDING_SUFFIXES:
            variant_filename = '%s-%s%s' % (dep_path, args.variant, suffix)
            default_filename = '%s%s' % (dep_path, suffix)

            output_path = os.path.join(args.output_dir, variant_filename)
            os.makedirs(os.path.dirname(output_path), exist_ok=True)

            content = _HEADER_TEMPLATE.format(default_path=default_filename)

            # Only write if content changed to avoid unnecessary rebuilds.
            if os.path.isfile(output_path):
                with open(output_path, 'r') as f:
                    if f.read() == content:
                        continue

            with open(output_path, 'w') as f:
                f.write(content)

    return 0


if __name__ == '__main__':
    sys.exit(main())
