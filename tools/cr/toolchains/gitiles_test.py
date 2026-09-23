#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `gitiles`.

`urllib.request.urlopen` is mocked throughout (no network access); retry
delays are mocked out too so the retry tests run instantly.

Coverage:

* `FetchRawTest` — success on the first attempt, a retry-then-succeed after a
  transient network error, and giving up after exhausting retries for each of
  the three failure modes (`HTTPError`, `URLError`, `TimeoutError`), plus the
  decode-failure path (bad base64) and the exact retry attempt count.
* `FetchChromiumFileTest` — URL construction from a tag + path, delegating to
  `fetch_raw`.
"""

from __future__ import annotations

import base64
import io
import sys
import unittest
import urllib.error
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

import gitiles as m  # pylint: disable=wrong-import-position


class _FakeResponse:
    """A context-manager stand-in for `http.client.HTTPResponse`."""

    def __init__(self, body: bytes):
        self._body = body

    def read(self) -> bytes:
        return self._body

    def __enter__(self):
        return self

    def __exit__(self, *_exc):
        return False


def _encoded(text: str) -> bytes:
    """Base64-encode *text*, mirroring gitiles' `?format=TEXT` response body.
    """
    return base64.b64encode(text.encode('utf-8'))


class FetchRawTest(unittest.TestCase):
    """Tests for `fetch_raw`."""

    def setUp(self):
        # Tests shouldn't actually sleep between retries.
        patcher = mock.patch('time.sleep')
        self.addCleanup(patcher.stop)
        patcher.start()

    def test_success_decodes_base64_body_on_first_attempt(self):
        with mock.patch('urllib.request.urlopen',
                        return_value=_FakeResponse(
                            _encoded('hello world'))) as urlopen:
            result = m.fetch_raw('https://example.invalid/x')
        self.assertEqual(result, 'hello world')
        urlopen.assert_called_once_with('https://example.invalid/x',
                                        timeout=m.HTTP_FETCH_TIMEOUT_SECS)

    def test_retries_then_succeeds_after_transient_network_error(self):
        responses = [
            urllib.error.URLError('connection reset'),
            _FakeResponse(_encoded('recovered')),
        ]
        with mock.patch('urllib.request.urlopen', side_effect=responses):
            result = m.fetch_raw('https://example.invalid/x')
        self.assertEqual(result, 'recovered')

    def test_gives_up_after_max_attempts_on_http_error(self):
        error = urllib.error.HTTPError('https://example.invalid/x', 500,
                                       'Internal Server Error', {},
                                       io.BytesIO(b'boom'))
        with mock.patch('urllib.request.urlopen', side_effect=error):
            with self.assertRaises(urllib.error.HTTPError):
                m.fetch_raw('https://example.invalid/x')

    def test_gives_up_after_max_attempts_on_url_error(self):
        with mock.patch('urllib.request.urlopen',
                        side_effect=urllib.error.URLError('nope')):
            with self.assertRaises(urllib.error.URLError):
                m.fetch_raw('https://example.invalid/x')

    def test_gives_up_after_max_attempts_on_timeout(self):
        with mock.patch('urllib.request.urlopen', side_effect=TimeoutError):
            with self.assertRaises(TimeoutError):
                m.fetch_raw('https://example.invalid/x')

    def test_gives_up_on_undecodable_body(self):
        # Not valid base64: decoding fails on every attempt.
        with mock.patch('urllib.request.urlopen',
                        return_value=_FakeResponse(b'!!!not-base64!!!')):
            with self.assertRaises(ValueError):
                m.fetch_raw('https://example.invalid/x')

    def test_retries_exactly_max_attempts_times_before_giving_up(self):
        with mock.patch('urllib.request.urlopen',
                        side_effect=urllib.error.URLError('nope')) as urlopen:
            with self.assertRaises(urllib.error.URLError):
                m.fetch_raw('https://example.invalid/x')
        self.assertEqual(urlopen.call_count, m.GITILES_FETCH_MAX_ATTEMPTS)


class FetchChromiumFileTest(unittest.TestCase):
    """Tests for `fetch_chromium_file`."""

    def test_builds_gitiles_tag_url_and_delegates_to_fetch_raw(self):
        with mock.patch.object(m, 'fetch_raw',
                               return_value='file contents') as fetch_raw:
            result = m.fetch_chromium_file('150.0.7841.1',
                                           'build/vs_toolchain.py')
        self.assertEqual(result, 'file contents')
        fetch_raw.assert_called_once_with(
            'https://chromium.googlesource.com/chromium/src/+/refs/tags/'
            '150.0.7841.1/build/vs_toolchain.py?format=TEXT')


if __name__ == '__main__':
    unittest.main()
