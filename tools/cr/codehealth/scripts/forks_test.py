#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for forks.py."""

from __future__ import annotations

import contextlib
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import forks
from forks import Fork, ForkIndex, ForkReport

_UPSTREAM = '\n'.join(
    [
        'chrome/browser/ui/views/toolbar/app_menu.cc',
        'services/network/cors/cors_url_loader.cc',
        'chrome/browser/foo.cc',
        'chrome/browser/bar.cc',
        'a/dup.cc',
        'b/dup.cc',
        ForkIndex.KNOWN['ios/browser/api/history/history_driver_ios.mm'],
    ]
)

_GREP = '\n'.join(
    [
        'browser/x.cc:10:// Copied from '
        'chrome/browser/ui/views/toolbar/app_menu.cc',
        'browser/x.cc-11-int x;',
        '--',
        'browser/y.cc:20:// Kept in sync with CorsURLLoader::FollowRedirect().',
        'browser/y.cc-21-',
        '--',
        'browser/z.cc:30:// Adapted from',
        'browser/z.cc-31-// //chrome/browser/foo.cc',
        '--',
        'browser/w.cc:40:// Copied from dup.cc, ambiguous',
        'browser/w.cc-41-// and from nowhere/else.cc',
        '--',
        'browser/v-v.cc:5:// taken from Trackr',
    ]
)


class ForkIndexTest(unittest.TestCase):
    def setUp(self) -> None:
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        (root / 'chromium_src/chrome/browser').mkdir(parents=True)
        (root / 'chromium_src/chrome/browser/foo.cc').write_text(
            '#include <chrome/browser/foo.cc>\n', encoding='utf-8', newline=''
        )
        (root / 'chromium_src/chrome/browser/bar.cc').write_text(
            'int bar;\n', encoding='utf-8', newline=''
        )
        brave_git = {
            'grep': _GREP,
            'ls-files': 'chromium_src/chrome/browser/foo.cc\n'
            'chromium_src/chrome/browser/bar.cc\n'
            'chromium_src/chrome/browser/gone.cc\n',
        }
        patches = (
            mock.patch.object(
                forks.repository,
                'chromium',
                SimpleNamespace(run_git=lambda *a, **k: _UPSTREAM),
            ),
            mock.patch.object(
                forks.repository,
                'brave',
                SimpleNamespace(
                    root=root, run_git=lambda *a, **k: brave_git[a[0]]
                ),
            ),
        )
        for patcher in patches:
            self.enterContext(patcher)

    def test_forks(self) -> None:
        found = ForkIndex('new').forks()
        self.assertEqual(
            {
                Fork(
                    'browser/x.cc:10',
                    'chrome/browser/ui/views/toolbar/app_menu.cc',
                    'comment',
                ),
                Fork(
                    'browser/y.cc:20',
                    'services/network/cors/cors_url_loader.cc',
                    'comment',
                ),
                Fork('browser/z.cc:30', 'chrome/browser/foo.cc', 'comment'),
                Fork(
                    'chromium_src/chrome/browser/bar.cc:1',
                    'chrome/browser/bar.cc',
                    'replacement',
                ),
            },
            {f for f in found if f.kind != 'known'},
        )
        self.assertEqual(
            len(ForkIndex.KNOWN), sum(f.kind == 'known' for f in found)
        )
        verbatim = {f.brave: f.verbatim for f in found if f.kind == 'comment'}
        self.assertEqual(
            {
                'browser/x.cc:10': True,
                'browser/y.cc:20': True,
                'browser/z.cc:30': False,
            },
            verbatim,
        )


_BRAVE_COPY = '''namespace {

// Copied from extensions/common/url_pattern.cc
std::string_view Canonicalize(std::string_view host) {
  if (host.ends_with('.')) {
    host.remove_suffix(1);
  }
  return host;
}

void F() {
  // Kept in sync with Foo::Bar().
  if (x && (!x->is_valid() ||
            y)) {
    return;
  }
}

}  // namespace
'''

_UPSTREAM_ORIGINAL = '''std::string_view Canonicalize(std::string_view host);

std::string_view Canonicalize(std::string_view host) {
  return base::TrimString(host, ".", base::TRIM_TRAILING);
}
'''


class FunctionCopyTest(unittest.TestCase):
    def test_copied_function_is_the_definition_after_the_comment(self) -> None:
        name, body = forks.copied_function(_BRAVE_COPY, 3)
        self.assertEqual('Canonicalize', name)
        self.assertTrue(body.startswith('std::string_view Canonicalize('))
        self.assertTrue(body.endswith('return host;\n}'))

    def test_a_copied_snippet_has_no_function(self) -> None:
        self.assertIsNone(forks.copied_function(_BRAVE_COPY, 12))

    def test_function_body_skips_declarations(self) -> None:
        self.assertEqual(
            'std::string_view Canonicalize(std::string_view host) {\n'
            '  return base::TrimString(host, ".", base::TRIM_TRAILING);\n}',
            forks.function_body(_UPSTREAM_ORIGINAL, 'Canonicalize'),
        )

    def test_body_diff_ignores_comments_and_whitespace(self) -> None:
        self.assertEqual(
            [],
            forks.body_diff(
                'int f() { return 1; }', 'int f() {\n  // x\n  return 1;\n}'
            ),
        )
        self.assertEqual(
            ['@@ -1 +1 @@', '-int f() { return 1; }', '+int f() { return 2; }'],
            forks.body_diff('int f() { return 1; }', 'int f() { return 2; }'),
        )


