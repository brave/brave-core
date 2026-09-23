#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `ephemeral_xcode`.

`urllib.request.urlopen` and `_check_call` are mocked throughout (no network
access, no real `xcode-select`/`xcodebuild`/`sudo`); filesystem-touching pieces
(`_install`, `_download_and_expand`) run against a real temp dir instead of
mocking `pathlib`/`shutil`, since that's cheap and exercises the real move/
cleanup behavior.

Coverage:

* `VersionSortKeyTest` -- numeric (not lexicographic) version-tuple parsing.
* `Sha1OfFileTest` -- chunked SHA-1 digest.
* `MacSdkInfoFromGniTest` -- `mac_sdk.gni` assignment parsing, and the missing-
  assignment error.
* `CheckDeveloperModeTest` -- the Developer Mode preflight.
* `MacosMajorVersionTest` -- `sw_vers -productVersion` major-version parsing.
* `FetchXcodeReleasesTest` -- the catalog fetch/parse.
* `DownloadToFileTest` -- streaming a response body to disk, with and without
  a `Content-Length` header.
* `ResolveReleaseTest` -- the catalog-matching logic, including regression
  coverage for two production bugs: a `KeyError` on catalog entries with no
  `sdks` key at all (pre-catalog Xcode releases), and an unhandled ambiguity
  when two final releases bundle the identical SDK build.
* `InstallTest` -- reusing an already-expanded Xcode vs. triggering a fresh
  download+expand.
* `DownloadAndExpandTest` -- the `unxip`/`xip --expand` choice, the move into
  place, stale-expand-dir cleanup, and the missing-`Xcode.app` failure.
* `DownloadXipTest` -- the SHA-1-verified download with its stripped-
  `_Universal` fallback URL and mismatch/retry handling.
* `SwitchTest` -- the full `xcode-select` switch sequence, including the
  Developer Mode preflight, the macOS-14+-only Gatekeeper scan, the
  timeout-bounded license-accept/`-runFirstLaunch` calls, and the stale-
  `ibtoold` kill.
* `SelectContextManagerTest` -- switch-then-reset, including on exception.
* `LocateAppTest` -- deriving the active `Xcode.app` from `xcode-select -p`.
* `VerifyVersionsTest` -- the active-Xcode/SDK cross-check against the
  resolved release and the pin.
* `InstallAndSelectTest` / `DeployTest` -- the end-to-end orchestration, call
  order, and reset-on-exception guarantee.
* `ResetTest` -- `xcode-select --reset`.
* `PropertiesTest` -- `app`/`release` raising before they're populated.
* `MainTest` -- the CLI: required arguments, the JSON output written, and
  `--verbose` wiring.
