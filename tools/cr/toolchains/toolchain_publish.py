# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Shared publishing helpers for the hermetic toolchain builders.

`build_windows_toolchain.py`, `build_xcode_toolchain.py`, and
`build_rust_toolchain.py` each build one hermetic toolchain archive and
publish it alongside a sibling YAML index recording its provenance. The three
builders are otherwise unrelated (different inputs, different archive
formats), but that publishing step -- probing whether a toolchain is already
published, fetching a published index, writing one, and uploading the
archive + index pair -- is identical across all three, so it lives here
instead of being carried as three near-identical copies.
"""

from __future__ import annotations

import logging
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Iterable

import yaml

# This is necessary because this module is used by brockit too.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from cherry_picks import _check_call  # pylint: disable=wrong-import-position
from upload import (  # pylint: disable=wrong-import-position
    S3Uploader, summarise)

# Read timeout for a single HTTP fetch: a published index, or an "already
# published?" probe.
DEFAULT_TIMEOUT_SECS = 30

# Brave MPL license header prepended to every published YAML index.
INDEX_LICENSE_HEADER_TEMPLATE = (
    '# Copyright (c) {year} The Brave Authors. All rights reserved.\n'
    '# This Source Code Form is subject to the terms of the Mozilla Public\n'
    '# License, v. 2.0. If a copy of the MPL was not distributed with this '
    'file,\n'
    '# You can obtain one at https://mozilla.org/MPL/2.0/.\n')


def brave_core_commit() -> str:
    """Return the brave-core HEAD commit this script was run from.

    Every toolchain builder lives in brave-core, so its repository HEAD
    identifies the exact version that produced the archive -- recorded in
    the index for provenance.
    """
    return _check_call('git',
                       'rev-parse',
                       'HEAD',
                       cwd=Path(__file__).resolve().parent,
                       capture_output=True).stdout.strip()


def remote_url_exists(url: str, timeout: int = DEFAULT_TIMEOUT_SECS) -> bool:
    """Return whether *url* already resolves to a published object.

    Treats HTTP 403/404 as "not published" -- the download bucket's CDN
    sometimes returns 403 where a 404 is expected. Any other HTTP status or a
    network error propagates, so a transient failure is never mistaken for
    "absent".
    """
    try:
        with urllib.request.urlopen(url, timeout=timeout):
            return True
    except urllib.error.HTTPError as e:
        if e.code in (403, 404):
            return False
        raise


def fetch_index(index_url: str,
                description: str = 'toolchain',
                timeout: int = DEFAULT_TIMEOUT_SECS) -> dict:
    """Download and parse a published toolchain sibling YAML index.

    *description* customises the log/error messages (e.g. `'hermetic Windows
    toolchain'`).

    Raises:
        RuntimeError: if the index cannot be fetched.
    """
    logging.debug('Fetching %s index %s', description, index_url)
    try:
        with urllib.request.urlopen(index_url, timeout=timeout) as response:
            return yaml.safe_load(response)
    except urllib.error.URLError as e:
        raise RuntimeError(
            f'Failed to fetch {description} index {index_url}: {e}') from e


def write_index_file(index_path: Path, index: dict) -> None:
    """Write *index* as a Brave-MPL-licensed YAML file at *index_path*.

    Every sibling index a toolchain builder publishes is written the same
    way: a license header followed by the mapping dumped with stable
    (insertion) key order. After writing, the file is read back and echoed to
    the console.
    """
    index_yaml = yaml.safe_dump(index,
                                sort_keys=False,
                                default_flow_style=False)
    license_header = INDEX_LICENSE_HEADER_TEMPLATE.format(
        year=time.gmtime().tm_year)
    index_path.write_text(f'{license_header}\n{index_yaml}',
                          encoding='utf-8',
                          newline='')
    logging.info('Wrote toolchain index %s:', index_path)
    print(index_path.read_bytes().decode('utf-8'))


def upload_files(bucket: str, prefix: str, paths: Iterable[Path]) -> None:
    """Upload each of *paths* to *bucket* under *prefix*, logging a summary.

    Shared by every toolchain builder's `--upload` step: each publishes an
    archive plus its sibling YAML index the same way, unsigned -- these
    internal/public build-deps buckets don't use the KMS-signed envelope
    `upload.py` otherwise supports.
    """
    uploader = S3Uploader(bucket=bucket)
    for path in paths:
        result = uploader.upload(path, prefix=prefix, sign=False)
        logging.info('Uploaded %s:\n%s', path.name, summarise(result))
