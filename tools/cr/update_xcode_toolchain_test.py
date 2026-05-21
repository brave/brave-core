#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `brockit.UpdateXcodeToolchain`.

The file is split in two layers, matching `rebase_v2_test.py`'s shape:

* `ReplaceConstantTest` exercises the pure file-content transform used to
  rewrite `build/mac/download_hermetic_xcode.py`. No git involved.

* `UpdateXcodeToolchainExecuteTest` drives
  `UpdateXcodeToolchain.execute` end-to-end against a `FakeChromiumRepo`,
  with the commit-msg hook installed so the `tags=toolchain` /
  `culprit=<hash>` env wiring is validated through the same pipeline that
  ships in production.
"""

from __future__ import annotations

import shutil
import stat
import subprocess
import unittest
from pathlib import Path

import brockit
import repository
from test.fake_chromium_repo import FakeChromiumRepo

# Source of the commit-msg hook. The integration tests symlink this into
# `.git/hooks/commit-msg` so the `tags` / `culprit` env vars are interpreted
# exactly the way they are in production.
HOOK_SOURCE: Path = (Path(__file__).resolve().parent / 'alias' /
                     'commit-msg.py')

# A representative archive URL emitted by
# `tools/cr/toolchain/build_xcode_toolchain.py`. The six dash-separated tokens
# in the filename are what `UpdateXcodeToolchain.execute` consumes.
TOOLCHAIN_URL = (
    'https://example.invalid/xcode-hermetic-toolchain/'
    'xcode-hermetic-toolchain-26.5-17F42-26.5-25F70-for-upstream-26.5-25F70'
    '.tar.gz')

# The starting on-disk content of `build/mac/download_hermetic_xcode.py`
# before the bump. Kept minimal -- the six constants are the only thing
# `UpdateXcodeToolchain.execute` rewrites.
HERMETIC_XCODE_SCRIPT_INITIAL = """\
# Sample header.

XCODE_VERSION = '26.4'
XCODE_BUILD_VERSION = '17E192'

MAC_SDK_OFFICIAL_VERSION = '26.4'
MAC_SDK_OFFICIAL_BUILD_VERSION = '25E236'

