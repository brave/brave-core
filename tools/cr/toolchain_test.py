#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the `toolchain` module.

Layers:

* Pure helpers -- `_render_py_literal`, `RustToolchain._commit_title`,
  `RustToolchain._load_extra_deps`, `XcodeToolchain._provenance_comment`, and
  `get_assigned_value`.

* Detection / attribution -- `Toolchain.was_updated`, the unified
  `Toolchain.find_culprit`, `RustToolchain.is_published`, and `Toolchain.check`,
  driven against a `FakeChromiumRepo`.

* Repin -- `RustToolchain.repin` and `XcodeToolchain.repin` end-to-end against a
  `FakeChromiumRepo` with the commit-msg hook installed (builders faked, so no
  network), validating the file rewrite *and* the `tags=toolchain` /
  `culprit=<hash>` commit.

* CI delegation -- `Toolchain.trigger` fans out to `ci.JenkinsCi` with the
  toolchain's frozen job URLs.
"""

from __future__ import annotations

import ast
import shutil
import stat
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

import toolchain
from test.fake_chromium_repo import FakeChromiumRepo
from versioning import Version

# Source of the commit-msg hook, copied into the fake brave repo so the
# `tags` / `culprit` env wiring is exercised exactly as in production.
HOOK_SOURCE: Path = (Path(__file__).resolve().parent / 'alias' /
                     'commit-msg.py')

# The Chromium tag the repin/detection tests resolve against.
CHROMIUM_TAG = '150.0.7850.1'

# ---------------------------------------------------------------------------
# Pure helpers.
# ---------------------------------------------------------------------------

NEW_ENTRY = {
    'src/third_party/rust-toolchain': {
        'bucket': 'https://example.invalid/rust-toolchain-aux/',
        'condition': 'not rust_force_head_revision',
        'objects': [
            {
                'object_name': 'linux-x64-rust-toolchain-2.tar.xz',
                'sha256sum': 'newlinuxsha',
                'condition': 'host_os == "linux"',
            },
            {
                'object_name': 'win-rust-toolchain-2.tar.xz',
                'sha256sum': 'newwinsha',
                'condition': 'host_os == "win"',
            },
        ],
    },
}

SAMPLE_INSTALLER = """\
# Installer module.

EXTRA_DEPS = {
    'src/other-dep': {
        'bucket': 'https://example.invalid/other/',
        'objects': [
            {
                'object_name': 'other.tar.xz',
                'sha256sum': 'othersha',
                'condition': 'host_os == "linux"',
            },
        ],
    },
    'src/third_party/rust-toolchain': {
        'bucket': 'https://example.invalid/rust-toolchain-aux/',
        'condition': 'not rust_force_head_revision',
        'objects': [
            {
                'object_name': 'old-linux-1.tar.xz',
                'sha256sum': 'oldlinuxsha',
                'condition': 'host_os == "linux"',
            },
        ],
    },
}


def untouched():
    return 1
