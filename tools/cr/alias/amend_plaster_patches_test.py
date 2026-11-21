#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Integration tests for amend_plaster_patches.py, running cmd.py as a
subprocess against a FakeChromiumRepo."""

import unittest

import _boot  # noqa: F401
from cmd_test import _Sandbox, _git

FOO = 'chrome/browser/foo.cc'
FOO_PLASTER = f'rewrite/{FOO}.yaml'
FOO_PATCH = 'patches/chrome-browser-foo.cc.patch'
FOO_PATCHINFO = 'patches/chrome-browser-foo.cc.patchinfo'


def _plaster_body(pattern: str) -> str:
    return (
        'substitutions:\n'
        '  - description: Rewrite the thing brave owns\n'
        '    regex:\n'
        f'      re_pattern: {pattern}\n'
        '      replace: BraveThing\n'
    )


class TestAmendPlasterPatches(unittest.TestCase):
    def setUp(self) -> None:
        self._sandbox = self.enterContext(_Sandbox())
        repo = self._sandbox.repo
        repo.write_and_stage_file(
            FOO,
            '// foo.cc\n  ChromeThing thing;\n// end of foo.cc\n',
            repo.chromium,
        )
        repo.commit('Add foo.cc', repo.chromium)

    def _git_out(self, *args: str) -> str:
        return _git(self._sandbox.root, *args).stdout.strip()

    def _commit_plaster(self, pattern: str) -> str:
        self._sandbox.repo.write_and_stage_file(
            FOO_PLASTER, _plaster_body(pattern), self._sandbox.root
        )
        _git(self._sandbox.root, 'commit', '-m', 'Add the foo.cc plaster')
        return self._git_out('rev-parse', 'HEAD')

    def test_amends_the_patches_of_an_added_plaster(self) -> None:
        parent = self._git_out('rev-parse', 'HEAD')
        self._commit_plaster('ChromeThing')

        result = self._sandbox.run_gc(['amend-plaster-patches'])

        self.assertEqual(result.returncode, 0, msg=result.stderr)
        self.assertEqual(self._git_out('rev-parse', 'HEAD^'), parent)
        self.assertEqual(
            self._git_out('log', '-1', '--format=%s'),
            'Add the foo.cc plaster',
        )
        self.assertEqual(
            self._git_out(
                'diff-tree', '--no-commit-id', '--name-only', '-r', 'HEAD'
            ).split(),
            [FOO_PATCH, FOO_PATCHINFO, FOO_PLASTER],
        )
        self.assertIn(
            '+  BraveThing thing;', self._git_out('show', f'HEAD:{FOO_PATCH}')
        )
        self.assertEqual(self._git_out('status', '--porcelain'), '')

    def test_leaves_a_commit_without_plasters_alone(self) -> None:
        self._commit_plaster('ChromeThing')
        self._sandbox.run_gc(['amend-plaster-patches'])
        (self._sandbox.root / 'other.txt').write_text('x\n', encoding='utf-8')
        _git(self._sandbox.root, 'add', 'other.txt')
        _git(self._sandbox.root, 'commit', '-m', 'Add other.txt')
        head = self._git_out('rev-parse', 'HEAD')

        result = self._sandbox.run_gc(['amend-plaster-patches'])

        self.assertEqual(result.returncode, 0, msg=result.stderr)
        self.assertEqual(self._git_out('rev-parse', 'HEAD'), head)

    def test_a_plaster_that_does_not_apply_is_an_error(self) -> None:
        head = self._commit_plaster('NoSuchThing')

        result = self._sandbox.run_gc(['amend-plaster-patches'])

        self.assertEqual(result.returncode, 1)
        self.assertIn(FOO_PLASTER, result.stderr)
        self.assertEqual(self._git_out('rev-parse', 'HEAD'), head)


if __name__ == '__main__':
    unittest.main()
