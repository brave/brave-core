#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for python_reach.py."""

from __future__ import annotations

import contextlib
from pathlib import Path
import re
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import python_reach
from python_reach import (
    Anchor,
    Literal,
    Override,
    PythonReach,
    PythonSource,
    module_anchors,
    range_text_anchors,
)

_OVERRIDE = '''\
import override_utils

PATTERN = (r'wrapper(\\.exe)', 'other')
assert PATTERN[0] in _WRAPPER_RE.pattern


@override_utils.override_function(globals())
def Process(original_function, db, args=None):
    """Not a literal."""
    return original_function(db, args, encoding='utf-8')


@override_utils.override_method(Config, name='load')
def _load(self, original_method):
    return original_method(self)


@override_utils.override_function(argparse.ArgumentParser)
def parse_args(self, original_method):
    return original_method(self)
'''


class PythonSourceTest(unittest.TestCase):
    def test_declarations(self) -> None:
        source = PythonSource(
            'def f(\n    a,\n    b=1,\n):\n    def inner(): pass\n'
            'class C:\n    def m(self, x): pass\n'
            '    class Nested:\n        def n(self): pass\n'
        )
        self.assertEqual({'f': '(a, b=1)'}, source.functions)
        self.assertEqual({('C', 'm'): '(self, x)'}, source.methods)
        self.assertEqual({'C'}, source.classes)

    def test_overrides(self) -> None:
        self.assertEqual(
            [
                Override('globals()', 'Process'),
                Override('Config', 'load'),
                Override('argparse.ArgumentParser', 'parse_args'),
            ],
            PythonSource(_OVERRIDE).overrides,
        )

    def test_literals_leave_out_docstrings_and_keyword_arguments(self) -> None:
        self.assertEqual(
            [
                Literal('wrapper(\\.exe)', 'wrapper(\\.exe)'),
                Literal('other', 'other'),
            ],
            PythonSource(_OVERRIDE).literals,
        )

    def test_literals_keep_their_escaped_text(self) -> None:
        self.assertEqual(
            [Literal('a\\.b', 'a\\\\.b')],
            PythonSource("x = 'a\\\\.b'\ny = f'{x}.y'\nz = 'ab'\n").literals,
        )

    def test_globals_read(self) -> None:
        names = PythonSource(_OVERRIDE).globals_read()
        self.assertLessEqual(
            {'_WRAPPER_RE', 'Config', 'PATTERN', 'override_utils'}, names
        )
        self.assertNotIn('db', names)

    def test_module_names(self) -> None:
        self.assertEqual(
            {'os', 'X', 'f', 'C'},
            PythonSource(
                'import os\nX = 1\ndef f(): y = 2\nclass C: pass\n'
            ).module_names(),
        )
        self.assertIsNone(PythonSource('print "py2"\n').module_names())


class ModuleAnchorsTest(unittest.TestCase):
    _OLD = (
        "_WRAPPER_RE = re.compile(r'(wrapper(\\.exe))')\n"
        'def Process(db, args=None): pass\n'
        'class Config:\n    def load(self): pass\n'
    )

    def _anchors(self, new: str) -> list[Anchor]:
        return module_anchors(
            'a/b.py',
            PythonSource(_OVERRIDE),
            PythonSource(self._OLD),
            PythonSource(new),
        )

    def test_nothing_dropped(self) -> None:
        self.assertEqual([], self._anchors(self._OLD + 'def New(): pass\n'))

    def test_dropped_text(self) -> None:
        new = self._OLD.replace('wrapper(\\.exe)', 'other_wrapper')
        self.assertEqual(
            [Anchor('a/b.py', 'text', "'wrapper(\\\\.exe)'", '1', '0')],
            self._anchors(new),
        )

    def test_dropped_names(self) -> None:
        new = (
            "_WRAPPER = re.compile(r'(wrapper(\\.exe))')\n"
            'class Config:\n    def reload(self): pass\n'
        )
        self.assertEqual(
            [
                Anchor('a/b.py', 'name', 'Process', 'defined', 'gone'),
                Anchor('a/b.py', 'name', 'Config.load', 'defined', 'gone'),
                Anchor('a/b.py', 'name', '_WRAPPER_RE', 'defined', 'gone'),
            ],
            self._anchors(new),
        )

    def test_changed_params(self) -> None:
        new = self._OLD.replace('(db, args=None)', '(db, args, target)')
        self.assertEqual(
            [
                Anchor(
                    'a/b.py',
                    'params',
                    'Process',
                    '(db, args=None)',
                    '(db, args, target)',
                )
            ],
            self._anchors(new),
        )


class RangeTextAnchorsTest(unittest.TestCase):
    _LITERAL = Literal('void Hook();', 'void Hook();')

    def test_removed_everywhere(self) -> None:
        self.assertEqual(
            [
                Anchor(
                    'o.py',
                    'text',
                    "'void Hook();'",
                    '2',
                    '0',
                    ('a.pidl', 'b.h'),
                )
            ],
            range_text_anchors(
                'o.py',
                [self._LITERAL],
                {
                    'a.pidl': ('void Hook();\n', None),
                    'b.h': ('void Hook();\n', 'void Other();\n'),
                },
            ),
        )

    def test_moved(self) -> None:
        self.assertEqual(
            [],
            range_text_anchors(
                'o.py',
                [self._LITERAL],
                {
                    'a.pidl': ('void Hook();\n', ''),
                    'b.h': (None, 'void Hook();'),
                },
            ),
        )