"""


class RenderLiteralTest(unittest.TestCase):
    """Tests for `toolchain._render_py_literal`."""

    def test_round_trips(self):
        self.assertEqual(
            ast.literal_eval(toolchain._render_py_literal(NEW_ENTRY)),
            NEW_ENTRY)

    def test_single_quoted_with_embedded_double_quotes(self):
        rendered = toolchain._render_py_literal(
            {'condition': 'host_os == "mac" and host_cpu == "arm64"'})
        self.assertIn(
            "    'condition': 'host_os == \"mac\" and host_cpu == \"arm64\"',",
            rendered)

    def test_empty_containers(self):
        self.assertEqual(toolchain._render_py_literal({}), '{}')
        self.assertEqual(toolchain._render_py_literal([]), '[]')


class GetAssignedValueTest(unittest.TestCase):
    """Tests for `toolchain.get_assigned_value`."""

    def test_reads_quoted_value(self):
        self.assertEqual(
            toolchain.get_assigned_value("RUST_REVISION = 'abc'",
                                         'RUST_REVISION'), 'abc')

    def test_reads_indented_gn_value(self):
        # mac_sdk.gni nests its assignments; the reader tolerates indentation.
        self.assertEqual(
            toolchain.get_assigned_value('  mac_sdk_official_version = "26.5"',
                                         'mac_sdk_official_version'), '26.5')

    def test_added_and_removed_filters(self):
        diff = "-SDK_VERSION = '1'\n+SDK_VERSION = '2'\n"
        self.assertEqual(
            toolchain.get_assigned_value(diff, 'SDK_VERSION', added=True), '2')
        self.assertEqual(
            toolchain.get_assigned_value(diff, 'SDK_VERSION', removed=True),
            '1')


class CommitTitleTest(unittest.TestCase):
    """Tests for `RustToolchain._commit_title`."""

    @staticmethod
    def _entry(object_name: str) -> dict:
        return {
            'src/third_party/rust-toolchain': {
                'objects': [{
                    'object_name': object_name
                }],
            }
        }

    def test_title_uses_upstream_rust_sub_revision(self):
        name = ('linux-x64-rust-toolchain-'
                '4c4205163abcbd08948b3efab796c543ba1ea687-2-'
                'llvmorg-23-init-10931-g20b6ec66-1.tar.xz')
        title = toolchain.RustToolchain._commit_title(self._entry(name))
        self.assertEqual(
            title, 'Rust/WASM toolchain (4c4205163abc-2, '
            'llvmorg-23-init-10931-g20b6ec66, sub 2)')

    def test_unparseable_name_falls_back_to_stem(self):
        title = toolchain.RustToolchain._commit_title(
            self._entry('linux-x64-rust-toolchain-2.tar.xz'))
        self.assertEqual(title,
                         'Rust/WASM toolchain linux-x64-rust-toolchain-2')


class LoadExtraDepsTest(unittest.TestCase):
    """Tests for `RustToolchain._load_extra_deps`."""

    def test_returns_node_and_value(self):
        node, value = toolchain.RustToolchain._load_extra_deps(
            SAMPLE_INSTALLER)
        self.assertEqual(node.targets[0].id, 'EXTRA_DEPS')
        self.assertIn('src/third_party/rust-toolchain', value)
        self.assertIn('src/other-dep', value)

    def test_missing_assignment_raises(self):
        with self.assertRaises(toolchain.InvalidInputException):
            toolchain.RustToolchain._load_extra_deps('X = 1\n')


# Provenance-comment fixtures (shared with the xcode repin test).
FAKE_INDEX = {
    'url': ('https://example.invalid/xcode-hermetic-toolchain/'
            'xcode-hermetic-toolchain-26.5-25F70.tar.gz'),
    'sha256sum': 'b' * 64,
    'xcode_version': '26.5',
    'xcode_build': '17F42',
    'metal_build': '17E188',
    'chromium_tag': CHROMIUM_TAG,
}


class ProvenanceCommentTest(unittest.TestCase):
    """Tests for `XcodeToolchain._provenance_comment`."""

    SDK_INFO = toolchain.build_xcode_toolchain.MacSdkInfo(
        sdk_version='26.5', product_build_version='25F70')

    def test_records_xcode_sdk_and_metal(self):
        comment = toolchain.XcodeToolchain._provenance_comment(
            self.SDK_INFO, FAKE_INDEX)
        lines = comment.splitlines()
        self.assertTrue(all(line.startswith('# ') for line in lines))
        self.assertTrue(all(len(line) <= 79 for line in lines))
        prose = ' '.join(line[2:] for line in lines)
        self.assertEqual(
            prose, 'This contains binaries from Xcode 26.5 (17F42) along with '
            'the macOS 26.5 SDK (25F70) and the Metal toolchain (17E188).')

    def test_missing_metal_build_falls_back_to_unknown(self):
        comment = toolchain.XcodeToolchain._provenance_comment(
            self.SDK_INFO, {
                **FAKE_INDEX, 'metal_build': None
            })
        self.assertIn('Metal toolchain (unknown)',
                      comment.replace('\n# ', ' '))


# ---------------------------------------------------------------------------
# CI delegation.
# ---------------------------------------------------------------------------


class TriggerTest(unittest.TestCase):
    """`Toolchain.trigger` fans out to `ci.JenkinsCi` with the frozen URLs."""

    def _trigger(self, target, **properties):
        launcher = MagicMock()
        with patch('toolchain.JenkinsCi.from_config', return_value=launcher):
            target.trigger(Version(CHROMIUM_TAG), watch=True, **properties)
        return launcher

    def test_rust_codifies_properties_and_no_build_param(self):
        spec = toolchain.RustToolchain().spec
        self.assertIsNone(spec.build_param)
        self.assertEqual(spec.properties,
                         ('brave_subrevision', 'chromium_ref'))

    def test_rust_triggers_four_jobs_with_properties_payload(self):
        rust = toolchain.RustToolchain()
        launcher = self._trigger(rust, brave_subrevision=7)
        launcher.trigger.assert_called_once()
        args, kwargs = launcher.trigger.call_args
        self.assertEqual(args[0], rust.spec.job_urls)
        self.assertEqual(len(args[0]), 4)
        # Rust has no build parameter; the tag rides in the PROPERTIES payload,
        # with `chromium_ref` filled from the triggered version.
        self.assertEqual(kwargs['params'], {})
        self.assertEqual(kwargs['properties'], {
            'brave_subrevision': 7,
            'chromium_ref': CHROMIUM_TAG,
        })

    def test_rust_requires_its_properties(self):
        launcher = MagicMock()
        with patch('toolchain.JenkinsCi.from_config', return_value=launcher):
            # `brave_subrevision` is not auto-derivable, so it must be supplied.
            with self.assertRaises(toolchain.InvalidInputException):
                toolchain.RustToolchain().trigger(Version(CHROMIUM_TAG))
        launcher.trigger.assert_not_called()

    def test_xcode_triggers_single_job_with_build_param(self):
        launcher = self._trigger(toolchain.XcodeToolchain())
        args, kwargs = launcher.trigger.call_args
        self.assertEqual(len(args[0]), 1)
        self.assertEqual(kwargs['params'], {'CHROMIUM_TAG': CHROMIUM_TAG})
        self.assertIsNone(kwargs['properties'])

    def test_build_param_toolchain_rejects_properties(self):
        launcher = MagicMock()
        with patch('toolchain.JenkinsCi.from_config', return_value=launcher):
            with self.assertRaises(toolchain.InvalidInputException):
                toolchain.XcodeToolchain().trigger(Version(CHROMIUM_TAG),
                                                   brave_subrevision=1)
        launcher.trigger.assert_not_called()

    def test_windows_triggers_single_job(self):
        launcher = self._trigger(toolchain.WindowsToolchain())
        self.assertEqual(len(launcher.trigger.call_args.args[0]), 1)


# ---------------------------------------------------------------------------
# Detection, attribution, and repin against a fake Chromium repo.
# ---------------------------------------------------------------------------


class _FakeRepoTest(unittest.TestCase):
    """Base fixture: a `FakeChromiumRepo` on a `cr150` branch with the hook."""

    def setUp(self):
        self.repo = FakeChromiumRepo()
        self.repo.setup()
        self.addCleanup(self.repo.cleanup)
        self.repo._run_git_command(['checkout', '-b', 'cr150'],
                                   self.repo.brave)
        self._install_commit_msg_hook()

    def _install_commit_msg_hook(self) -> None:
        hooks_dir = self.repo.brave / '.git' / 'hooks'
        hooks_dir.mkdir(exist_ok=True)
        dest = hooks_dir / 'commit-msg'
        if dest.exists() or dest.is_symlink():
            dest.unlink()
        shutil.copy2(HOOK_SOURCE, dest)
        dest.chmod(dest.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP
                   | stat.S_IXOTH)

    def _brave_head(self) -> str:
        return self.repo._run_git_command(['rev-parse', 'HEAD'],
                                          self.repo.brave)

    def _last_brave_message(self) -> str:
        return self.repo._run_git_command(['log', '-1', '--format=%B'],
                                          self.repo.brave)


class RustRepinTest(_FakeRepoTest):
    """End-to-end `RustToolchain.repin` against a `FakeChromiumRepo`."""

    INSTALLER = 'tools/cr/install_extra_deps.py'

    def setUp(self):
        super().setUp()
        self.rust = toolchain.RustToolchain()
        self._seed_installer()
        patcher = patch.object(toolchain.build_rust_toolchain,
                               'rust_toolchain_extra_dep',
                               return_value=NEW_ENTRY)
        self.builder = patcher.start()
        self.addCleanup(patcher.stop)

    def _seed_installer(self, content: str = SAMPLE_INSTALLER) -> None:
        path = self.repo.brave / self.INSTALLER
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8', newline='')
        self.repo._run_git_command(['add', str(path)], self.repo.brave)
        self.repo._run_git_command(
            ['commit', '--no-verify', '-m', 'Seed installer'], self.repo.brave)

    def _seed_rust_revision_bump(self) -> str:
        """Seed a Chromium commit introducing the Rust/Clang revision constants
        and tag it as `CHROMIUM_TAG`, returning its hash (what the unified
        `find_culprit` pickaxe should attribute the update to)."""
        self.repo.write_and_stage_file(
            'tools/rust/update_rust.py',
            "RUST_REVISION = 'abc123def'\nRUST_SUB_REVISION = 2\n",
            self.repo.chromium)
        self.repo.write_and_stage_file(
            'tools/clang/scripts/update.py',
            "CLANG_REVISION = 'llvmorg-23-init-10931-g20b6ec66'\n",
            self.repo.chromium)
        culprit = self.repo.commit('Roll clang+rust revision',
                                   self.repo.chromium)
        self.repo._run_git_command(['tag', CHROMIUM_TAG], self.repo.chromium)
        return culprit

    def _installer_text(self) -> str:
        return (self.repo.brave / self.INSTALLER).read_bytes().decode('utf-8')

    def test_updates_and_commits_with_auto_culprit(self):
        culprit = self._seed_rust_revision_bump()

        self.rust.repin(Version(CHROMIUM_TAG), culprit=None)

        text = self._installer_text()
        self.assertIn('linux-x64-rust-toolchain-2.tar.xz', text)
        self.assertIn('win-rust-toolchain-2.tar.xz', text)
        self.assertNotIn('old-linux-1.tar.xz', text)
        self.assertIn("'src/other-dep'", text)
        self.assertIn('def untouched():', text)

        message = self._last_brave_message()
        subject = message.splitlines()[0]
        self.assertIn('[cr150]', subject)
        self.assertIn('[toolchain]', subject)
        self.assertIn('Rust/WASM toolchain', subject)
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{culprit}',
            message)
        self.assertIn('Roll clang+rust revision', message)

    def test_explicit_culprit_overrides_autodetect(self):
        autodetect = self._seed_rust_revision_bump()
        self.repo.write_and_stage_file('docs/unrelated.txt', 'noise\n',
                                       self.repo.chromium)
        culprit = self.repo.commit('Unrelated chromium change',
                                   self.repo.chromium)

        self.rust.repin(Version(CHROMIUM_TAG), culprit=culprit)

        message = self._last_brave_message()
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{culprit}',
            message)
        self.assertIn('Unrelated chromium change', message)
        self.assertNotIn(autodetect, message)

    def test_idempotent_second_run_does_not_commit(self):
        self._seed_rust_revision_bump()

        self.rust.repin(Version(CHROMIUM_TAG), culprit=None)
        head_after_first = self._brave_head()

        self.rust.repin(Version(CHROMIUM_TAG), culprit=None)
        self.assertEqual(self._brave_head(), head_after_first)

    def test_staged_files_block_the_update(self):
        self.repo.write_and_stage_file('staged.txt', 'wip\n', self.repo.brave)
        with self.assertRaises(toolchain.InvalidInputException):
            self.rust.repin(Version(CHROMIUM_TAG), culprit='deadbeef')


# Fixtures for the xcode repin test.
HERMETIC_XCODE_SCRIPT_INITIAL = """\
# Sample header.

