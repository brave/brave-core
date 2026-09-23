#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Emits a horizontally mirrored copy of a Nala vector drawable.

Upstream ships a few icons as an LTR/RTL pair selected through a values-ldrtl
alias, so overriding only the LTR name would leave the mirrored variant on
upstream art. Rather than checking in a second hand-written vector, the RTL half
is generated from the same Nala source by wrapping its contents in a group that
flips it about the viewport's vertical axis.
"""

import argparse
import re
import sys

# Matches the <vector> opening tag, which carries the viewport dimensions.
_VECTOR_OPEN_RE = re.compile(r'<vector\b[^>]*?>', re.DOTALL)
_VIEWPORT_WIDTH_RE = re.compile(r'android:viewportWidth="([\d.]+)"')


def mirror(source: str) -> str:
    open_tag = _VECTOR_OPEN_RE.search(source)
    if not open_tag:
        raise ValueError('no <vector> element found')

    viewport_width = _VIEWPORT_WIDTH_RE.search(open_tag.group(0))
    if not viewport_width:
        raise ValueError('<vector> has no android:viewportWidth')

    close_index = source.rindex('</vector>')

    # Mirroring about the vertical axis moves the drawing off-canvas, so shift
    # it back by the viewport width.
    group_open = ('    <group\n'
                  '        android:scaleX="-1"\n'
                  f'        android:translateX="{viewport_width.group(1)}">\n')

    body = source[open_tag.end():close_index].strip('\n')
    return (source[:open_tag.end()] + '\n' + group_open + body + '\n' +
            '    </group>\n' + source[close_index:])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', required=True, help='Nala vector drawable')
    parser.add_argument('--output', required=True, help='mirrored drawable')
    args = parser.parse_args()

    with open(args.input, 'r', encoding='utf-8') as f:
        source = f.read()

    try:
        mirrored = mirror(source)
    except ValueError as e:
        sys.stderr.write(f'{args.input}: {e}\n')
        return 1

    with open(args.output, 'w', encoding='utf-8') as f:
        f.write(mirrored)

    return 0


if __name__ == '__main__':
    sys.exit(main())
