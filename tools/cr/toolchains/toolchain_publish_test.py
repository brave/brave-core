#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `toolchain_publish`.

`urllib.request.urlopen` is mocked throughout (no network access); `S3Uploader`
is mocked in `UploadFilesTest` (its own behavior is covered by `upload_test.py`).
`brave_core_commit` is the one exception: it is exercised against the real
brave-core checkout this test runs from, rather than mocking `git`.

Coverage:

* `BraveCoreCommitTest` -- resolves to the real `git rev-parse HEAD` of the
  checkout this module lives in.
* `RemoteUrlExistsTest` -- the 403/404-tolerant "already published?" probe.
* `FetchIndexTest` -- downloading and parsing a published sibling index, and
  wrapping a fetch failure in a `RuntimeError` that names *description*.
* `WriteIndexFileTest` -- the license header + YAML dump + echo-to-console
  write path.
* `UploadFilesTest` -- `S3Uploader` construction and the per-path
  `prefix`/`sign=False` upload call.
"""

from __future__ import annotations

import contextlib
import io
import subprocess
import sys
import tempfile
import time
import unittest
import urllib.error
from pathlib import Path
from unittest import mock

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parent))

import toolchain_publish as m  # pylint: disable=wrong-import-position


class _FakeContextResponse:
    """A context-manager stand-in for `http.client.HTTPResponse`, used where
    the response body itself is never read (only entering the `with` block
    matters)."""

    def __enter__(self):
        return self

    def __exit__(self, *_exc):
        return False


class BraveCoreCommitTest(unittest.TestCase):
    """Tests for `brave_core_commit`."""

    def test_resolves_to_the_real_checkout_head(self):
        expected = subprocess.run(('git', 'rev-parse', 'HEAD'),
                                  cwd=Path(m.__file__).resolve().parent,
                                  check=True,
                                  capture_output=True,
                                  text=True).stdout.strip()
        self.assertEqual(m.brave_core_commit(), expected)


class RemoteUrlExistsTest(unittest.TestCase):
    """Tests for `remote_url_exists`."""

    URL = 'https://example.invalid/toolchain.yaml'

    def test_true_when_url_resolves(self):
        with mock.patch('urllib.request.urlopen',
                        return_value=_FakeContextResponse()) as urlopen:
            self.assertTrue(m.remote_url_exists(self.URL))
        urlopen.assert_called_once_with(self.URL,
                                        timeout=m.DEFAULT_TIMEOUT_SECS)

    def test_false_on_404(self):
        error = urllib.error.HTTPError(self.URL, 404, 'Not Found', {},
                                       io.BytesIO(b''))
        with mock.patch('urllib.request.urlopen', side_effect=error):
            self.assertFalse(m.remote_url_exists(self.URL))

    def test_false_on_403(self):
        # The download bucket's CDN sometimes returns 403 where a 404 is
        # expected.
        error = urllib.error.HTTPError(self.URL, 403, 'Forbidden', {},
                                       io.BytesIO(b''))
        with mock.patch('urllib.request.urlopen', side_effect=error):
            self.assertFalse(m.remote_url_exists(self.URL))

    def test_other_http_error_propagates(self):
        error = urllib.error.HTTPError(self.URL, 500, 'Internal Server Error',
                                       {}, io.BytesIO(b''))
        with mock.patch('urllib.request.urlopen', side_effect=error):
            with self.assertRaises(urllib.error.HTTPError):
                m.remote_url_exists(self.URL)

    def test_honours_custom_timeout(self):
        with mock.patch('urllib.request.urlopen',
                        return_value=_FakeContextResponse()) as urlopen:
            m.remote_url_exists(self.URL, timeout=5)
        urlopen.assert_called_once_with(self.URL, timeout=5)


class FetchIndexTest(unittest.TestCase):
    """Tests for `fetch_index`."""

    URL = 'https://example.invalid/toolchain.yaml'

    def test_parses_the_published_yaml_index(self):
        body = yaml.safe_dump({'sha256sum': 'abc', 'size_bytes': 3})
        with mock.patch('urllib.request.urlopen',
                        return_value=io.BytesIO(
                            body.encode('utf-8'))) as urlopen:
            result = m.fetch_index(self.URL)
        self.assertEqual(result, {'sha256sum': 'abc', 'size_bytes': 3})
        urlopen.assert_called_once_with(self.URL,
                                        timeout=m.DEFAULT_TIMEOUT_SECS)

    def test_wraps_fetch_failure_naming_the_description(self):
        with mock.patch('urllib.request.urlopen',
                        side_effect=urllib.error.URLError('boom')):
            with self.assertRaises(RuntimeError) as ctx:
                m.fetch_index(self.URL,
                              description='hermetic Windows toolchain')
        self.assertIn('hermetic Windows toolchain', str(ctx.exception))
        self.assertIn(self.URL, str(ctx.exception))

    def test_default_description_is_toolchain(self):
        with mock.patch('urllib.request.urlopen',
                        side_effect=urllib.error.URLError('boom')):
            with self.assertRaises(RuntimeError) as ctx:
                m.fetch_index(self.URL)
        self.assertIn('toolchain index', str(ctx.exception))

    def test_honours_custom_timeout(self):
        body = yaml.safe_dump({'k': 'v'})
        with mock.patch('urllib.request.urlopen',
                        return_value=io.BytesIO(
                            body.encode('utf-8'))) as urlopen:
            m.fetch_index(self.URL, timeout=5)
        urlopen.assert_called_once_with(self.URL, timeout=5)


class WriteIndexFileTest(unittest.TestCase):
    """Tests for `write_index_file`."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.index_path = Path(self._tmp.name) / 'toolchain.yaml'

    def test_writes_license_header_then_the_yaml_mapping(self):
        with mock.patch.object(m.time,
                               'gmtime',
                               return_value=time.struct_time((2031, ) +
                                                             (0, ) * 8)):
            m.write_index_file(self.index_path,
                               {'url': 'https://example.invalid/x.tar.xz'})

        text = self.index_path.read_text(encoding='utf-8')
        header = m.INDEX_LICENSE_HEADER_TEMPLATE.format(year=2031)
        self.assertTrue(text.startswith(header))

        # The mapping survives round-tripping, unaffected by the `#`-comment
        # license header preceding it.
        parsed = yaml.safe_load(text)
        self.assertEqual(parsed, {'url': 'https://example.invalid/x.tar.xz'})

    def test_preserves_insertion_order_over_sorted_order(self):
        index = {'z_field': 1, 'a_field': 2}
        m.write_index_file(self.index_path, index)
        text = self.index_path.read_text(encoding='utf-8')
        self.assertLess(text.index('z_field'), text.index('a_field'))

    def test_echoes_written_content_to_the_console(self):
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            m.write_index_file(self.index_path, {'k': 'v'})
        # `print` appends its own trailing newline on top of the file's.
        self.assertEqual(stdout.getvalue(),
                         self.index_path.read_text(encoding='utf-8') + '\n')


class UploadFilesTest(unittest.TestCase):
    """Tests for `upload_files`."""

    def test_uploads_every_path_with_bucket_prefix_unsigned(self):
        calls = []
        fake_uploader = mock.Mock()
        fake_uploader.upload.side_effect = (
            lambda path, prefix, sign: calls.append((path, prefix, sign)))

        paths = [Path('archive.tar.xz'), Path('archive.yaml')]
        with mock.patch.object(m, 'S3Uploader',
                               return_value=fake_uploader) as uploader_cls, \
             mock.patch.object(m, 'summarise', return_value='summary'):
            m.upload_files('my-bucket', 'my-prefix', paths)

        uploader_cls.assert_called_once_with(bucket='my-bucket')
        self.assertEqual(calls, [
            (Path('archive.tar.xz'), 'my-prefix', False),
            (Path('archive.yaml'), 'my-prefix', False),
        ])

    def test_uploads_nothing_for_an_empty_path_list(self):
        fake_uploader = mock.Mock()
        with mock.patch.object(m, 'S3Uploader', return_value=fake_uploader):
            m.upload_files('my-bucket', 'my-prefix', [])
        fake_uploader.upload.assert_not_called()


if __name__ == '__main__':
    unittest.main()
