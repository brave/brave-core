# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Fetch raw file contents from Chromium's gitiles (googlesource) mirror.
"""

from __future__ import annotations

import base64
import logging
import time
import urllib.error
import urllib.request

# Gitiles raw-text endpoint template for a `chromium/src` file at a release
# tag. The `?format=TEXT` suffix returns the file as a single base64-encoded
# blob with no surrounding HTML or headers.
CHROMIUM_SRC_FILE_URL_TEMPLATE = (
    'https://chromium.googlesource.com/chromium/src/+/refs/tags/{tag}/{path}'
    '?format=TEXT')

# Per-request timeout for gitiles fetches.
HTTP_FETCH_TIMEOUT_SECS = 30

# Retry policy for gitiles fetches: gitiles can flake on freshly pushed tags.
GITILES_FETCH_MAX_ATTEMPTS = 3
GITILES_FETCH_RETRY_DELAY_SECS = 2


def fetch_raw(url: str) -> str:
    """Fetch *url* (a gitiles `?format=TEXT` link) and decode its body.

    Gitiles' raw-text endpoint returns the requested file as a single
    base64-encoded blob with no surrounding HTML or headers. Retries a few
    times since gitiles can flake on freshly pushed tags.

    Raises:
        urllib.error.URLError: (including `HTTPError`) if every attempt fails
            with a network or HTTP error.
        ValueError: if the response cannot be base64/UTF-8-decoded on the
            final attempt.
    """
    for attempt in range(1, GITILES_FETCH_MAX_ATTEMPTS + 1):
        logging.info('Fetching %s (attempt %d/%d)', url, attempt,
                     GITILES_FETCH_MAX_ATTEMPTS)
        is_last_attempt = attempt == GITILES_FETCH_MAX_ATTEMPTS
        encoded: bytes | None = None
        try:
            with urllib.request.urlopen(
                    url, timeout=HTTP_FETCH_TIMEOUT_SECS) as response:
                encoded = response.read()
            return base64.b64decode(encoded).decode('utf-8')
        except urllib.error.HTTPError as e:
            # `HTTPError.read()` returns the response body. Let's log it.
            body = e.read().decode('utf-8', errors='replace')
            logging.error('HTTP %s on %s; response body:\n%s', e.code, url,
                          body)
            if is_last_attempt:
                raise
        except (urllib.error.URLError, TimeoutError) as e:
            # No response body for non-HTTP failures (DNS, connect timeout,
            # read timeout). Just surface the underlying reason.
            logging.error('Network error fetching %s: %s', url, e)
            if is_last_attempt:
                raise
        except ValueError as e:
            # Let's log the raw response whenever there are decoding issues.
            preview = (encoded or b'').decode('utf-8', errors='replace')
            logging.error('Decode failed for %s: %s; raw response:\n%s', url,
                          e, preview)
            if is_last_attempt:
                raise
        time.sleep(GITILES_FETCH_RETRY_DELAY_SECS)
    # Unreachable: the final attempt's except handlers always re-raise.
    # Present so every control-flow path honors the `-> str` signature
    # (pylint inconsistent-return-statements).
    raise RuntimeError(f'fetch_raw fell through retry loop: {url}')


def fetch_chromium_file(tag: str, path: str) -> str:
    """Fetch *path*'s raw contents from `chromium/src` at release *tag*.

    Convenience wrapper around `fetch_raw` for the common case every current
    caller wants: a single `chromium/src` file at a specific release tag (e.g.
    `build/vs_toolchain.py`, `build/config/mac/mac_sdk.gni`).
    """
    url = CHROMIUM_SRC_FILE_URL_TEMPLATE.format(tag=tag, path=path)
    return fetch_raw(url)