MAC_SDK_UPSTREAM_VERSION = '26.4'
MAC_SDK_UPSTREAM_BUILD_VERSION = '25E236'
"""

# The starting content of `build/config/mac/mac_sdk.gni` on the chromium
# side, before the upstream switch. The first commit that introduces the
# matching upstream build number is what `_resolve_chromium_commit`'s
# pickaxe is supposed to find.
MAC_SDK_GNI_INITIAL = """\
mac_sdk_official_version = "26.4"
mac_sdk_official_build_version = "25E236"
"""

MAC_SDK_GNI_BUMPED = """\
mac_sdk_official_version = "26.5"
mac_sdk_official_build_version = "25F70"
"""


class ReplaceConstantTest(unittest.TestCase):
    """Tests for `UpdateXcodeToolchain._replace_constant`."""

    SAMPLE = ("XCODE_VERSION = '26.4'\n"
              "XCODE_BUILD_VERSION = '17E192'\n"
              'MAC_SDK_OFFICIAL_VERSION = "26.4"\n')

    def test_rewrites_single_quoted_value(self):
        result = brockit.UpdateXcodeToolchain._replace_constant(
            self.SAMPLE, 'XCODE_VERSION', '26.5')
        self.assertIn("XCODE_VERSION = '26.5'", result)
        # Untouched lines pass through unchanged.
        self.assertIn("XCODE_BUILD_VERSION = '17E192'", result)
        self.assertIn('MAC_SDK_OFFICIAL_VERSION = "26.4"', result)

    def test_rewrites_double_quoted_value(self):
        """Double-quoted assignments are matched and normalised to
        single-quoted output (matching the script's existing style)."""
        result = brockit.UpdateXcodeToolchain._replace_constant(
            self.SAMPLE, 'MAC_SDK_OFFICIAL_VERSION', '26.5')
        self.assertIn("MAC_SDK_OFFICIAL_VERSION = '26.5'", result)

    def test_replaces_only_first_occurrence(self):
        """The pattern is anchored at line start with `count=1`. A bare
        substring match elsewhere in the file (e.g. a comment that
        mentions `XCODE_VERSION`) does not get rewritten."""
        text = ("# XCODE_VERSION = '???' is the constant below.\n"
                "XCODE_VERSION = '26.4'\n")
        result = brockit.UpdateXcodeToolchain._replace_constant(
            text, 'XCODE_VERSION', '26.5')
        self.assertEqual(
            result, "# XCODE_VERSION = '???' is the constant below.\n"
            "XCODE_VERSION = '26.5'\n")

    def test_missing_constant_raises(self):
        """A constant that doesn't exist in the file at all surfaces as an
        `InvalidInputException` rather than silently leaving the file
        unchanged."""
        with self.assertRaises(brockit.InvalidInputException):
            brockit.UpdateXcodeToolchain._replace_constant(
                self.SAMPLE, 'NOT_A_REAL_CONSTANT', '26.5')


class UpdateXcodeToolchainExecuteTest(unittest.TestCase):
    """End-to-end tests for `UpdateXcodeToolchain.execute`.

    Each test seeds the fake brave repo with a starting
    `build/mac/download_hermetic_xcode.py`, optionally seeds the fake
    chromium repo with a `build/config/mac/mac_sdk.gni` bump, installs the
    commit-msg hook so the env-var plumbing is exercised end-to-end, and
    then drives `UpdateXcodeToolchain.execute` directly.
    """

    def setUp(self):
        self.repo = FakeChromiumRepo()
        self.repo.setup()
        self.addCleanup(self.repo.cleanup)
        self._checkout_cr_branch()
        self._install_commit_msg_hook()

    # -- fixture helpers ----------------------------------------------------

    def _checkout_cr_branch(self, name: str = 'cr150') -> None:
        """Switches the fake brave repo onto a `cr{N}` branch.

        The commit-msg hook keys off the current branch name to inject the
        `[cr{N}]` prefix into the final commit message, so every
        integration test runs on a branch that matches that pattern.
        """
        self.repo._run_git_command(['checkout', '-b', name], self.repo.brave)

    def _install_commit_msg_hook(self) -> None:
        """Copies `tools/cr/alias/commit-msg.py` into the fake brave repo's
        `.git/hooks/commit-msg` and marks it executable.

        Using a copy (rather than a symlink) keeps the fixture
        self-contained -- the hook keeps working even after the source
        worktree the test was launched from has been wiped by other test
        cleanup.
        """
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
        """Writes `build/mac/download_hermetic_xcode.py` into fake brave
        with the supplied content and commits it.

        Commits via the hook-bypass path (`--no-verify`) so this fixture
        commit doesn't try to invoke the hook before we've seeded any
        culprit history.
        """
        script_path = self.repo.brave / brockit.HERMETIC_XCODE_SCRIPT
        script_path.parent.mkdir(parents=True, exist_ok=True)
        script_path.write_text(content, encoding='utf-8', newline='')
        self.repo._run_git_command(['add', str(script_path)], self.repo.brave)
        self.repo._run_git_command(
            ['commit', '--no-verify', '-m', 'Seed hermetic xcode script'],
            self.repo.brave)

    def _seed_mac_sdk_bump(self,
                           initial: str = MAC_SDK_GNI_INITIAL,
                           bumped: str | None = MAC_SDK_GNI_BUMPED) -> str:
        """Lays down `build/config/mac/mac_sdk.gni` on fake chromium with
        two commits: the initial pin, then the bump.

        Returns the hash of the *bump* commit -- that's what the pickaxe
        in `_resolve_chromium_commit` is supposed to find. When `bumped`
        is `None`, only the initial commit is created (used to test the
        unresolvable-commit failure path).
        """
        gni = self.repo.chromium / 'build' / 'config' / 'mac' / 'mac_sdk.gni'
        gni.parent.mkdir(parents=True, exist_ok=True)
        gni.write_text(initial, encoding='utf-8', newline='')
        self.repo._run_git_command(['add', str(gni)], self.repo.chromium)
        self.repo._run_git_command(['commit', '-m', 'mac: initial SDK pin'],
                                   self.repo.chromium)

        if bumped is None:
            return ''

        gni.write_text(bumped, encoding='utf-8', newline='')
        self.repo._run_git_command(['add', str(gni)], self.repo.chromium)
        self.repo._run_git_command(['commit', '-m', 'mac: Switch to SDK 26.5'],
                                   self.repo.chromium)
        return self.repo._run_git_command(['rev-parse', 'HEAD'],
                                          self.repo.chromium)

    def _last_brave_commit_message(self) -> str:
        return self.repo._run_git_command(['log', '-1', '--format=%B'],
                                          self.repo.brave)

    def _last_brave_subject(self) -> str:
        return self.repo._run_git_command(['log', '-1', '--format=%s'],
                                          self.repo.brave)

    def _read_hermetic_xcode_script(self) -> str:
        return (self.repo.brave /
                brockit.HERMETIC_XCODE_SCRIPT).read_text(encoding='utf-8')

    # -- tests --------------------------------------------------------------

    def test_constants_rewritten_and_committed(self):
        """Happy path: every constant takes its value from the URL, the
        edit lands on disk, and the commit-msg hook stamps the final
        message with `[cr150][toolchain]` plus the chromium culprit
        body."""
        self._seed_hermetic_xcode_script()
        culprit = self._seed_mac_sdk_bump()

        brockit.UpdateXcodeToolchain().execute(url=TOOLCHAIN_URL, culprit=None)

        script = self._read_hermetic_xcode_script()
        self.assertIn("XCODE_VERSION = '26.5'", script)
        self.assertIn("XCODE_BUILD_VERSION = '17F42'", script)
        self.assertIn("MAC_SDK_OFFICIAL_VERSION = '26.5'", script)
        self.assertIn("MAC_SDK_OFFICIAL_BUILD_VERSION = '25F70'", script)
        self.assertIn("MAC_SDK_UPSTREAM_VERSION = '26.5'", script)
        self.assertIn("MAC_SDK_UPSTREAM_BUILD_VERSION = '25F70'", script)

        message = self._last_brave_commit_message()
        # The hook must have stamped both tags. The relative order between
        # `[cr150]` and `[toolchain]` comes from set iteration in
        # `commit-msg.py`, so just assert both are present in the
        # subject.
        subject = message.splitlines()[0]
        self.assertIn('[cr150]', subject)
        self.assertIn('[toolchain]', subject)
        self.assertIn('Switch to Xcode 26.5 17F42', subject)

        # The culprit body the hook appends includes the link and the
        # full `git log -1 <hash>` payload from chromium.
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{culprit}',
            message)
        self.assertIn('mac: Switch to SDK 26.5', message)

    def test_culprit_override_skips_auto_detection(self):
        """An explicit `culprit=<hash>` bypasses the mac_sdk.gni pickaxe
        entirely. We prove this by passing a hash that the auto-detector
        would never have produced -- a commit on the chromium master
        branch that doesn't touch `mac_sdk.gni` at all."""
        self._seed_hermetic_xcode_script()
        # Seed an unrelated chromium commit so we have a real hash to
        # reference.
        self.repo.write_and_stage_file('docs/unrelated.txt', 'noise\n',
                                       self.repo.chromium)
        override_hash = self.repo.commit('Unrelated chromium change',
                                         self.repo.chromium)
        # Seed mac_sdk.gni only with the initial state; a successful run
        # here means the auto-detector was never asked.
        self._seed_mac_sdk_bump(bumped=None)

        brockit.UpdateXcodeToolchain().execute(url=TOOLCHAIN_URL,
                                               culprit=override_hash)

        message = self._last_brave_commit_message()
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{override_hash}',
            message)
        self.assertIn('Unrelated chromium change', message)

    def test_already_pinned_raises(self):
        """When `build/mac/download_hermetic_xcode.py` already carries the
        URL's six tokens, the regex substitutions are all no-ops and the
        task aborts instead of producing an empty commit."""
        self._seed_hermetic_xcode_script(
            content=("XCODE_VERSION = '26.5'\n"
                     "XCODE_BUILD_VERSION = '17F42'\n"
                     "MAC_SDK_OFFICIAL_VERSION = '26.5'\n"
                     "MAC_SDK_OFFICIAL_BUILD_VERSION = '25F70'\n"
                     "MAC_SDK_UPSTREAM_VERSION = '26.5'\n"
                     "MAC_SDK_UPSTREAM_BUILD_VERSION = '25F70'\n"))
        self._seed_mac_sdk_bump()
        head_before = self.repo._run_git_command(['rev-parse', 'HEAD'],
                                                 self.repo.brave)

        with self.assertRaises(brockit.InvalidInputException):
            brockit.UpdateXcodeToolchain().execute(url=TOOLCHAIN_URL,
                                                   culprit=None)

        # No commit was produced.
        head_after = self.repo._run_git_command(['rev-parse', 'HEAD'],
                                                self.repo.brave)
        self.assertEqual(head_before, head_after)

    def test_bad_url_raises(self):
        """A URL whose filename doesn't fit the
        `xcode-hermetic-toolchain-<...>.tar.gz` shape is rejected before
        any disk state is touched."""
        self._seed_hermetic_xcode_script()
        self._seed_mac_sdk_bump()
        original = self._read_hermetic_xcode_script()

        with self.assertRaises(brockit.InvalidInputException):
            brockit.UpdateXcodeToolchain().execute(
                url='https://example.invalid/some-other-archive.tar.gz',
                culprit=None)

        self.assertEqual(self._read_hermetic_xcode_script(), original)

    def test_unresolvable_chromium_commit_raises(self):
        """When `mac_sdk.gni` has never carried the URL's upstream build
        number, the pickaxe returns nothing and the task points the user
        at `--culprit` to recover."""
        self._seed_hermetic_xcode_script()
        # Seed mac_sdk.gni without ever introducing the URL's build
        # number, so `git log -S 25F70 -- build/config/mac/mac_sdk.gni`
        # comes back empty.
        self._seed_mac_sdk_bump(bumped=None)
        head_before = self.repo._run_git_command(['rev-parse', 'HEAD'],
                                                 self.repo.brave)

        with self.assertRaises(brockit.InvalidInputException):
            brockit.UpdateXcodeToolchain().execute(url=TOOLCHAIN_URL,
                                                   culprit=None)

        # The script edit happens before the chromium lookup, so the
        # working tree may be dirty. What we care about here is that no
        # commit slipped through.
        head_after = self.repo._run_git_command(['rev-parse', 'HEAD'],
                                                self.repo.brave)
        self.assertEqual(head_before, head_after)


if __name__ == '__main__':
    unittest.main()