"""

from __future__ import annotations

import contextlib
import hashlib
import io
import json
import subprocess
import sys
import tempfile
import unittest
import urllib.error
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

import ephemeral_xcode as m  # pylint: disable=wrong-import-position


def _completed(stdout: str = '') -> subprocess.CompletedProcess:
    """A `_check_call`-shaped result carrying just *stdout*."""
    return subprocess.CompletedProcess([], 0, stdout=stdout)


def _xcode_entry(number: str,
                 build: str,
                 *,
                 sdk_build: str | None = None,
                 release: str = 'final',
                 filename: str | None = None,
                 sha1: str = 'deadbeef',
                 universal: bool = True,
                 has_url: bool = True,
                 name: str = 'Xcode') -> dict:
    """Build a minimal xcodereleases.com-shaped catalog entry.

    Args:
        sdk_build: The macOS SDK build this entry bundles. `None` omits the
            `sdks` key entirely, matching pre-catalog Xcode releases.
        release: `'final'`, `'beta'`, or `'rc'`; controls the `version.release`
            object's shape.
        has_url: When `False`, omits `links.download.url` entirely.
    """
    version: dict = {'number': number, 'build': build}
    if release == 'final':
        version['release'] = {'release': True}
    elif release == 'beta':
        version['release'] = {'beta': 1}
    elif release == 'rc':
        version['release'] = {'rc': 1}

    entry: dict = {'name': name, 'version': version}
    if sdk_build is not None:
        entry['sdks'] = {'macOS': [{'build': sdk_build}]}
    if filename is None:
        arch = 'Universal' if universal else 'Apple_silicon'
        filename = f'Xcode_{number}_{arch}.xip'
    if has_url:
        entry['links'] = {
            'download': {
                'url': f'https://example.invalid/{filename}'
            }
        }
    entry['checksums'] = {'sha1': sha1}
    return entry


class VersionSortKeyTest(unittest.TestCase):
    """Tests for `_version_sort_key`."""

    def test_parses_a_dotted_version_into_an_int_tuple(self):
        self.assertEqual(m._version_sort_key('26.5'), (26, 5))

    def test_sorts_numerically_even_when_build_strings_sort_backwards(self):
        # As strings, '17F42' > '17F113' (chronologically backwards); the
        # version number itself must sort '26.5' before '26.6'.
        versions = ['26.6', '26.5']
        versions.sort(key=m._version_sort_key)
        self.assertEqual(versions, ['26.5', '26.6'])

    def test_ignores_non_numeric_trailing_components(self):
        # `'0-beta'.isdigit()` is False, so that whole component is dropped
        # rather than partially parsed.
        self.assertEqual(m._version_sort_key('6.0-beta'), (6, ))


class Sha1OfFileTest(unittest.TestCase):
    """Tests for `_sha1_of_file`."""

    def test_computes_the_correct_digest_reading_in_chunks(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            path = Path(tmp_dir) / 'data.bin'
            content = b'some xcode archive bytes' * 100
            path.write_bytes(content)
            self.assertEqual(m._sha1_of_file(path),
                             hashlib.sha1(content).hexdigest())


class MacSdkInfoFromGniTest(unittest.TestCase):
    """Tests for `MacSdkInfo.from_gni`."""

    def test_parses_both_assignments(self):
        text = ('mac_sdk_official_version = "26.5"\n'
                'mac_sdk_official_build_version = "25F70"\n')
        info = m.MacSdkInfo.from_gni(text)
        self.assertEqual(info, m.MacSdkInfo('26.5', '25F70'))

    def test_raises_when_version_assignment_is_missing(self):
        text = 'mac_sdk_official_build_version = "25F70"\n'
        with self.assertRaises(RuntimeError):
            m.MacSdkInfo.from_gni(text)

    def test_raises_when_build_assignment_is_missing(self):
        text = 'mac_sdk_official_version = "26.5"\n'
        with self.assertRaises(RuntimeError):
            m.MacSdkInfo.from_gni(text)


class CheckDeveloperModeTest(unittest.TestCase):
    """Tests for `_check_developer_mode`."""

    def test_passes_silently_when_enabled(self):
        with mock.patch.object(m,
                               '_check_call',
                               return_value=_completed(
                                   'Developer mode is currently enabled.\n')):
            m._check_developer_mode()  # does not raise

    def test_raises_an_actionable_error_when_disabled(self):
        with mock.patch.object(m,
                               '_check_call',
                               return_value=_completed(
                                   'Developer mode is currently disabled.\n')):
            with self.assertRaises(RuntimeError) as ctx:
                m._check_developer_mode()
        self.assertIn('DevToolsSecurity -enable', str(ctx.exception))

    def test_warns_instead_of_raising_when_disabled_and_warn_only(self):
        with mock.patch.object(
                m, '_check_call',
                return_value=_completed(
                    'Developer mode is currently disabled.\n')), \
             mock.patch.object(m.logging, 'warning') as warning:
            m._check_developer_mode(warn_only=True)  # does not raise
        warning.assert_called_once()
        self.assertIn('DevToolsSecurity -enable', warning.call_args.args[0])

    def test_does_not_warn_when_enabled_and_warn_only(self):
        with mock.patch.object(
                m, '_check_call',
                return_value=_completed(
                    'Developer mode is currently enabled.\n')), \
             mock.patch.object(m.logging, 'warning') as warning:
            m._check_developer_mode(warn_only=True)  # does not raise
        warning.assert_not_called()


class MacosMajorVersionTest(unittest.TestCase):
    """Tests for `_macos_major_version`."""

    def test_parses_the_major_component_of_a_dotted_version(self):
        with mock.patch.object(m,
                               '_check_call',
                               return_value=_completed('14.6.1\n')):
            self.assertEqual(m._macos_major_version(), 14)

    def test_parses_a_bare_major_version(self):
        with mock.patch.object(m,
                               '_check_call',
                               return_value=_completed('15\n')):
            self.assertEqual(m._macos_major_version(), 15)


class FetchXcodeReleasesTest(unittest.TestCase):
    """Tests for `_fetch_xcode_releases`."""

    def test_fetches_and_parses_the_catalog(self):
        body = json.dumps([{'name': 'Xcode'}]).encode('utf-8')
        with mock.patch('urllib.request.urlopen',
                        return_value=io.BytesIO(body)) as urlopen:
            result = m._fetch_xcode_releases()
        self.assertEqual(result, [{'name': 'Xcode'}])
        urlopen.assert_called_once_with(m.XCODE_RELEASES_API_URL,
                                        timeout=m.HTTP_FETCH_TIMEOUT_SECS)


class _FakeDownloadResponse:
    """A context-manager stand-in serving *body* in one `read()` call."""

    def __init__(self, body: bytes, content_length: str | None = None):
        self._body = body
        self._served = False
        self.headers = {} if content_length is None else {
            'Content-Length': content_length
        }

    def read(self, _n: int) -> bytes:
        if self._served:
            return b''
        self._served = True
        return self._body

    def __enter__(self):
        return self

    def __exit__(self, *_exc):
        return False


class DownloadToFileTest(unittest.TestCase):
    """Tests for `_download_to_file`."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.dest = Path(self._tmp.name) / 'out.bin'

    def test_streams_the_response_body_to_dest(self):
        with mock.patch('urllib.request.urlopen',
                        return_value=_FakeDownloadResponse(
                            b'xcode-bytes', content_length='11')):
            m._download_to_file('https://example.invalid/x.xip', self.dest)
        self.assertEqual(self.dest.read_bytes(), b'xcode-bytes')

    def test_works_without_a_content_length_header(self):
        with mock.patch('urllib.request.urlopen',
                        return_value=_FakeDownloadResponse(b'data')):
            m._download_to_file('https://example.invalid/x.xip', self.dest)
        self.assertEqual(self.dest.read_bytes(), b'data')