class ForkReportTest(unittest.TestCase):
    def _diverged_report(
        self, before: str = _UPSTREAM_ORIGINAL, debt: bool = False
    ) -> list[str]:
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        (root / 'x.cc').write_text(_BRAVE_COPY, encoding='utf-8', newline='')
        copied = Fork('x.cc:3', 'a/orig.cc', 'comment', verbatim=True)
        adapted = Fork('x.cc:3', 'a/orig.cc', 'comment')

        def chromium_git(*args, **_):
            if args[0] != 'show':
                return ''
            return before if args[1].startswith('old:') else _UPSTREAM_ORIGINAL

        with (
            mock.patch.object(
                ForkIndex, 'forks', lambda self: [copied, adapted]
            ),
            mock.patch.object(
                forks.repository,
                'chromium',
                SimpleNamespace(run_git=chromium_git),
            ),
            mock.patch.object(
                forks.repository, 'brave', SimpleNamespace(root=root)
            ),
        ):
            return ForkReport('old', 'new', debt).lines()

    def test_reports_copies_of_originals_the_range_changed(self) -> None:
        before = _UPSTREAM_ORIGINAL.replace('TRIM_TRAILING', 'TRIM_ALL')
        lines = self._diverged_report(before)
        self.assertIn(
            '1 copied functions differ from their original at new, which the '
            'range changed:',
            lines,
        )
        self.assertIn('x.cc:3 Canonicalize() <- a/orig.cc', lines)
        self.assertIn(
            '  +  return base::TrimString(host, ".", base::TRIM_TRAILING);',
            lines,
        )
        self.assertIn(
            "0 differ from an original the range didn't change", lines
        )

    def test_counts_copies_of_unchanged_originals(self) -> None:
        lines = self._diverged_report()
        self.assertIn(
            "1 differ from an original the range didn't change (list them "
            'with --debt)',
            lines,
        )
        self.assertFalse(any('Canonicalize()' in line for line in lines))
        lines = self._diverged_report(debt=True)
        self.assertIn('x.cc:3 Canonicalize() <- a/orig.cc', lines)

    def test_lines(self) -> None:
        fork = Fork('browser/x.cc:10', 'a/orig.cc', 'comment')
        log = '@@@ 111 Fix orig\n\na/orig.cc\nother.cc\n@@@ 222 Other\n\nx.cc\n'
        with (
            mock.patch.object(ForkIndex, 'forks', lambda self: [fork]),
            mock.patch.object(
                forks.repository,
                'chromium',
                SimpleNamespace(run_git=lambda *a, **k: log),
            ),
            mock.patch.object(ForkReport, '_reimplemented', lambda self: []),
        ):
            lines = ForkReport('old', 'new').lines()
        self.assertEqual(
            [
                '1 forks indexed, 1 originals changed in the range.',
                '',
                'a/orig.cc',
                '  <- browser/x.cc:10 (comment)',
                '  111 Fix orig',
                '',
                '0 functions Brave renames to `_ChromiumImpl` and never calls, '
                'whose original the range changed:',
            ],
            lines,
        )

    def test_reimplemented_functions_of_changed_originals(self) -> None:
        overrides = {
            # Replaces `Run()`, and wraps `Stop()`.
            'chromium_src/a/b.cc': (
                '#define Run Run_ChromiumImpl\n'
                '#define Stop Stop_ChromiumImpl\n'
                '#include <a/b.cc>\n'
                '#undef Run\n#undef Stop\n'
                'void B::Run() {}\n'
                'void B::Stop() { Stop_ChromiumImpl(); }\n'
            ),
            # The header renames `Run()` too: listed once.
            'chromium_src/a/b.h': '#define Run Run_ChromiumImpl\n',
            'chromium_src/c/d.cc': '#define Go Go_ChromiumImpl\n',
        }
        upstream = {
            'old': 'void B::Run() {\n  A();\n}\nvoid B::Stop() {\n  A();\n}\n',
            'new': 'void B::Run() {\n  B();\n}\nvoid B::Stop() {\n  B();\n}\n',
        }
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for path, text in overrides.items():
                (root / path).parent.mkdir(parents=True, exist_ok=True)
                (root / path).write_text(text, encoding='utf-8')
            for args in (['init', '-q'], ['add', '.']):
                subprocess.run(['git', '-C', tmp, *args], check=True)
            diffs = []

            def chromium_git(*args, **_):
                if args[0] == 'diff':
                    diffs.append(args)
                    # `c/d.cc` didn't change, so it isn't read.
                    return 'a/b.cc\n'
                return upstream[args[1].split(':')[0]]

            with (
                mock.patch.object(
                    forks.repository, 'brave', SimpleNamespace(root=root)
                ),
                mock.patch.object(
                    forks.repository,
                    'chromium',
                    SimpleNamespace(run_git=chromium_git),
                ),
            ):
                found = ForkReport('old', 'new')._reimplemented()
        self.assertEqual(
            [
                (
                    'chromium_src/a/b.cc, chromium_src/a/b.h',
                    'Run',
                    'a/b.cc',
                    ['@@ -2 +2 @@', '-  A();', '+  B();'],
                )
            ],
            found,
        )
        self.assertEqual(1, len(diffs))
        self.assertIn('c/d.cc', diffs[0])


if __name__ == '__main__':
    unittest.main()
