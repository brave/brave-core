#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `brockit.UpdateRustWasmToolchain` and `_render_py_literal`.

Two layers:

* `RenderLiteralTest` / `LoadExtraDepsTest` cover the pure source-handling --
  rendering an `EXTRA_DEPS` literal and reading it back out with `ast`.

* `UpdateExecuteTest` drives `execute` end-to-end against a `FakeChromiumRepo`
  with the commit-msg hook installed (the builder is faked, so no network),
  validating the file rewrite *and* the `tags=toolchain` / `culprit=<hash>`
  commit -- the same way `UpdateXcodeToolchainExecuteTest` works.
"""

from __future__ import annotations

import ast
import shutil
import stat
import unittest
from pathlib import Path
from unittest.mock import patch

import brockit
from test.fake_chromium_repo import FakeChromiumRepo

# Source of the commit-msg hook, symlinked/copied into the fake brave repo so
# the `tags` / `culprit` env wiring is exercised exactly as in production.
HOOK_SOURCE: Path = (Path(__file__).resolve().parent / 'alias' /
                     'commit-msg.py')

# A stand-in for install_extra_deps.py: two EXTRA_DEPS entries (only the
# rust-toolchain one should change) plus trailing code that must survive.
SAMPLE_FILE = """\
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

# What the faked builder returns -- a fresh rust-toolchain entry.
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


class RenderLiteralTest(unittest.TestCase):
    """Tests for `brockit._render_py_literal`."""

    def test_round_trips(self):
        self.assertEqual(
            ast.literal_eval(brockit._render_py_literal(NEW_ENTRY)), NEW_ENTRY)

    def test_single_quoted_with_embedded_double_quotes(self):
        rendered = brockit._render_py_literal(
            {'condition': 'host_os == "mac" and host_cpu == "arm64"'})
        self.assertIn(
            "    'condition': 'host_os == \"mac\" and host_cpu == \"arm64\"',",
            rendered)

    def test_empty_containers(self):
        self.assertEqual(brockit._render_py_literal({}), '{}')
        self.assertEqual(brockit._render_py_literal([]), '[]')


class CommitTitleTest(unittest.TestCase):
    """Tests for `UpdateRustWasmToolchain._commit_title`."""

    @staticmethod
    def _entry(object_name: str) -> dict:
        """Wrap `object_name` in the minimal `EXTRA_DEPS`-shaped dict the
        title builder reads its first object from."""
        return {
            'src/third_party/rust-toolchain': {
                'objects': [{
                    'object_name': object_name
                }],
            }
        }

    def test_title_uses_upstream_rust_sub_revision(self):
        # Full archive name with an upstream rust sub revision (`2`) that
        # differs from Brave's respin counter (`1`), so a title that wrongly
        # rendered the brave sub revision would read `sub 1`.
        name = ('linux-x64-rust-toolchain-'
                '4c4205163abcbd08948b3efab796c543ba1ea687-2-'
                'llvmorg-23-init-10931-g20b6ec66-1.tar.xz')
        title = brockit.UpdateRustWasmToolchain._commit_title(
            self._entry(name))
        self.assertEqual(
            title, 'Rust/WASM toolchain (4c4205163abc-2, '
            'llvmorg-23-init-10931-g20b6ec66, sub 2)')

    def test_unparseable_name_falls_back_to_stem(self):
        title = brockit.UpdateRustWasmToolchain._commit_title(
            self._entry('linux-x64-rust-toolchain-2.tar.xz'))
        self.assertEqual(title,
                         'Rust/WASM toolchain linux-x64-rust-toolchain-2')


class LoadExtraDepsTest(unittest.TestCase):
    """Tests for `UpdateRustWasmToolchain._load_extra_deps`."""

    def test_returns_node_and_value(self):
        node, value = brockit.UpdateRustWasmToolchain._load_extra_deps(
            SAMPLE_FILE)
        self.assertEqual(node.targets[0].id, 'EXTRA_DEPS')
        self.assertIn('src/third_party/rust-toolchain', value)
        self.assertIn('src/other-dep', value)

    def test_missing_assignment_raises(self):
        with self.assertRaises(brockit.InvalidInputException):
            brockit.UpdateRustWasmToolchain._load_extra_deps('X = 1\n')