class ResolveReleaseTest(unittest.TestCase):
    """Tests for `EphemeralXcode._resolve_release`.

    Includes regression coverage for two production bugs: a `KeyError` on
    catalog entries with no `sdks` key at all (very old Xcode releases), and
    an unhandled ambiguity when two final releases bundle the identical SDK
    build.
    """

    def _resolve(self,
                 data: list[dict],
                 sdk_version: str = '26.5',
                 sdk_build: str = '25F70') -> m.XcodeRelease:
        xcode = m.EphemeralXcode()
        with mock.patch.object(m, '_fetch_xcode_releases', return_value=data):
            xcode._resolve_release(m.MacSdkInfo(sdk_version, sdk_build))
        return xcode.release

    def test_ignores_non_xcode_named_entries(self):
        data = [
            _xcode_entry('26.5',
                         '17F42',
                         sdk_build='25F70',
                         name='Swift Playgrounds')
        ]
        with self.assertRaises(RuntimeError):
            self._resolve(data)

    def test_skips_beta_entries(self):
        data = [
            _xcode_entry('26.5', '17F5012f', sdk_build='25F70', release='beta')
        ]
        with self.assertRaises(RuntimeError):
            self._resolve(data)

    def test_skips_release_candidate_entries(self):
        data = [_xcode_entry('26.5', '17F42', sdk_build='25F70', release='rc')]
        with self.assertRaises(RuntimeError):
            self._resolve(data)

    def test_skips_entries_missing_the_sdks_key_entirely(self):
        # Regression test: pre-catalog Xcode entries have no `sdks` key at
        # all, which used to raise KeyError instead of being treated as "no
        # match".
        data = [
            _xcode_entry('6.4', '6E23', sdk_build=None),
            _xcode_entry('26.5', '17F42', sdk_build='25F70'),
        ]
        release = self._resolve(data)
        self.assertEqual(release.build, '17F42')

    def test_skips_entries_whose_macos_sdks_lack_the_target_build(self):
        data = [_xcode_entry('26.5', '17F42', sdk_build='25F60')]
        with self.assertRaises(RuntimeError):
            self._resolve(data)

    def test_skips_entries_with_no_download_url(self):
        data = [
            _xcode_entry('26.5', '17F42', sdk_build='25F70', has_url=False),
            _xcode_entry('26.6', '17F113', sdk_build='25F70'),
        ]
        release = self._resolve(data)
        self.assertEqual(release.build, '17F113')

    def test_raises_naming_the_target_build_when_nothing_matches(self):
        data = [_xcode_entry('26.5', '17F42', sdk_build='25F60')]
        with self.assertRaises(RuntimeError) as ctx:
            self._resolve(data)
        self.assertIn('25F70', str(ctx.exception))

    def test_prefers_universal_over_arch_specific_archive(self):
        data = [
            _xcode_entry('26.5', '17F42', sdk_build='25F70', universal=False),
            _xcode_entry('26.5', '17F42', sdk_build='25F70', universal=True),
        ]
        release = self._resolve(data)
        self.assertIn('Universal', release.xip_filename)

    def test_falls_back_to_non_universal_when_none_is_listed(self):
        data = [
            _xcode_entry('26.5', '17F42', sdk_build='25F70', universal=False)
        ]
        release = self._resolve(data)
        self.assertNotIn('Universal', release.xip_filename)

    def test_picks_the_oldest_when_two_final_releases_share_an_sdk_build(self):
        # Regression test: Apple sometimes ships a point release (e.g. 26.6)
        # that bundles the exact same SDK build as its predecessor (26.5)
        # without bumping it; this used to raise "Ambiguous Xcode release".
        data = [
            _xcode_entry('26.6', '17F113', sdk_build='25F70'),
            _xcode_entry('26.5', '17F42', sdk_build='25F70'),
        ]
        release = self._resolve(data)
        self.assertEqual(release.version, '26.5')
        self.assertEqual(release.build, '17F42')

    def test_tie_break_does_not_depend_on_catalog_order(self):
        data = [
            _xcode_entry('26.5', '17F42', sdk_build='25F70'),
            _xcode_entry('26.6', '17F113', sdk_build='25F70'),
        ]
        release = self._resolve(data)
        self.assertEqual(release.version, '26.5')

    def test_stores_all_resolved_release_fields(self):
        data = [
            _xcode_entry('26.5', '17F42', sdk_build='25F70', sha1='cafef00d')
        ]
        release = self._resolve(data)
        self.assertEqual(release.version, '26.5')
        self.assertEqual(release.build, '17F42')
        self.assertEqual(release.download_url,
                         'https://example.invalid/Xcode_26.5_Universal.xip')
        self.assertEqual(release.xip_filename, 'Xcode_26.5_Universal.xip')
        self.assertEqual(release.sha1, 'cafef00d')