MAC_BINARIES_HASH = 'oldhash'

# This contains binaries from Xcode 26.4 (17E202) along with the macOS 26.4 SDK
# (25E251) and the Metal toolchain (17E150).
MAC_SDK_OFFICIAL_VERSION = '26.4'
MAC_SDK_OFFICIAL_BUILD_VERSION = '25E251'
XCODE_TOOLCHAIN_DOWNLOAD_URL = (
    'https://example.invalid/xcode-hermetic-toolchain/xcode-hermetic-toolchain-'
    f'{MAC_SDK_OFFICIAL_VERSION}-{MAC_SDK_OFFICIAL_BUILD_VERSION}.tar.gz')

# The toolchain will not be downloaded if the minimum OS version is not met. 19
# is the Darwin major version number for macOS 10.15. Xcode 26.4 17E202 only
# runs on macOS 26.1 and newer, but some bots are still running older OS
# versions. macOS 10.15.4, the OS minimum through Xcode 12.4, still seems to
# work.
MAC_MINIMUM_OS_VERSION = [19, 4]
"""

MAC_SDK_GNI_INITIAL = """\
  mac_sdk_official_version = "26.4"
  mac_sdk_official_build_version = "25E236"
"""

MAC_SDK_GNI_BUMPED = """\
  mac_sdk_official_version = "26.5"
  mac_sdk_official_build_version = "25F70"
