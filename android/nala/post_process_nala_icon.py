#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Emits a Nala vector drawable with the requested transforms applied.

--mirror emits a horizontally mirrored copy. Upstream ships a few icons as an
LTR/RTL pair selected through a values-ldrtl alias, so overriding only the LTR
name would leave the mirrored variant on upstream art. Rather than checking in a
second hand-written vector, the RTL half is generated from the same Nala source
by wrapping its contents in a group that flips it about the viewport's vertical
axis.

--tint adds android:tint to the <vector> root. Nala vectors carry their colour
on the path as android:fillColor="@color/...", a plain resource reference that
resolves from the configuration alone, while the upstream drawables being
overridden tint the whole vector through a theme attribute, which is late-bound
against the inflating context's theme and so follows dynamic colours. An
untinted Nala copy would freeze the icon at Nala's static fill, so entries whose
call site applies no tint of its own re-add the upstream tint here. With the
default src_in tint mode the tint's RGB replaces the opaque Nala fill, leaving
only the glyph shape from Nala.

The two transforms are independent and may be combined: the tint lands on the
root element, the mirror wraps the root's children.
"""

import argparse
import re
import sys

# Matches the <vector> opening tag, which carries the viewport dimensions and is
# where android:tint belongs.
_VECTOR_OPEN_RE = re.compile(r'<vector\b[^>]*?>', re.DOTALL)
_VIEWPORT_WIDTH_RE = re.compile(r'android:viewportWidth="([\d.]+)"')


def tint(source: str, color: str) -> str:
    open_tag = _VECTOR_OPEN_RE.search(source)
    if not open_tag:
        raise ValueError('no <vector> element found')

    if 'android:tint' in open_tag.group(0):
        raise ValueError('<vector> already declares android:tint')

    # Insert before the opening tag's closing '>'.
    insert_at = open_tag.end() - 1
    return (
        source[:insert_at]
        + f'\n    android:tint="{color}"'
        + source[insert_at:]
    )


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
    group_open = (
        '    <group\n'
        '        android:scaleX="-1"\n'
        f'        android:translateX="{viewport_width.group(1)}">\n'
    )

    body = source[open_tag.end() : close_index].strip('\n')
    return (
        source[: open_tag.end()]
        + '\n'
        + group_open
        + body
        + '\n'
        + '    </group>\n'
        + source[close_index:]
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', required=True, help='Nala vector drawable')
    parser.add_argument('--output', required=True, help='transformed drawable')
    parser.add_argument(
        '--mirror',
        action='store_true',
        help='flip horizontally, for an RTL variant',
    )
    parser.add_argument(
        '--tint', help='tint value, e.g. @macro/default_icon_color'
    )
    args = parser.parse_args()

    if not args.mirror and not args.tint:
        parser.error('nothing to do: pass --mirror and/or --tint')

    with open(args.input, 'r', encoding='utf-8') as f:
        result = f.read()

    try:
        if args.tint:
            result = tint(result, args.tint)
        if args.mirror:
            result = mirror(result)
    except ValueError as e:
        sys.stderr.write(f'{args.input}: {e}\n')
        return 1

    with open(args.output, 'w', encoding='utf-8') as f:
        f.write(result)

    return 0


if __name__ == '__main__':
    sys.exit(main())