class InstallTest(unittest.TestCase):
    """Tests for `EphemeralXcode._install`."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        patcher = mock.patch.object(m, 'XCODE_APPS_DIR', Path(self._tmp.name))
        patcher.start()
        self.addCleanup(patcher.stop)

    def _xcode(self, build: str = '17F42') -> m.EphemeralXcode:
        xcode = m.EphemeralXcode()
        xcode._release = m.XcodeRelease(version='26.5',
                                        build=build,
                                        download_url='u',
                                        xip_filename='f',
                                        sha1='s')
        return xcode

    def test_reuses_an_existing_install_without_downloading(self):
        xcode = self._xcode()
        app_path = Path(self._tmp.name) / 'xcode_17f42.app'
        app_path.mkdir()
        with mock.patch.object(xcode, '_download_and_expand') as download:
            result = xcode._install()
        self.assertEqual(result, app_path)
        download.assert_not_called()

    def test_downloads_and_expands_when_not_present(self):
        xcode = self._xcode()
        app_path = Path(self._tmp.name) / 'xcode_17f42.app'
        with mock.patch.object(xcode, '_download_and_expand') as download:
            result = xcode._install()
        self.assertEqual(result, app_path)
        download.assert_called_once_with(app_path)

    def test_app_name_uses_the_lowercased_build_number(self):
        xcode = self._xcode(build='17F42')
        with mock.patch.object(xcode, '_download_and_expand'):
            result = xcode._install()
        self.assertEqual(result.name, 'xcode_17f42.app')


class DownloadAndExpandTest(unittest.TestCase):
    """Tests for `EphemeralXcode._download_and_expand`."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.app_path = Path(self._tmp.name) / 'xcode_17f42.app'
        self.expand_dir = self.app_path.with_name(
            f'.{self.app_path.stem}.expand')
        self.xcode = m.EphemeralXcode()
        self.xcode._release = m.XcodeRelease(
            version='26.5',
            build='17F42',
            download_url='u',
            xip_filename='Xcode_26.5_Universal.xip',
            sha1='s')

    @staticmethod
    def _check_call_creates_app(*_args,
                                **kwargs) -> subprocess.CompletedProcess:
        """A `_check_call` stand-in mimicking `unxip`/`xip`'s real effect."""
        (Path(kwargs['cwd']) / 'Xcode.app').mkdir(parents=True)
        return _completed()

    def test_uses_unxip_when_available_on_path(self):
        with mock.patch.object(self.xcode, '_download_xip'), \
             mock.patch('shutil.which', return_value='/usr/local/bin/unxip'), \
             mock.patch.object(
                 m, '_check_call',
                 side_effect=self._check_call_creates_app) as check_call:
            self.xcode._download_and_expand(self.app_path)
        self.assertTrue(self.app_path.is_dir())
        self.assertEqual(check_call.call_args.args[0], 'unxip')

    def test_falls_back_to_xip_expand_when_unxip_is_missing(self):
        with mock.patch.object(self.xcode, '_download_xip'), \
             mock.patch('shutil.which', return_value=None), \
             mock.patch.object(
                 m, '_check_call',
                 side_effect=self._check_call_creates_app) as check_call:
            self.xcode._download_and_expand(self.app_path)
        self.assertTrue(self.app_path.is_dir())
        self.assertEqual(check_call.call_args.args[:2], ('xip', '--expand'))

    def test_raises_when_expansion_does_not_produce_an_app_bundle(self):
        with mock.patch.object(self.xcode, '_download_xip'), \
             mock.patch('shutil.which', return_value=None), \
             mock.patch.object(m, '_check_call', return_value=_completed()):
            with self.assertRaises(RuntimeError):
                self.xcode._download_and_expand(self.app_path)
        # The transient expand dir is cleaned up even on failure.
        self.assertFalse(self.expand_dir.exists())
        self.assertFalse(self.app_path.exists())

    def test_removes_a_stale_leftover_expand_dir_before_expanding(self):
        self.expand_dir.mkdir(parents=True)
        (self.expand_dir / 'stale.txt').write_text('leftover')
        with mock.patch.object(self.xcode, '_download_xip'), \
             mock.patch('shutil.which', return_value=None), \
             mock.patch.object(
                 m, '_check_call', side_effect=self._check_call_creates_app):
            self.xcode._download_and_expand(self.app_path)
        self.assertTrue(self.app_path.is_dir())
        self.assertFalse(self.expand_dir.exists())


