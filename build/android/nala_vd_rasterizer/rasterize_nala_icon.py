#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Generates density PNGs from a Nala VectorDrawable at build time.

Invokes the NalaVdRasterizer java_binary wrapper once, rendering the input
vector into every requested density bucket. Used by the per-icon actions in
//brave/android/nala/BUILD.gn to override upstream raster icons with PNGs
derived from the Nala vector (the single source of truth).
"""

import argparse
import os
import subprocess
import sys

# Pixel-per-dp scale factor for each density bucket. An "ldrtl-" prefix (RTL
# layout direction) does not change the scale, so it is stripped before lookup.
DENSITY_SCALES = {
    'mdpi': '1',
    'hdpi': '1.5',
    'xhdpi': '2',
    'xxhdpi': '3',
    'xxxhdpi': '4',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rasterizer',
                        required=True,
                        help='Path to the NalaVdRasterizer wrapper script.')
    parser.add_argument('--input',
                        required=True,
                        help='Nala VectorDrawable xml.')
    parser.add_argument(
        '--output-dir',
        required=True,
        help='Resource res/ dir to write drawable-<bucket>/ into.')
    parser.add_argument('--dest-name',
                        required=True,
                        help='Output drawable name, without extension.')
    parser.add_argument(
        '--density',
        action='append',
        required=True,
        metavar='BUCKET',
        help='Density bucket, e.g. "xhdpi" or "ldrtl-xhdpi". Repeatable.')
    options = parser.parse_args()

    cmd = [options.rasterizer, options.input]
    for bucket in options.density:
        scale = DENSITY_SCALES.get(bucket.split('-')[-1])
        if scale is None:
            parser.error('unknown density bucket %r' % bucket)
        # ldrtl buckets hold the RTL artwork, which upstream ships as a
        # horizontal mirror of the LTR icon, so mirror the render to match.
        mirror = '1' if bucket.startswith('ldrtl-') else '0'
        out_dir = os.path.join(options.output_dir, 'drawable-' + bucket)
        os.makedirs(out_dir, exist_ok=True)
        cmd += [
            os.path.join(out_dir, options.dest_name + '.png'), scale, mirror
        ]

    return subprocess.run(cmd, check=False).returncode


if __name__ == '__main__':
    sys.exit(main())
