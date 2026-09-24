#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `build_rust_toolchain`'s sibling-index naming and lookup.

`urllib.request.urlopen` is mocked throughout (no network access).

Coverage:

* `ToolchainIndexNameTest` -- the `<platform>-<upstream_stem>-
  <brave_subrevision>.yaml` sibling-index naming.
* `RustToolchainExtraDepTest` -- `rust_toolchain_extra_dep` end to end: for
  every supported platform it must derive the exact sibling-index URL and
  archive object name (`<platform>-<upstream_stem>-<brave_subrevision>
  .tar.xz`), fetch the (mocked) index, and assemble the `EXTRA_DEPS` entry.
  This is the per-platform object-name/URL derivation that used to be
  exercised (as per-platform archive downloads) in `toolchain_test.py`'s
  `RustRepinTest` before the sibling-index redesign; it now lives here
  instead.
"""

from __future__ import annotations

import io
import sys
import unittest
import urllib.error
from pathlib import Path
from unittest import mock

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parent))

import build_rust_toolchain as m  # pylint: disable=wrong-import-position


def _index_response(entry: dict) -> io.BytesIO:
    """A `urlopen`-compatible stand-in serving *entry* as YAML bytes."""
    return io.BytesIO(yaml.safe_dump(entry).encode('utf-8'))


class ToolchainIndexNameTest(unittest.TestCase):
    """Tests for `toolchain_index_name`."""

    def test_names_the_sibling_index_after_platform_stem_and_subrevision(self):
        self.assertEqual(
            m.toolchain_index_name('linux-x64',
                                   'rust-toolchain-abc-2-llvmorg-x', 3),
            'linux-x64-rust-toolchain-abc-2-llvmorg-x-3.yaml')

    def test_distinct_brave_subrevisions_get_distinct_index_names(self):
        first = m.toolchain_index_name('win', 'rust-toolchain-abc', 1)
        second = m.toolchain_index_name('win', 'rust-toolchain-abc', 2)
        self.assertNotEqual(first, second)


class RustToolchainExtraDepTest(unittest.TestCase):
    """Tests for `rust_toolchain_extra_dep`."""

    UPSTREAM_STEM = 'rust-toolchain-abc123def-2-llvmorg-23-init-10931-g20b6ec66'
    BRAVE_SUBREVISION = 3

    def _expected_index_url(self, platform_prefix: str) -> str:
        return (f'{m.TOOLCHAIN_BUCKET_URL}/{platform_prefix}-'
                f'{self.UPSTREAM_STEM}-{self.BRAVE_SUBREVISION}.yaml')

    def _expected_object_name(self, platform_prefix: str) -> str:
        return (f'{platform_prefix}-{self.UPSTREAM_STEM}-'
                f'{self.BRAVE_SUBREVISION}.tar.xz')

    def _responses(self) -> dict[str, dict]:
        """One well-formed index entry per supported platform, keyed by the
        exact sibling-index URL that platform's entry must be served at."""
        return {
            self._expected_index_url(platform_prefix): {
                'url': (f'{m.TOOLCHAIN_BUCKET_URL}/'
                        f'{self._expected_object_name(platform_prefix)}'),
                'sha256sum': f'sha-{platform_prefix}',
                'size_bytes': len(platform_prefix),
            }
            for platform_prefix in m.SUPPORTED_PLATFORM_CONDITIONS
        }

    def test_derives_url_and_object_name_per_platform(self):
        responses = self._responses()

        def fake_urlopen(url, timeout=None):
            del timeout
            # KeyError (-> test failure) if a URL other than the exact,
            # hand-computed sibling-index URL for some platform is requested.
            return _index_response(responses[url])

        with mock.patch('urllib.request.urlopen',
                        side_effect=fake_urlopen) as urlopen:
            extra_dep = m.rust_toolchain_extra_dep(self.UPSTREAM_STEM,
                                                   self.BRAVE_SUBREVISION)

        self.assertEqual(urlopen.call_count,
                         len(m.SUPPORTED_PLATFORM_CONDITIONS))
        for platform_prefix in m.SUPPORTED_PLATFORM_CONDITIONS:
            urlopen.assert_any_call(
                self._expected_index_url(platform_prefix),
                timeout=m.toolchain_publish.DEFAULT_TIMEOUT_SECS)

        objects = extra_dep[m.RUST_TOOLCHAIN_DEP_PATH]['objects']
        self.assertEqual(len(objects), len(m.SUPPORTED_PLATFORM_CONDITIONS))

        by_name = {obj['object_name']: obj for obj in objects}
        linux_name = self._expected_object_name('linux-x64')
        self.assertIn(linux_name, by_name)
        linux_obj = by_name[linux_name]
        self.assertEqual(linux_obj['sha256sum'], 'sha-linux-x64')
        self.assertEqual(linux_obj['size_bytes'], len('linux-x64'))
        self.assertEqual(linux_obj['overlayed_on'],
                         f'Linux_x64/{self.UPSTREAM_STEM}.tar.xz')
        self.assertEqual(linux_obj['condition'], 'host_os == "linux"')

        win_name = self._expected_object_name('win')
        self.assertIn(win_name, by_name)
        self.assertEqual(by_name[win_name]['overlayed_on'],
                         f'Win/{self.UPSTREAM_STEM}.tar.xz')

        self.assertEqual(extra_dep[m.RUST_TOOLCHAIN_DEP_PATH]['bucket'],
                         f'{m.TOOLCHAIN_BUCKET_URL}/')
        self.assertEqual(extra_dep[m.RUST_TOOLCHAIN_DEP_PATH]['condition'],
                         m.RUST_TOOLCHAIN_DEP_CONDITION)

    def test_distinct_brave_subrevisions_derive_distinct_urls(self):
        seen_urls = []

        def fake_urlopen(url, timeout=None):
            del timeout
            seen_urls.append(url)
            object_name = url.rsplit('/', 1)[-1].removesuffix('.yaml')
            return _index_response({
                'url': f'{m.TOOLCHAIN_BUCKET_URL}/{object_name}.tar.xz',
                'sha256sum': 'sha',
                'size_bytes': 1,
            })

        with mock.patch('urllib.request.urlopen', side_effect=fake_urlopen):
            m.rust_toolchain_extra_dep(self.UPSTREAM_STEM, 1)
            m.rust_toolchain_extra_dep(self.UPSTREAM_STEM, 2)

        first_round, second_round = seen_urls[:4], seen_urls[4:]
        self.assertEqual(len(first_round), 4)
        self.assertEqual(len(second_round), 4)
        self.assertFalse(set(first_round) & set(second_round))

    def test_missing_index_raises(self):
        error = urllib.error.HTTPError('https://example.invalid/x', 404,
                                       'Not Found', {}, io.BytesIO(b''))
        with mock.patch('urllib.request.urlopen', side_effect=error):
            with self.assertRaises(RuntimeError):
                m.rust_toolchain_extra_dep(self.UPSTREAM_STEM,
                                           self.BRAVE_SUBREVISION)

    def test_index_pointing_at_a_different_object_raises(self):
        # A well-formed index, but its `url` names an object other than the
        # one this exact platform/subrevision should have -- i.e. it was
        # fetched from (or published at) the wrong key.
        def fake_urlopen(url, timeout=None):
            del url, timeout
            return _index_response({
                'url': f'{m.TOOLCHAIN_BUCKET_URL}/unexpected.tar.xz',
                'sha256sum': 'sha',
                'size_bytes': 1,
            })

        with mock.patch('urllib.request.urlopen', side_effect=fake_urlopen):
            with self.assertRaises(RuntimeError):
                m.rust_toolchain_extra_dep(self.UPSTREAM_STEM,
                                           self.BRAVE_SUBREVISION)

    def test_malformed_index_raises(self):
        with mock.patch('urllib.request.urlopen',
                        return_value=_index_response({'unexpected': 'shape'})):
            with self.assertRaises(RuntimeError):
                m.rust_toolchain_extra_dep(self.UPSTREAM_STEM,
                                           self.BRAVE_SUBREVISION)


if __name__ == '__main__':
    unittest.main()