"""

MAC_TOOLCHAIN_PY = """\
# Upstream header.
MAC_BINARIES_TAG = 'sometag'

# The toolchain will not be downloaded if the minimum OS version is not met. 19
# is the Darwin major version number for macOS 10.15. Xcode 26.5 17F42 only
# runs on macOS 26.3 and newer, but some bots are still running older OS
# versions. macOS 10.15.4, the OS minimum through Xcode 12.4, still seems to
# work.
MAC_MINIMUM_OS_VERSION = [20, 0]

OTHER_CONSTANT = 1
"""


class XcodeRepinTest(_FakeRepoTest):
    """End-to-end `XcodeToolchain.repin` against a `FakeChromiumRepo`."""

    def setUp(self):
        super().setUp()
        self.xcode = toolchain.XcodeToolchain()
        self._seed_hermetic_xcode_script()
        patcher = patch.object(toolchain.build_xcode_toolchain,
                               'fetch_published_index',
                               return_value=FAKE_INDEX)
        self.fetch_index = patcher.start()
        self.addCleanup(patcher.stop)

    def _seed_hermetic_xcode_script(
            self, content: str = HERMETIC_XCODE_SCRIPT_INITIAL) -> None:
        script_path = self.repo.brave / self.xcode.spec.script
        script_path.parent.mkdir(parents=True, exist_ok=True)
        script_path.write_text(content, encoding='utf-8', newline='')
        self.repo._run_git_command(['add', str(script_path)], self.repo.brave)
        self.repo._run_git_command(
            ['commit', '--no-verify', '-m', 'Seed hermetic xcode script'],
            self.repo.brave)

    def _seed_mac_sdk_bump(self,
                           mac_toolchain_py: str = MAC_TOOLCHAIN_PY) -> str:
        gni = self.repo.chromium / 'build' / 'config' / 'mac' / 'mac_sdk.gni'
        gni.parent.mkdir(parents=True, exist_ok=True)
        gni.write_text(MAC_SDK_GNI_INITIAL, encoding='utf-8', newline='')
        self.repo._run_git_command(['add', str(gni)], self.repo.chromium)
        self.repo._run_git_command(['commit', '-m', 'mac: initial SDK pin'],
                                   self.repo.chromium)

        gni.write_text(MAC_SDK_GNI_BUMPED, encoding='utf-8', newline='')
        tc = self.repo.chromium / 'build' / 'mac_toolchain.py'
        tc.write_text(mac_toolchain_py, encoding='utf-8', newline='')
        self.repo._run_git_command(['add', str(gni), str(tc)],
                                   self.repo.chromium)
        culprit = self.repo.commit('mac: Switch to SDK 26.5',
                                   self.repo.chromium)
        self.repo._run_git_command(['tag', CHROMIUM_TAG], self.repo.chromium)
        return culprit

    def _read_hermetic_xcode_script(self) -> str:
        return (self.repo.brave /
                self.xcode.spec.script).read_text(encoding='utf-8')

    def test_constants_rewritten_and_committed(self):
        culprit = self._seed_mac_sdk_bump()

        self.xcode.repin(Version(CHROMIUM_TAG), culprit=None)

        script = self._read_hermetic_xcode_script()
        self.assertIn(f"MAC_BINARIES_HASH = '{FAKE_INDEX['sha256sum']}'",
                      script)
        self.assertIn("MAC_SDK_OFFICIAL_VERSION = '26.5'", script)
        self.assertIn("MAC_SDK_OFFICIAL_BUILD_VERSION = '25F70'", script)
        self.assertNotIn("'oldhash'", script)
        sdk_info = toolchain.build_xcode_toolchain.MacSdkInfo(
            sdk_version='26.5', product_build_version='25F70')
        self.assertIn(
            toolchain.XcodeToolchain._provenance_comment(sdk_info, FAKE_INDEX),
            script)
        self.assertIn('MAC_MINIMUM_OS_VERSION = [20, 0]', script)
        self.assertIn('runs on macOS 26.3 and newer', script)
        self.assertNotIn('MAC_MINIMUM_OS_VERSION = [19, 4]', script)
        self.assertNotIn('OTHER_CONSTANT', script)
        self.assertNotIn('MAC_BINARIES_TAG', script)

        self.assertEqual(self.fetch_index.call_args.args[0], sdk_info)

        message = self._last_brave_message()
        subject = message.splitlines()[0]
        self.assertIn('[cr150]', subject)
        self.assertIn('[toolchain]', subject)
        self.assertIn('Switch to Xcode 26.5 17F42', subject)
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{culprit}',
            message)
        self.assertIn('mac: Switch to SDK 26.5', message)

    def test_culprit_override_skips_auto_detection(self):
        autodetect = self._seed_mac_sdk_bump()
        self.repo.write_and_stage_file('docs/unrelated.txt', 'noise\n',
                                       self.repo.chromium)
        override = self.repo.commit('Unrelated chromium change',
                                    self.repo.chromium)

        self.xcode.repin(Version(CHROMIUM_TAG), culprit=override)

        message = self._last_brave_message()
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{override}',
            message)
        self.assertIn('Unrelated chromium change', message)
        self.assertNotIn(autodetect, message)

    def test_already_pinned_second_run_raises(self):
        self._seed_mac_sdk_bump()

        self.xcode.repin(Version(CHROMIUM_TAG), culprit=None)
        head_after_first = self._brave_head()

        with self.assertRaises(toolchain.InvalidInputException):
            self.xcode.repin(Version(CHROMIUM_TAG), culprit=None)
        self.assertEqual(self._brave_head(), head_after_first)

    def test_index_fetch_failure_raises(self):
        self._seed_mac_sdk_bump()
        self.fetch_index.side_effect = RuntimeError('index not found')
        head_before = self._brave_head()

        with self.assertRaises(toolchain.BadOutcomeException):
            self.xcode.repin(Version(CHROMIUM_TAG), culprit=None)
        self.assertEqual(self._brave_head(), head_before)

    def test_missing_upstream_min_os_block_raises(self):
        self._seed_mac_sdk_bump(
            mac_toolchain_py='# upstream with no min-os block\nFOO = 1\n')
        head_before = self._brave_head()

        with self.assertRaises(toolchain.InvalidInputException):
            self.xcode.repin(Version(CHROMIUM_TAG), culprit=None)
        self.assertEqual(self._brave_head(), head_before)

    def test_staged_files_block_the_update(self):
        self.repo.write_and_stage_file('staged.txt', 'wip\n', self.repo.brave)
        with self.assertRaises(toolchain.InvalidInputException):
            self.xcode.repin(Version(CHROMIUM_TAG), culprit='deadbeef')


class DetectionTest(_FakeRepoTest):
    """`was_updated`, `find_culprit`, and `check` against a fake repo."""

    def setUp(self):
        super().setUp()
        self.rust = toolchain.RustToolchain()

    def _seed_rust_range(self) -> tuple[str, str]:
        """Two Chromium commits: the base pin and a revision bump tagged as
        `CHROMIUM_TAG`. Returns `(base_ref, bump_culprit)`."""
        self.repo.write_and_stage_file(
            'tools/rust/update_rust.py',
            "RUST_REVISION = 'old'\nRUST_SUB_REVISION = 1\n",
            self.repo.chromium)
        self.repo.write_and_stage_file('tools/clang/scripts/update.py',
                                       "CLANG_REVISION = 'clang-old'\n",
                                       self.repo.chromium)
        base = self.repo.commit('Base rust pin', self.repo.chromium)

        self.repo.write_and_stage_file(
            'tools/rust/update_rust.py',
            "RUST_REVISION = 'new'\nRUST_SUB_REVISION = 2\n",
            self.repo.chromium)
        self.repo.write_and_stage_file('tools/clang/scripts/update.py',
                                       "CLANG_REVISION = 'clang-new'\n",
                                       self.repo.chromium)
        culprit = self.repo.commit('Roll rust revision', self.repo.chromium)
        self.repo._run_git_command(['tag', CHROMIUM_TAG], self.repo.chromium)
        return base, culprit

    def test_was_updated_true_on_revision_change(self):
        base, _ = self._seed_rust_range()
        self.assertTrue(self.rust.was_updated(base, CHROMIUM_TAG))

    def test_was_updated_false_without_change(self):
        base, _ = self._seed_rust_range()
        # base..base is an empty range: nothing changed.
        self.assertFalse(self.rust.was_updated(base, base))

    def test_find_culprit_pickaxes_the_bump(self):
        _, culprit = self._seed_rust_range()
        self.assertEqual(self.rust.find_culprit(CHROMIUM_TAG), culprit)

    def test_find_culprit_returns_explicit_value(self):
        self._seed_rust_range()
        self.assertEqual(self.rust.find_culprit(CHROMIUM_TAG, 'given'),
                         'given')

    def test_find_culprit_raises_when_unresolvable(self):
        self._seed_rust_range()
        # No culprit assignment readable -> nothing to pickaxe.
        with patch.object(self.rust, '_culprit_regex', return_value=None):
            with self.assertRaises(toolchain.InvalidInputException):
                self.rust.find_culprit(CHROMIUM_TAG)

    def test_check_reports_advisory_when_not_published(self):
        base, culprit = self._seed_rust_range()
        with patch.object(self.rust, 'is_published', return_value=False):
            advisory = self.rust.check(base, CHROMIUM_TAG)
        self.assertIsNotNone(advisory)
        self.assertIn('Rust toolchain', advisory.description)
        self.assertEqual(advisory.commit_hash, culprit)
        self.assertIn('Roll rust revision', advisory.commit_message)

    def test_check_suppressed_when_published(self):
        base, _ = self._seed_rust_range()
        with patch.object(self.rust, 'is_published', return_value=True):
            self.assertIsNone(self.rust.check(base, CHROMIUM_TAG))

    def test_check_none_without_change(self):
        base, _ = self._seed_rust_range()
        self.assertIsNone(self.rust.check(base, base))


class IsPublishedTest(unittest.TestCase):
    """`RustToolchain.is_published` probes the S3 archive by HEAD."""

    def setUp(self):
        self.rust = toolchain.RustToolchain()

    def _head(self, status=None, exc=None):
        with patch.object(self.rust, '_revision', return_value='r-s-c'):
            with patch('toolchain.requests.head') as head:
                if exc is not None:
                    head.side_effect = exc
                else:
                    head.return_value = MagicMock(status_code=status)
                return self.rust.is_published(CHROMIUM_TAG), head

    def test_published_when_200(self):
        published, head = self._head(status=200)
        self.assertTrue(published)
        # The probed URL carries the revision triple and the first brave
        # sub-revision suffix.
        self.assertIn('r-s-c-1.tar.xz', head.call_args.args[0])

    def test_not_published_when_404(self):
        published, _ = self._head(status=404)
        self.assertFalse(published)

    def test_not_published_on_request_error(self):
        published, _ = self._head(
            exc=toolchain.requests.RequestException('boom'))
        self.assertFalse(published)


class RecoverTest(unittest.TestCase):
    """`recover` closes the loop for Rust and is a no-op for the rest."""

    def setUp(self):
        self.rust = toolchain.RustToolchain()
        self.version = Version(CHROMIUM_TAG)

    def test_success_triggers_watches_and_repins(self):
        with patch.object(self.rust, 'trigger', return_value=True) as trigger, \
                patch.object(self.rust, 'repin') as repin:
            recovered = self.rust.recover(self.version, 'culprithash')

        self.assertTrue(recovered)
        trigger.assert_called_once_with(self.version,
                                        watch=True,
                                        brave_subrevision=1)
        repin.assert_called_once_with(self.version, 'culprithash')

    def test_failed_build_keeps_advisory(self):
        with patch.object(self.rust, 'trigger', return_value=False), \
                patch.object(self.rust, 'repin') as repin:
            self.assertFalse(self.rust.recover(self.version, 'h'))
        repin.assert_not_called()

    def test_missing_credentials_keeps_advisory(self):
        with patch.object(
                self.rust,
                'trigger',
                side_effect=toolchain.InvalidInputException('creds')):
            self.assertFalse(self.rust.recover(self.version, 'h'))

    def test_repin_failure_keeps_advisory(self):
        with patch.object(self.rust, 'trigger', return_value=True), \
                patch.object(self.rust,
                             'repin',
                             side_effect=toolchain.BadOutcomeException('boom')):
            self.assertFalse(self.rust.recover(self.version, 'h'))

    def test_base_toolchain_cannot_recover(self):
        # A toolchain without an override never recovers, so its advisory
        # always survives for the user.
        self.assertFalse(toolchain.XcodeToolchain().recover(self.version, 'h'))


if __name__ == '__main__':
    unittest.main()