class DownloadXipTest(unittest.TestCase):
    """Tests for `EphemeralXcode._download_xip`."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.dest = Path(self._tmp.name) / 'Xcode_26.5_Universal.xip'
        self.xcode = m.EphemeralXcode()

    def _set_release(self,
                     filename: str = 'Xcode_26.5_Universal.xip',
                     content: bytes = b'good-bytes') -> bytes:
        self.xcode._release = m.XcodeRelease(
            version='26.5',
            build='17F42',
            download_url='https://apple.example/x',
            xip_filename=filename,
            sha1=hashlib.sha1(content).hexdigest())
        return content

    def test_downloads_and_verifies_on_the_first_url(self):
        content = self._set_release()

        def fake_download(_url, dest):
            dest.write_bytes(content)

        with mock.patch.object(m,
                               '_download_to_file',
                               side_effect=fake_download) as download:
            self.xcode._download_xip(self.dest)
        self.assertEqual(self.dest.read_bytes(), content)
        download.assert_called_once_with(
            m.XCODE_ARCHIVE_BUCKET_URL + 'Xcode_26.5_Universal.xip', self.dest)

    def test_falls_back_to_the_stripped_universal_variant_on_failure(self):
        content = self._set_release()
        attempted = []

        def fake_download(url, dest):
            attempted.append(url)
            if url.endswith('Xcode_26.5_Universal.xip'):
                raise urllib.error.URLError('boom')
            dest.write_bytes(content)

        with mock.patch.object(m,
                               '_download_to_file',
                               side_effect=fake_download):
            self.xcode._download_xip(self.dest)
        self.assertEqual(len(attempted), 2)
        self.assertTrue(attempted[1].endswith('Xcode_26.5.xip'))
        self.assertEqual(self.dest.read_bytes(), content)

    def test_no_stripped_variant_for_a_non_universal_filename(self):
        self._set_release(filename='Xcode_26.5_Apple_silicon.xip')
        with mock.patch.object(
                m, '_download_to_file',
                side_effect=urllib.error.URLError('boom')) as download:
            with self.assertRaises(RuntimeError):
                self.xcode._download_xip(self.dest)
        download.assert_called_once()

    def test_sha1_mismatch_deletes_the_file_and_eventually_raises(self):
        self._set_release()  # expects the SHA-1 of b'good-bytes'

        def fake_download(_url, dest):
            dest.write_bytes(b'wrong-bytes')

        with mock.patch.object(m,
                               '_download_to_file',
                               side_effect=fake_download):
            with self.assertRaises(RuntimeError) as ctx:
                self.xcode._download_xip(self.dest)
        self.assertIsInstance(ctx.exception.__cause__, RuntimeError)
        self.assertIn('SHA-1 mismatch', str(ctx.exception.__cause__))
        self.assertFalse(self.dest.exists())

    def test_raises_after_exhausting_every_url(self):
        self._set_release()
        with mock.patch.object(m,
                               '_download_to_file',
                               side_effect=urllib.error.URLError('boom')):
            with self.assertRaises(RuntimeError) as ctx:
                self.xcode._download_xip(self.dest)
        self.assertIsInstance(ctx.exception.__cause__, urllib.error.URLError)


class SwitchTest(unittest.TestCase):
    """Tests for `EphemeralXcode._switch`.

    Covers the safety checks added on top of the base `xcode-select -s`:
    Developer Mode preflight, the macOS-14+-only Gatekeeper scan, timeout-
    bounded license-accept/`-runFirstLaunch`, and the stale-`ibtoold` kill.
    """

    APP_PATH = Path('/Applications/xcode_17f42.app')

    def _run(self, macos_major: int) -> tuple[mock.Mock, mock.Mock]:
        xcode = m.EphemeralXcode()
        with mock.patch.object(m, '_check_developer_mode') as dev_mode, \
             mock.patch.object(m, '_macos_major_version',
                               return_value=macos_major), \
             mock.patch.object(m, '_check_call',
                               return_value=_completed()) as check_call:
            xcode._switch(self.APP_PATH)
        return dev_mode, check_call

    def test_checks_developer_mode_before_switching(self):
        dev_mode, check_call = self._run(macos_major=14)
        dev_mode.assert_called_once_with(warn_only=False)
        self.assertEqual(
            check_call.call_args_list[0],
            mock.call('sudo', '/usr/bin/xcode-select', '-s',
                      str(self.APP_PATH)))

    def test_skip_developer_mode_check_makes_the_preflight_warn_only(self):
        xcode = m.EphemeralXcode()
        with mock.patch.object(m, '_check_developer_mode') as dev_mode, \
             mock.patch.object(m, '_macos_major_version', return_value=14), \
             mock.patch.object(m, '_check_call', return_value=_completed()):
            xcode._switch(self.APP_PATH, skip_developer_mode_check=True)
        dev_mode.assert_called_once_with(warn_only=True)

    def test_stops_before_any_check_call_when_developer_mode_check_fails(self):
        xcode = m.EphemeralXcode()
        with mock.patch.object(m,
                               '_check_developer_mode',
                               side_effect=RuntimeError('nope')), \
             mock.patch.object(m, '_check_call') as check_call:
            with self.assertRaises(RuntimeError):
                xcode._switch(self.APP_PATH)
        check_call.assert_not_called()

    def test_runs_a_gatekeeper_scan_on_macos_14_and_later(self):
        _, check_call = self._run(macos_major=14)
        check_call.assert_any_call('/usr/bin/gktool',
                                   'scan',
                                   str(self.APP_PATH),
                                   timeout=m.GATEKEEPER_SCAN_TIMEOUT_SECS)

    def test_skips_the_gatekeeper_scan_before_macos_14(self):
        _, check_call = self._run(macos_major=13)
        for call in check_call.call_args_list:
            self.assertNotIn('/usr/bin/gktool', call.args)

    def test_accepts_the_license_and_runs_first_launch_with_timeouts(self):
        _, check_call = self._run(macos_major=14)
        check_call.assert_any_call('sudo',
                                   '/usr/bin/xcodebuild',
                                   '-license',
                                   'accept',
                                   timeout=m.LICENSE_ACCEPT_TIMEOUT_SECS)
        check_call.assert_any_call('sudo',
                                   '/usr/bin/xcodebuild',
                                   '-runFirstLaunch',
                                   timeout=m.RUN_FIRST_LAUNCH_TIMEOUT_SECS)

    def test_kills_stale_ibtoold_tolerating_a_missing_process(self):
        _, check_call = self._run(macos_major=14)
        check_call.assert_any_call('pkill', '-f', '/ibtoold($| )', check=False)

    def test_reloads_simctl_last(self):
        _, check_call = self._run(macos_major=14)
        self.assertEqual(check_call.call_args_list[-1],
                         mock.call('xcrun', 'simctl', 'list'))


class SelectContextManagerTest(unittest.TestCase):
    """Tests for `EphemeralXcode._select`."""

    def test_switches_then_resets_on_normal_exit(self):
        xcode = m.EphemeralXcode()
        calls = []
        with mock.patch.object(
                xcode, '_switch',
                side_effect=lambda _p, **_kwargs: calls.append('switch')), \
             mock.patch.object(
                 xcode, 'reset', side_effect=lambda: calls.append('reset')):
            with xcode._select(Path('/Applications/xcode.app')):
                calls.append('body')
        self.assertEqual(calls, ['switch', 'body', 'reset'])

    def test_resets_even_when_the_body_raises(self):
        xcode = m.EphemeralXcode()
        with mock.patch.object(xcode, '_switch'), \
             mock.patch.object(xcode, 'reset') as reset:
            with self.assertRaises(ValueError):
                with xcode._select(Path('/Applications/xcode.app')):
                    raise ValueError('boom')
        reset.assert_called_once_with()


class LocateAppTest(unittest.TestCase):
    """Tests for `EphemeralXcode._locate_app`."""

    def test_derives_the_app_bundle_from_the_developer_dir(self):
        xcode = m.EphemeralXcode()
        with mock.patch.object(
                m,
                '_check_call',
                return_value=_completed(
                    '/Applications/Xcode.app/Contents/Developer\n')):
            xcode._locate_app()
        self.assertEqual(xcode.app, Path('/Applications/Xcode.app'))

    def test_raises_when_the_developer_dir_is_not_inside_an_app_bundle(self):
        xcode = m.EphemeralXcode()
        with mock.patch.object(m,
                               '_check_call',
                               return_value=_completed(
                                   '/Library/Developer/CommandLineTools\n')):
            with self.assertRaises(RuntimeError):
                xcode._locate_app()


def _fake_verify_check_call(*args, **_kwargs) -> subprocess.CompletedProcess:
    """Serve `_verify_versions`'s two distinct `xcodebuild -version[...]`
    invocations their matching canned output."""
    if 'macosx' in args:
        stdout = 'SDKVersion: 26.5\nProductBuildVersion: 25F70\n'
    else:
        stdout = 'Xcode 26.5\nBuild version 17F42\n'
    return _completed(stdout)


class VerifyVersionsTest(unittest.TestCase):
    """Tests for `EphemeralXcode._verify_versions`."""

    def _xcode_with_release(self, build: str = '17F42') -> m.EphemeralXcode:
        xcode = m.EphemeralXcode()
        xcode._release = m.XcodeRelease(version='26.5',
                                        build=build,
                                        download_url='u',
                                        xip_filename='f',
                                        sha1='s')
        return xcode

    def test_passes_when_the_active_xcode_and_sdk_match(self):
        xcode = self._xcode_with_release()
        with mock.patch.object(m,
                               '_check_call',
                               side_effect=_fake_verify_check_call):
            xcode._verify_versions(m.MacSdkInfo('26.5', '25F70'))  # no raise

    def test_raises_when_the_active_xcode_build_does_not_match_resolved(self):
        xcode = self._xcode_with_release(build='DIFFERENTBUILD')
        with mock.patch.object(m,
                               '_check_call',
                               side_effect=_fake_verify_check_call):
            with self.assertRaises(RuntimeError):
                xcode._verify_versions(m.MacSdkInfo('26.5', '25F70'))

    def test_raises_when_the_active_sdk_does_not_match_the_pin(self):
        xcode = self._xcode_with_release()
        with mock.patch.object(m,
                               '_check_call',
                               side_effect=_fake_verify_check_call):
            with self.assertRaises(RuntimeError):
                xcode._verify_versions(m.MacSdkInfo('26.5', 'WRONGBUILD'))

    def test_raises_when_xcodebuild_output_is_missing_expected_fields(self):
        xcode = self._xcode_with_release()
        with mock.patch.object(m,
                               '_check_call',
                               return_value=_completed('garbage')):
            with self.assertRaises(RuntimeError):
                xcode._verify_versions(m.MacSdkInfo('26.5', '25F70'))


class InstallAndSelectTest(unittest.TestCase):
    """Tests for `EphemeralXcode.install_and_select`."""

    def test_runs_resolve_install_switch_locate_verify_in_order(self):
        xcode = m.EphemeralXcode()
        app_path = Path('/Applications/xcode_17f42.app')
        calls = []
        with mock.patch.object(
                xcode, '_resolve_release',
                side_effect=lambda _info: calls.append('resolve')), \
             mock.patch.object(
                 xcode, '_install',
                 side_effect=lambda: calls.append('install') or app_path), \
             mock.patch.object(
                 xcode, '_switch',
                 side_effect=lambda _p, **_kwargs: calls.append('switch')), \
             mock.patch.object(
                 xcode, '_locate_app',
                 side_effect=lambda: calls.append('locate')), \
             mock.patch.object(
                 xcode, '_verify_versions',
                 side_effect=lambda _info: calls.append('verify')):
            result = xcode.install_and_select(m.MacSdkInfo('26.5', '25F70'))
        self.assertEqual(calls,
                         ['resolve', 'install', 'switch', 'locate', 'verify'])
        self.assertEqual(result, app_path)


class DeployTest(unittest.TestCase):
    """Tests for `EphemeralXcode.deploy`."""

    def _mocks(self, app_path: Path, calls: list[str]):
        # `mock.patch.object` on a class attribute replaces it with a plain
        # `Mock`, which is not a descriptor: accessing it via an instance
        # does *not* bind `self`, so these side effects take only the
        # explicit call arguments.
        return (
            mock.patch.object(
                m.EphemeralXcode,
                '_resolve_release',
                side_effect=lambda _info: calls.append('resolve')),
            mock.patch.object(m.EphemeralXcode,
                              '_install',
                              return_value=app_path),
            mock.patch.object(
                m.EphemeralXcode,
                '_switch',
                side_effect=lambda _p, **_kwargs: calls.append('switch')),
            mock.patch.object(m.EphemeralXcode,
                              'reset',
                              side_effect=lambda: calls.append('reset')),
            mock.patch.object(m.EphemeralXcode,
                              '_locate_app',
                              side_effect=lambda: calls.append('locate')),
            mock.patch.object(
                m.EphemeralXcode,
                '_verify_versions',
                side_effect=lambda _info: calls.append('verify')),
        )

    def test_yields_self_populated_and_resets_on_exit(self):
        xcode = m.EphemeralXcode()
        app_path = Path('/Applications/xcode_17f42.app')
        calls: list[str] = []
        with contextlib.ExitStack() as stack:
            for patcher in self._mocks(app_path, calls):
                stack.enter_context(patcher)
            with xcode.deploy(m.MacSdkInfo('26.5', '25F70')) as yielded:
                calls.append('body')
                self.assertIs(yielded, xcode)
        self.assertEqual(
            calls, ['resolve', 'switch', 'locate', 'verify', 'body', 'reset'])

    def test_resets_when_the_body_raises(self):
        xcode = m.EphemeralXcode()
        app_path = Path('/Applications/xcode_17f42.app')
        calls: list[str] = []
        with contextlib.ExitStack() as stack:
            for patcher in self._mocks(app_path, calls):
                stack.enter_context(patcher)
            with self.assertRaises(ValueError):
                with xcode.deploy(m.MacSdkInfo('26.5', '25F70')):
                    raise ValueError('boom')
        self.assertIn('reset', calls)


class ResetTest(unittest.TestCase):
    """Tests for `EphemeralXcode.reset`."""

    def test_reverts_xcode_select(self):
        with mock.patch.object(m, '_check_call') as check_call:
            m.EphemeralXcode().reset()
        check_call.assert_called_once_with('sudo', '/usr/bin/xcode-select',
                                           '--reset')


class PropertiesTest(unittest.TestCase):
    """Tests for the `app`/`release` properties before they're populated."""

    def test_app_raises_before_locate_app_has_run(self):
        with self.assertRaises(RuntimeError):
            _ = m.EphemeralXcode().app

    def test_release_raises_before_resolve_release_has_run(self):
        with self.assertRaises(RuntimeError):
            _ = m.EphemeralXcode().release


