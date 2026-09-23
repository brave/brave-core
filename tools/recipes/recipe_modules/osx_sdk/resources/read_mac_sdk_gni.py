#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Resolve the macOS SDK version/build pin from a checked-out `mac_sdk.gni`,
run as a recipe step.

`build/config/mac/mac_sdk.gni` is where Chromium pins the macOS SDK build via
`mac_sdk_official_version` / `mac_sdk_official_build_version`. `gn`
auto-formats these assignments as `key = "value"`, so a simple regex recovers
each value -- the same approach
`tools/cr/toolchains/ephemeral_xcode.py`'s `MacSdkInfo.from_gni` takes on the
brave-core side; a recipe module can't import that script's Python directly
(see `osx_sdk/api.py`), so this is a small, deliberate duplicate of its
parsing that should stay in sync with it.

Writes `{"sdk_version": ..., "sdk_build_version": ...}` as JSON to
`--json-output`.
"""

from __future__ import annotations

import argparse
import json
import re
import sys

# A double-quoted string value as it appears in `build/config/mac/mac_sdk.gni`,
# e.g. the `"26.5"` in `mac_sdk_official_version = "26.5"`.
_GN_VALUE_RE = '{key} = "([^"]+)"'


def _gn_value(text: str, key: str, source: str) -> str:
    """Return the value assigned to *key* in *text*.

    Raises:
        RuntimeError: If no such assignment is found.
    """
    match = re.search(_GN_VALUE_RE.format(key=key), text)
    if not match:
        raise RuntimeError(f'{key} not found in {source}')
    return match.group(1)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'mac_sdk_gni',
        help='Path to the checked-out build/config/mac/mac_sdk.gni.')
    parser.add_argument('--json-output',
                        required=True,
                        type=argparse.FileType('w'))
    args = parser.parse_args(argv)

    with open(args.mac_sdk_gni, encoding='utf-8') as f:
        text = f.read()

    result = {
        'sdk_version': _gn_value(text, 'mac_sdk_official_version',
                                 args.mac_sdk_gni),
        'sdk_build_version': _gn_value(text, 'mac_sdk_official_build_version',
                                       args.mac_sdk_gni),
    }

    with args.json_output as f:
        json.dump(result, f)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
