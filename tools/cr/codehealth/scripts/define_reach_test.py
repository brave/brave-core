#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for define_reach.py."""

from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import define_reach
from define_reach import DefineReach, Reach


class DefinesTest(unittest.TestCase):
    def test_defines(self) -> None:
        text = (
            '#ifndef FOO_H_\n#define FOO_H_\n'
            '#define Foo BraveFoo\n'
            '#define Bar(x) Bar_ChromiumImpl(x)\n'
            '#define BRAVE_FOO_EXTRA x\n'
            '#define FLAG\n'
            '#define HELPER(x) x\n'
            'HELPER(1)\n'
            '#include <foo.h>\n#undef Foo\n#undef Bar\n'
        )
        self.assertEqual(
            [('Foo', False), ('Bar', False), ('HELPER', True)],
            DefineReach('a', 'b').defines(text),
        )

    def test_helpers_used_by_an_included_brave_file(self) -> None:
        text = (
            '#define X(name) name,\n'
            '#include "brave/base/list.inc"\n#undef X\n'
            '#define Foo BraveFoo\n'
        )
        self.assertEqual(
            [('X', True), ('Foo', False)],
            DefineReach('a', 'b').defines(text, ['X(kOne)\nX(kTwo)\n']),
        )

    def test_helpers_reached_by_token_pasting(self) -> None:
        text = (
            '#define IMPL_0(...)\n#define IMPL_1(...) __VA_ARGS__\n'
            '#define IMPL_CAT(v) CAT(IMPL_, v)\n'
            '#define Foo_Bar BraveFoo_Bar\n'
        )
        self.assertEqual(
            [
                ('IMPL_0', True),
                ('IMPL_1', True),
                ('IMPL_CAT', True),
                ('Foo_Bar', False),
            ],
            DefineReach('a', 'b').defines(text),
        )

    def test_patched_words_are_from_added_lines(self) -> None:
        patch = (
            '--- a/x.cc\n+++ b/x.cc\n'
            ' context Ctx\n-removed Gone\n+  BRAVE_HOOK(Added);\n'
        )
        self.assertEqual(
            {'BRAVE_HOOK', 'Added'}, DefineReach.patched_words([patch])
        )

    def test_upstream_files(self) -> None:
        self.assertEqual(
            ['a/b.h', 'a/b.cc', 'a/b.mm'], DefineReach.upstream_files('a/b.h')
        )
        self.assertEqual(['a/b.cc'], DefineReach.upstream_files('a/b.cc'))

    def test_count_is_by_word(self) -> None:
        self.assertEqual(
            2, DefineReach.count('Foo', ['Foo FooBar', None, 'x.Foo()'])
        )


class RunTest(unittest.TestCase):
    """A Chromium repository with two tags, and a Brave override."""

    @staticmethod
    def _git(cwd: Path, *args: str) -> None:
        subprocess.run(
            ['git', '-C', str(cwd), *args],
            check=True,
            capture_output=True,
            env={
                'GIT_AUTHOR_NAME': 't',
                'GIT_AUTHOR_EMAIL': 't@t',
                'GIT_COMMITTER_NAME': 't',
                'GIT_COMMITTER_EMAIL': 't@t',
                'HOME': str(cwd),
                'PATH': '/usr/bin:/bin',
            },
        )

    @staticmethod
    def _write(path: Path, text: str) -> None:
        path.write_text(text, encoding='utf-8', newline='')

    def setUp(self) -> None:
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root)
        chromium, brave = root / 'src', root / 'brave'
        (chromium / 'a').mkdir(parents=True)
        self._git(chromium, 'init', '-q', '-b', 'main')
        self._write(chromium / 'a/b.h', 'void Old();\nvoid Same();\n')
        self._write(chromium / 'a/b.cc', 'void Old() {}\nvoid Same() {}\n')
        self._git(chromium, 'add', '.')
        self._git(chromium, 'commit', '-q', '-m', 'old')
        self._git(chromium, 'tag', 'old')
        self._write(chromium / 'a/b.h', 'void New();\nvoid Same();\n')
        self._write(chromium / 'a/b.cc', 'void New() {}\nvoid Same() {}\n')
        self._git(chromium, 'commit', '-q', '-am', 'new')
        self._git(chromium, 'tag', 'new')
        (brave / 'chromium_src/a').mkdir(parents=True)
        self._write(
            brave / 'chromium_src/a/b.h',
            '#define Old BraveOld\n#define Same BraveSame\n'
            '#include <a/b.h>\n#undef Old\n#undef Same\n',
        )
        self._write(brave / 'chromium_src/a/brave_only.h', '#define X Y\n')
        for name, path in (('chromium', chromium), ('brave', brave)):
            self.enterContext(
                mock.patch.object(
                    define_reach.repository, name, SimpleNamespace(root=path)
                )
            )

    def test_run(self) -> None:
        self.assertEqual(
            [Reach('a/b.h', 'Old', 2, 0)], DefineReach('old', 'new').run()
        )

    def test_skips_dead_tokens_a_patch_adds(self) -> None:
        brave = define_reach.repository.brave.root
        self._write(
            brave / 'chromium_src/a/b.cc',
            '#define HOOK Brave()\n#define Gone BraveGone\n'
            '#include <a/b.cc>\n#undef HOOK\n#undef Gone\n',
        )
        (brave / 'patches').mkdir()
        self._write(
            brave / 'patches/a-b.cc.patch',
            '--- a/a/b.cc\n+++ b/a/b.cc\n+HOOK;\n',
        )
        self.assertEqual(
            [Reach('a/b.cc', 'Gone', 0, 0), Reach('a/b.h', 'Old', 2, 0)],
            DefineReach('old', 'new').run(),
        )


if __name__ == '__main__':
    unittest.main()