class MainTest(unittest.TestCase):
    """Tests for the `main` CLI entry point."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.json_path = Path(self._tmp.name) / 'result.json'

    def _fake_xcode(self, app_path: Path) -> mock.Mock:
        fake = mock.Mock()
        fake.install_and_select.return_value = app_path
        fake.release = m.XcodeRelease(version='26.5',
                                      build='17F42',
                                      download_url='u',
                                      xip_filename='f',
                                      sha1='s')
        return fake

    def test_writes_the_resolved_xcode_info_as_json(self):
        app_path = Path('/Applications/xcode_17f42.app')
        fake_xcode = self._fake_xcode(app_path)
        with mock.patch.object(m, 'EphemeralXcode', return_value=fake_xcode):
            returncode = m.main([
                '--sdk-version', '26.5', '--sdk-build', '25F70',
                '--json-output',
                str(self.json_path)
            ])
        self.assertEqual(returncode, 0)
        fake_xcode.install_and_select.assert_called_once_with(
            m.MacSdkInfo('26.5', '25F70'), skip_developer_mode_check=False)
        self.assertEqual(
            json.loads(self.json_path.read_text()), {
                'app': str(app_path),
                'xcode_version': '26.5',
                'xcode_build': '17F42',
                'sdk_version': '26.5',
                'sdk_build_version': '25F70',
            })

    def test_no_developer_mode_check_flag_is_threaded_through(self):
        app_path = Path('/Applications/xcode_17f42.app')
        fake_xcode = self._fake_xcode(app_path)
        with mock.patch.object(m, 'EphemeralXcode', return_value=fake_xcode):
            m.main([
                '--sdk-version', '26.5', '--sdk-build', '25F70',
                '--json-output',
                str(self.json_path), '--no-developer-mode-check'
            ])
        fake_xcode.install_and_select.assert_called_once_with(
            m.MacSdkInfo('26.5', '25F70'), skip_developer_mode_check=True)

    def test_missing_required_arguments_exits_nonzero(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                m.main(['--sdk-version', '26.5'])

    def test_verbose_flag_enables_debug_logging(self):
        fake_xcode = self._fake_xcode(Path('/Applications/xcode.app'))
        with mock.patch.object(m, 'EphemeralXcode', return_value=fake_xcode), \
             mock.patch.object(m.logging, 'basicConfig') as basic_config:
            m.main([
                '--sdk-version', '26.5', '--sdk-build', '25F70',
                '--json-output',
                str(self.json_path), '--verbose'
            ])
        basic_config.assert_called_once_with(level=m.logging.DEBUG,
                                             format='%(message)s')


if __name__ == '__main__':
    unittest.main()
