#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `brockit.UpdateXcodeToolchain`.

The file is split into layers, matching `update_rust_wasm_toolchain_test.py`'s
shape:

* `ProvenanceCommentTest` exercises the pure provenance-comment transform used
  to rewrite `build/mac/download_hermetic_xcode.py`. No git or network involved.

* `UpdateXcodeToolchainExecuteTest` drives `UpdateXcodeToolchain.execute`
  end-to-end against a `FakeChromiumRepo` (the published-index fetch is faked,
  so no network), with the commit-msg hook installed so the `tags=toolchain` /
  `culprit=<hash>` env wiring is validated through the same pipeline that ships
  in production.
"""

from __future__ import annotations

import shutil
import stat
import unittest
from pathlib import Path
from unittest.mock import patch

import brockit
from test.fake_chromium_repo import FakeChromiumRepo

# Source of the commit-msg hook. The integration tests copy this into
# `.git/hooks/commit-msg` so the `tags` / `culprit` env vars are interpreted
# exactly the way they are in production.
HOOK_SOURCE: Path = (Path(__file__).resolve().parent / 'alias' /
                     'commit-msg.py')

# The Chromium tag `execute` resolves `--to` against; the mac_sdk.gni bump
# commit is tagged with it so the SDK pin can be read at the ref.
CHROMIUM_TAG = '150.0.7850.1'

# What the faked `build_xcode_toolchain.fetch_published_index` returns -- the
# published index for the bumped SDK (26.5 / 25F70). The SDK version/build are
# carried by the index `url`/name; the fields below are what brockit pins and
# titles the commit from.
FAKE_INDEX = {
    'url': ('https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.'
            'on.aws/xcode-hermetic-toolchain/'
            'xcode-hermetic-toolchain-26.5-25F70.tar.gz'),
    'sha256sum': 'b' * 64,
    'xcode_version': '26.5',
    'xcode_build': '17F42',
    'metal_build': '17E188',
    'chromium_tag': CHROMIUM_TAG,
}

# The starting on-disk content of `build/mac/download_hermetic_xcode.py` before
# the bump. The archive hash, the two SDK constants, the provenance comment, and
# the `MAC_MINIMUM_OS_VERSION` block are what `UpdateXcodeToolchain.execute`
# rewrites; everything else must survive untouched.
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

# The starting content of `build/config/mac/mac_sdk.gni` on the chromium side,
# before the upstream switch. The first commit that introduces the matching
# upstream build number is what `_resolve_chromium_culprit`'s pickaxe is
# supposed to find.
MAC_SDK_GNI_INITIAL = """\
mac_sdk_official_version = "26.4"
mac_sdk_official_build_version = "25E236"
"""

MAC_SDK_GNI_BUMPED = """\
mac_sdk_official_version = "26.5"
mac_sdk_official_build_version = "25F70"
"""

# Chromium's `build/mac_toolchain.py` at the bumped ref. Its
# `MAC_MINIMUM_OS_VERSION` block (a distinct comment and a bumped `[20, 0]`
# value) is what `execute` lifts verbatim into the downloader. The surrounding
# lines exist so the extraction regex has to isolate just the block.
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


class ProvenanceCommentTest(unittest.TestCase):
    """Tests for `UpdateXcodeToolchain._provenance_comment`."""

    SDK_INFO = brockit.build_xcode_toolchain.MacSdkInfo(
        sdk_version='26.5', product_build_version='25F70')

    def test_records_xcode_sdk_and_metal(self):
        comment = brockit.UpdateXcodeToolchain._provenance_comment(
            self.SDK_INFO, FAKE_INDEX)
        # Every line is a comment, stays within the column budget, and the
        # joined prose names each component the archive bundles.
        lines = comment.splitlines()
        self.assertTrue(all(line.startswith('# ') for line in lines))
        self.assertTrue(all(len(line) <= 79 for line in lines))
        prose = ' '.join(line[2:] for line in lines)
        self.assertEqual(
            prose, 'This contains binaries from Xcode 26.5 (17F42) along with '
            'the macOS 26.5 SDK (25F70) and the Metal toolchain (17E188).')

    def test_missing_metal_build_falls_back_to_unknown(self):
        comment = brockit.UpdateXcodeToolchain._provenance_comment(
            self.SDK_INFO, {
                **FAKE_INDEX, 'metal_build': None
            })
        self.assertIn('Metal toolchain (unknown)',
                      comment.replace('\n# ', ' '))


class UpdateXcodeToolchainExecuteTest(unittest.TestCase):
    """End-to-end tests for `UpdateXcodeToolchain.execute`.

    Each test seeds the fake brave repo with a starting
    `build/mac/download_hermetic_xcode.py`, seeds the fake chromium repo with a
    `build/config/mac/mac_sdk.gni` bump tagged as `CHROMIUM_TAG`, fakes the
    published-index fetch, installs the commit-msg hook so the env-var plumbing
    is exercised end-to-end, and then drives `UpdateXcodeToolchain.execute`
    directly.
    """

    def setUp(self):
        self.repo = FakeChromiumRepo()
        self.repo.setup()
        self.addCleanup(self.repo.cleanup)
        self._checkout_cr_branch()
        self._install_commit_msg_hook()
        self._seed_hermetic_xcode_script()

        patcher = patch.object(brockit.build_xcode_toolchain,
                               'fetch_published_index',
                               return_value=FAKE_INDEX)
        self.fetch_index = patcher.start()
        self.addCleanup(patcher.stop)

    # -- fixture helpers ----------------------------------------------------

    def _checkout_cr_branch(self, name: str = 'cr150') -> None:
        """Switches the fake brave repo onto a `cr{N}` branch.

        The commit-msg hook keys off the current branch name to inject the
        `[cr{N}]` prefix into the final commit message, so every integration
        test runs on a branch that matches that pattern.
        """
        self.repo._run_git_command(['checkout', '-b', name], self.repo.brave)

    def _install_commit_msg_hook(self) -> None:
        """Copies `tools/cr/alias/commit-msg.py` into the fake brave repo's
        `.git/hooks/commit-msg` and marks it executable."""
        hooks_dir = self.repo.brave / '.git' / 'hooks'
        hooks_dir.mkdir(exist_ok=True)
        dest = hooks_dir / 'commit-msg'
        if dest.exists() or dest.is_symlink():
            dest.unlink()
        shutil.copy2(HOOK_SOURCE, dest)
        dest.chmod(dest.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP
                   | stat.S_IXOTH)

    def _seed_hermetic_xcode_script(
            self, content: str = HERMETIC_XCODE_SCRIPT_INITIAL) -> None:
        """Writes `build/mac/download_hermetic_xcode.py` into fake brave with
        the supplied content and commits it (hook-bypassed via `--no-verify`,
        since no culprit history is seeded yet)."""
        script_path = self.repo.brave / brockit.HERMETIC_XCODE_SCRIPT
        script_path.parent.mkdir(parents=True, exist_ok=True)
        script_path.write_text(content, encoding='utf-8', newline='')
        self.repo._run_git_command(['add', str(script_path)], self.repo.brave)
        self.repo._run_git_command(
            ['commit', '--no-verify', '-m', 'Seed hermetic xcode script'],
            self.repo.brave)

    def _seed_mac_sdk_bump(self,
                           mac_toolchain_py: str = MAC_TOOLCHAIN_PY) -> str:
        """Lays down `build/config/mac/mac_sdk.gni` on fake chromium with two
        commits -- the initial pin then the bump to 26.5/25F70 -- writes
        `build/mac_toolchain.py` alongside the bump, and tags the bump commit as
        `CHROMIUM_TAG`.

        Returns the bump commit hash: the ref `execute` reads the SDK pin and
        the minimum-OS block from, and what `_resolve_chromium_culprit`'s
        pickaxe should attribute the update to.
        """
        gni = self.repo.chromium / 'build' / 'config' / 'mac' / 'mac_sdk.gni'
        gni.parent.mkdir(parents=True, exist_ok=True)
        gni.write_text(MAC_SDK_GNI_INITIAL, encoding='utf-8', newline='')
        self.repo._run_git_command(['add', str(gni)], self.repo.chromium)
        self.repo._run_git_command(['commit', '-m', 'mac: initial SDK pin'],
                                   self.repo.chromium)

        gni.write_text(MAC_SDK_GNI_BUMPED, encoding='utf-8', newline='')
        toolchain = self.repo.chromium / 'build' / 'mac_toolchain.py'
        toolchain.write_text(mac_toolchain_py, encoding='utf-8', newline='')
        self.repo._run_git_command(
            ['add', str(gni), str(toolchain)], self.repo.chromium)
        culprit = self.repo.commit('mac: Switch to SDK 26.5',
                                   self.repo.chromium)
        self.repo._run_git_command(['tag', CHROMIUM_TAG], self.repo.chromium)
        return culprit

    def _last_brave_commit_message(self) -> str:
        return self.repo._run_git_command(['log', '-1', '--format=%B'],
                                          self.repo.brave)

    def _brave_head(self) -> str:
        return self.repo._run_git_command(['rev-parse', 'HEAD'],
                                          self.repo.brave)

    def _read_hermetic_xcode_script(self) -> str:
        return (self.repo.brave /
                brockit.HERMETIC_XCODE_SCRIPT).read_text(encoding='utf-8')

    # -- tests --------------------------------------------------------------

    def test_constants_rewritten_and_committed(self):
        """Happy path: the archive hash and SDK constants take their values
        from the index/SDK pin, the provenance comment is regenerated, the edit
        lands on disk, and the commit-msg hook stamps the final message with
        `[cr150][toolchain]` plus the chromium culprit body."""
        culprit = self._seed_mac_sdk_bump()

        brockit.UpdateXcodeToolchain().execute(chromium_ref=CHROMIUM_TAG,
                                               culprit=None)

        script = self._read_hermetic_xcode_script()
        self.assertIn(f"MAC_BINARIES_HASH = '{FAKE_INDEX['sha256sum']}'",
                      script)
        self.assertIn("MAC_SDK_OFFICIAL_VERSION = '26.5'", script)
        self.assertIn("MAC_SDK_OFFICIAL_BUILD_VERSION = '25F70'", script)
        self.assertNotIn("'oldhash'", script)
        # The provenance comment was regenerated from the resolved toolchain.
        sdk_info = brockit.build_xcode_toolchain.MacSdkInfo(
            sdk_version='26.5', product_build_version='25F70')
        self.assertIn(
            brockit.UpdateXcodeToolchain._provenance_comment(
                sdk_info, FAKE_INDEX), script)
        # The MAC_MINIMUM_OS_VERSION block was lifted verbatim from upstream
        # (bumped value + its distinct comment), replacing the old block.
        self.assertIn('MAC_MINIMUM_OS_VERSION = [20, 0]', script)
        self.assertIn('runs on macOS 26.3 and newer', script)
        self.assertNotIn('MAC_MINIMUM_OS_VERSION = [19, 4]', script)
        self.assertNotIn('runs on macOS 26.1 and newer', script)
        # The unrelated upstream lines were not dragged along with the block.
        self.assertNotIn('OTHER_CONSTANT', script)
        self.assertNotIn('MAC_BINARIES_TAG', script)

        # The index was resolved for the SDK read from mac_sdk.gni at the ref.
        self.assertEqual(self.fetch_index.call_args.args[0], sdk_info)

        message = self._last_brave_commit_message()
        # The hook must have stamped both tags. The relative order between
        # `[cr150]` and `[toolchain]` comes from set iteration in
        # `commit-msg.py`, so just assert both are present in the subject.
        subject = message.splitlines()[0]
        self.assertIn('[cr150]', subject)
        self.assertIn('[toolchain]', subject)
        self.assertIn('Switch to Xcode 26.5 17F42', subject)

        # The culprit body the hook appends includes the link and the full
        # `git log -1 <hash>` payload from chromium.
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{culprit}',
            message)
        self.assertIn('mac: Switch to SDK 26.5', message)

    def test_culprit_override_skips_auto_detection(self):
        """An explicit `culprit=<hash>` bypasses the mac_sdk.gni pickaxe
        entirely. We prove this by passing a hash that the auto-detector would
        never have produced -- an unrelated chromium commit -- and asserting
        the auto-detected bump hash is absent from the message."""
        autodetect = self._seed_mac_sdk_bump()
        self.repo.write_and_stage_file('docs/unrelated.txt', 'noise\n',
                                       self.repo.chromium)
        override = self.repo.commit('Unrelated chromium change',
                                    self.repo.chromium)

        brockit.UpdateXcodeToolchain().execute(chromium_ref=CHROMIUM_TAG,
                                               culprit=override)

        message = self._last_brave_commit_message()
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{override}',
            message)
        self.assertIn('Unrelated chromium change', message)
        self.assertNotIn(autodetect, message)

    def test_already_pinned_second_run_raises(self):
        """A second run over an already-pinned file is a byte-identical no-op:
        nothing is rewritten, the task aborts instead of producing an empty
        commit, and HEAD is unchanged."""
        self._seed_mac_sdk_bump()
        toolchain = brockit.UpdateXcodeToolchain()

        toolchain.execute(chromium_ref=CHROMIUM_TAG, culprit=None)
        head_after_first = self._brave_head()

        with self.assertRaises(brockit.InvalidInputException):
            toolchain.execute(chromium_ref=CHROMIUM_TAG, culprit=None)
        self.assertEqual(self._brave_head(), head_after_first)

    def test_index_fetch_failure_raises(self):
        """A failure resolving the published index surfaces as a
        `BadOutcomeException` and produces no commit."""
        self._seed_mac_sdk_bump()
        self.fetch_index.side_effect = RuntimeError('index not found')
        head_before = self._brave_head()

        with self.assertRaises(brockit.BadOutcomeException):
            brockit.UpdateXcodeToolchain().execute(chromium_ref=CHROMIUM_TAG,
                                                   culprit=None)
        self.assertEqual(self._brave_head(), head_before)

    def test_missing_upstream_min_os_block_raises(self):
        """When Chromium's `build/mac_toolchain.py` carries no
        `MAC_MINIMUM_OS_VERSION` block, the task aborts (there is nothing to
        mirror) and produces no commit."""
        self._seed_mac_sdk_bump(
            mac_toolchain_py='# upstream with no min-os block\nFOO = 1\n')
        head_before = self._brave_head()

        with self.assertRaises(brockit.InvalidInputException):
            brockit.UpdateXcodeToolchain().execute(chromium_ref=CHROMIUM_TAG,
                                                   culprit=None)
        self.assertEqual(self._brave_head(), head_before)

    def test_unresolvable_culprit_raises(self):
        """`_resolve_chromium_culprit` raises when no commit up to the ref set
        the SDK being pinned, steering the user at `--culprit` to recover."""
        ref = self._seed_mac_sdk_bump()
        # An SDK pair that mac_sdk.gni never carried in its history: the
        # pickaxe comes back empty.
        phantom = brockit.build_xcode_toolchain.MacSdkInfo(
            sdk_version='99.9', product_build_version='Z9Z9')
        with self.assertRaises(brockit.InvalidInputException):
            brockit.UpdateXcodeToolchain()._resolve_chromium_culprit(
                phantom, ref)

    def test_staged_files_block_the_update(self):
        self.repo.write_and_stage_file('staged.txt', 'wip\n', self.repo.brave)
        with self.assertRaises(brockit.InvalidInputException):
            brockit.UpdateXcodeToolchain().execute(chromium_ref=CHROMIUM_TAG,
                                                   culprit='deadbeef')


if __name__ == '__main__':
    unittest.main()