class UpdateExecuteTest(unittest.TestCase):
    """End-to-end `execute` against a `FakeChromiumRepo` (builder faked)."""

    INSTALLER = 'tools/cr/toolchains/install_extra_deps.py'

    # The Chromium tag `execute` resolves `--to` against; the revision-bump
    # commit is tagged with it so the revision scripts can be read at the ref.
    CHROMIUM_TAG = '150.0.7850.1'

    def setUp(self):
        self.repo = FakeChromiumRepo()
        self.repo.setup()
        self.addCleanup(self.repo.cleanup)
        self._checkout_cr_branch()
        self._install_commit_msg_hook()
        self._seed_installer()

        patcher = patch.object(brockit.build_rust_toolchain,
                               'rust_toolchain_extra_dep',
                               return_value=NEW_ENTRY)
        self.builder = patcher.start()
        self.addCleanup(patcher.stop)

    # -- fixture helpers ----------------------------------------------------

    def _checkout_cr_branch(self, name: str = 'cr150') -> None:
        self.repo._run_git_command(['checkout', '-b', name], self.repo.brave)

    def _install_commit_msg_hook(self) -> None:
        hooks_dir = self.repo.brave / '.git' / 'hooks'
        hooks_dir.mkdir(exist_ok=True)
        dest = hooks_dir / 'commit-msg'
        if dest.exists() or dest.is_symlink():
            dest.unlink()
        shutil.copy2(HOOK_SOURCE, dest)
        dest.chmod(dest.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP
                   | stat.S_IXOTH)

    def _seed_installer(self, content: str = SAMPLE_FILE) -> None:
        path = self.repo.brave / self.INSTALLER
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8', newline='')
        self.repo._run_git_command(['add', str(path)], self.repo.brave)
        self.repo._run_git_command(
            ['commit', '--no-verify', '-m', 'Seed installer'], self.repo.brave)

    def _seed_rust_revision_bump(self) -> str:
        """Seed a Chromium commit introducing the Rust/Clang revision constants
        in both scripts and tag it as `CHROMIUM_TAG`, returning its hash -- the
        ref `execute` reads the revision constants from, and what the pickaxe in
        `_resolve_chromium_culprit` should attribute the update to."""
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
        self.repo._run_git_command(['tag', self.CHROMIUM_TAG],
                                   self.repo.chromium)
        return culprit

    def _installer_text(self) -> str:
        return (self.repo.brave / self.INSTALLER).read_bytes().decode('utf-8')

    def _brave_head(self) -> str:
        return self.repo._run_git_command(['rev-parse', 'HEAD'],
                                          self.repo.brave)

    def _last_brave_message(self) -> str:
        return self.repo._run_git_command(['log', '-1', '--format=%B'],
                                          self.repo.brave)

    # -- tests --------------------------------------------------------------

    def test_updates_and_commits_with_auto_culprit(self):
        culprit = self._seed_rust_revision_bump()

        brockit.UpdateRustWasmToolchain().execute(
            chromium_ref=self.CHROMIUM_TAG, culprit=None)

        text = self._installer_text()
        self.assertIn('linux-x64-rust-toolchain-2.tar.xz', text)
        self.assertIn('win-rust-toolchain-2.tar.xz', text)
        self.assertNotIn('old-linux-1.tar.xz', text)
        # The unrelated entry and trailing code survive.
        self.assertIn("'src/other-dep'", text)
        self.assertIn('def untouched():', text)

        message = self._last_brave_message()
        subject = message.splitlines()[0]
        self.assertIn('[cr150]', subject)
        self.assertIn('[toolchain]', subject)
        self.assertIn('Rust/WASM toolchain', subject)
        # Auto-detected culprit rendered into the body.
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{culprit}',
            message)
        self.assertIn('Roll clang+rust revision', message)

    def test_explicit_culprit_overrides_autodetect(self):
        # Seed the revision bump (read for the toolchain stem and what
        # auto-detect would attribute to) then point --culprit at an unrelated
        # commit, proving the explicit value wins over auto-detection.
        autodetect = self._seed_rust_revision_bump()
        self.repo.write_and_stage_file('docs/unrelated.txt', 'noise\n',
                                       self.repo.chromium)
        culprit = self.repo.commit('Unrelated chromium change',
                                   self.repo.chromium)

        brockit.UpdateRustWasmToolchain().execute(
            chromium_ref=self.CHROMIUM_TAG, culprit=culprit)

        message = self._last_brave_message()
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{culprit}',
            message)
        self.assertIn('Unrelated chromium change', message)
        # The auto-detected culprit was overridden, not used.
        self.assertNotIn(autodetect, message)

    def test_idempotent_second_run_does_not_commit(self):
        self._seed_rust_revision_bump()
        toolchain = brockit.UpdateRustWasmToolchain()

        toolchain.execute(chromium_ref=self.CHROMIUM_TAG, culprit=None)
        head_after_first = self._brave_head()

        # The file already holds NEW_ENTRY, so a re-run is a byte-identical
        # no-op and must not produce a second commit.
        toolchain.execute(chromium_ref=self.CHROMIUM_TAG, culprit=None)
        self.assertEqual(self._brave_head(), head_after_first)

    def test_staged_files_block_the_update(self):
        self.repo.write_and_stage_file('staged.txt', 'wip\n', self.repo.brave)
        with self.assertRaises(brockit.InvalidInputException):
            brockit.UpdateRustWasmToolchain().execute(
                chromium_ref=self.CHROMIUM_TAG, culprit='deadbeef')


if __name__ == '__main__':
    unittest.main()
