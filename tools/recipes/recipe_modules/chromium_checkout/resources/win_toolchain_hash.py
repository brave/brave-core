#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Resolve the `GYP_MSVS_HASH_<hash>` override for the hermetic Windows
toolchain, run as a recipe step.

Chromium's `build/vs_toolchain.py` pins a `TOOLCHAIN_HASH` that gclient's
`win_toolchain` hook resolves to `<TOOLCHAIN_HASH>.zip` on Google's own
toolchain bucket. Brave republishes that same toolchain under a hash of its
own archive's content, alongside a sibling `<TOOLCHAIN_HASH>.yaml` index
naming it (see `tools/cr/toolchains/build_windows_toolchain.py`), since
producing a byte-identical archive isn't guaranteed.

`_GetDesiredVsToolchainHashes` in `build/vs_toolchain.py` substitutes hashes
via `GYP_MSVS_HASH_<TOOLCHAIN_HASH>`, so setting that env var to Brave's
published hash is what makes the hook fetch Brave's archive instead of
failing to find upstream's on Brave's bucket.

Writes `{"toolchain_hash": ..., "published_hash": ...}` as JSON to
`--json-output`. `published_hash` is `null` if no index has been published
yet for this upstream hash.

The caller can use this utility to set `GYP_MSVS_HASH_<hash>` for a checkout.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.error
import urllib.request

import yaml

# A single-quoted string value as it appears in `build/vs_toolchain.py`, e.g.
# the `'3bfcb536c8'` in `TOOLCHAIN_HASH = '3bfcb536c8'`.
_TOOLCHAIN_HASH_RE = re.compile(r"TOOLCHAIN_HASH = '([^']+)'")

_FETCH_TIMEOUT_SECS = 30


def _read_toolchain_hash(vs_toolchain_py: str) -> str:
    """Return the `TOOLCHAIN_HASH` pin from *vs_toolchain_py*'s source.

    Raises:
        RuntimeError: If no `TOOLCHAIN_HASH` assignment is found.
    """
    with open(vs_toolchain_py, encoding='utf-8') as f:
        text = f.read()
    match = _TOOLCHAIN_HASH_RE.search(text)
    if not match:
        raise RuntimeError(
            f'Could not find TOOLCHAIN_HASH in {vs_toolchain_py}.')
    return match.group(1)


def _fetch_published_hash(index_url: str) -> str | None:
    """Return the `hash` published at *index_url*, or `None` if not found.

    Raises:
        RuntimeError: If *index_url* could not be fetched (for a reason other
            than not existing yet) or its content is malformed.
    """
    try:
        with urllib.request.urlopen(index_url,
                                    timeout=_FETCH_TIMEOUT_SECS) as response:
            index = yaml.safe_load(response)
    except urllib.error.HTTPError as e:
        # Mirrors `build_windows_toolchain._remote_url_exists`: the download
        # bucket's CDN sometimes returns 403 where a 404 is expected for a
        # missing key.
        if e.code in (403, 404):
            return None
        raise RuntimeError(f'Failed to fetch {index_url}: {e}') from e
    except urllib.error.URLError as e:
        raise RuntimeError(f'Failed to fetch {index_url}: {e}') from e

    try:
        return index['hash']
    except (TypeError, KeyError) as e:
        raise RuntimeError(
            f'{index_url} has no "hash" entry: {index!r}') from e


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('vs_toolchain_py',
                        help='Path to the checked-out build/vs_toolchain.py.')
    parser.add_argument(
        'index_base_url',
        help='Base URL the sibling toolchain index is published under '
        '(the upstream toolchain hash + ".yaml" is appended to it).')
    parser.add_argument('--json-output',
                        required=True,
                        type=argparse.FileType('w'))
    opts = parser.parse_args(argv)

    toolchain_hash = _read_toolchain_hash(opts.vs_toolchain_py)
    index_url = f'{opts.index_base_url}{toolchain_hash}.yaml'
    published_hash = _fetch_published_hash(index_url)

    with opts.json_output as f:
        json.dump(
            {
                'toolchain_hash': toolchain_hash,
                'published_hash': published_hash,
            }, f)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