class EreEscapeTest(unittest.TestCase):
    def test_escapes(self) -> None:
        text = 'a(\\.b)|[c]*+?{1}^$'
        self.assertIsNotNone(re.fullmatch(python_reach.ere_escape(text), text))


class RunTest(unittest.TestCase):
    """A Chromium repository with a sub-repository, two tags, and Brave
    overrides inlined by patches."""

    @staticmethod
    def _git(cwd: Path, *args: str) -> str:
        return subprocess.run(
            ['git', '-C', str(cwd), *args],
            check=True,
            capture_output=True,
            text=True,
            env={
                'GIT_AUTHOR_NAME': 't',
                'GIT_AUTHOR_EMAIL': 't@t',
                'GIT_COMMITTER_NAME': 't',
                'GIT_COMMITTER_EMAIL': 't@t',
                'HOME': str(cwd),
                'PATH': '/usr/bin:/bin',
            },
        ).stdout.strip()

    @staticmethod
    def _write(path: Path, text: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding='utf-8', newline='')

    def _commit(self, tag: str, sub_text: str, files: dict[str, str]) -> None:
        chromium = self._chromium
        sub = chromium / 'third_party/sub'
        self._write(sub / 'x/y.py', sub_text)
        self._git(sub, 'commit', '-q', '--allow-empty', '-am', tag)
        sha = self._git(sub, 'rev-parse', 'HEAD')
        for path, text in files.items():
            self._write(chromium / path, text)
        self._git(chromium, 'add', 'tools', 'third_party/blink')
        self._git(
            chromium,
            'update-index',
            '--add',
            '--cacheinfo',
            f'160000,{sha},third_party/sub',
        )
        self._git(chromium, 'commit', '-q', '-m', tag)
        self._git(chromium, 'tag', tag)

    def setUp(self) -> None:
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        self._chromium = chromium = root / 'src'
        brave = root / 'brave'
        chromium.mkdir()
        self._git(chromium, 'init', '-q', '-b', 'main')
        sub = chromium / 'third_party/sub'
        sub.mkdir(parents=True)
        self._git(sub, 'init', '-q', '-b', 'main')
        self._write(sub / 'x/y.py', 'def Sub(): pass\n')
        self._git(sub, 'add', '.')
        self._git(sub, 'commit', '-q', '-m', 'init')
        self._commit(
            'old',
            'def Sub(): pass\n',
            {
                'tools/a/compile.py': (
                    "_RE = r'wrapper(\\.exe)'\ndef Run(x): pass\n"
                ),
                'tools/a/same.py': 'def Same(): pass\n',
                'third_party/blink/core/probes.pidl': (
                    'void Hook([Keep] Frame*);\n'
                ),
                'third_party/blink/other.py': 'def Gen(): pass\n',
            },
        )
        self._commit(
            'new',
            'def Renamed(): pass\n',
            {
                'tools/a/compile.py': "_RE = r'wrapper'\ndef Run(x): pass\n",
                'third_party/blink/core/probes.pidl': 'void Other();\n',
            },
        )
        overrides = {
            'tools/a/compile.py': (
                "assert r'wrapper(\\.exe)' in _RE\n"
                '@override_utils.override_function(globals())\n'
                'def Run(original_function, x): pass\n'
            ),
            'tools/a/same.py': (
                '@override_utils.override_function(globals())\n'
                'def Gone(original_function): pass\n'
            ),
            'third_party/blink/other.py': (
                "_HOOK = 'void Hook([Keep] Frame*);'\n"
            ),
            'third_party/sub/x/y.py': (
                '@override_utils.override_function(globals())\n'
                'def Sub(original_function): pass\n'
            ),
            'PRESUBMIT.py': "assert 'never inlined' in x\n",
        }
        for path, text in overrides.items():
            self._write(brave / 'chromium_src' / path, text)
        for patch, path in (
            ('tools-a-compile.py.patch', 'tools/a/compile.py'),
            ('tools-a-same.py.patch', 'tools/a/same.py'),
            ('third_party-blink-other.py.patch', 'third_party/blink/other.py'),
            ('third_party/sub/x-y.py.patch', 'x/y.py'),
        ):
            self._write(
                brave / 'patches' / patch,
                f'diff --git a/{path} b/{path}\n'
                '+inline_chromium_src_override(globals(), locals())\n',
            )
        (brave / 'rewrite').mkdir()
        for name, path in (('chromium', chromium), ('brave', brave)):
            self.enterContext(
                mock.patch.object(
                    python_reach.repository, name, SimpleNamespace(root=path)
                )
            )

    def test_run(self) -> None:
        self.assertEqual(
            [
                # Upstream's `same.py` didn't change, so its dead target isn't
                # this range's.
                Anchor(
                    'third_party/sub/x/y.py', 'name', 'Sub', 'defined', 'gone'
                ),
                Anchor(
                    'third_party/blink/other.py',
                    'text',
                    "'void Hook([Keep] Frame*);'",
                    '1',
                    '0',
                    ('third_party/blink/core/probes.pidl',),
                ),
                Anchor(
                    'tools/a/compile.py',
                    'text',
                    "'wrapper(\\\\.exe)'",
                    '1',
                    '0',
                ),
            ],
            PythonReach('old', 'new').run(),
        )

    def test_unchanged_sub_repository_is_skipped(self) -> None:
        self.assertEqual([], list(PythonReach('new', 'new').run()))


if __name__ == '__main__':
    unittest.main()
