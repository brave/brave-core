#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `conflict_resolution`.

Layers:

* Pure helpers -- `_SingleConflict`, `_changed_lines`, and the version bump
  choices, with no git involved.

* `ConflictedStep` -- what is read from a rebase stopped by a real git conflict.

* Rules -- each one run through `ConflictedStep.resolve`, over a rebase that
  git stopped on the conflict the rule is for, built in a `FakeChromiumRepo`.

* `CrBranchRebaseTest` -- a `brockit rebase` that goes through the conflicts a
  `cr` branch always has on its own.
"""

from __future__ import annotations

import os
import subprocess
import unittest
from unittest.mock import patch

import brockit
import conflict_resolution
from conflict_resolution import Resolution, Rule, Stage
import plaster
import repository
import versioning
from test.fake_chromium_repo import DEFAULT_BRANCH, FakeChromiumRepo
from test.fake_terminal import ConsoleCapture

# The milestone tracker, and a file next to it the tracker rule leaves alone.
TRACKER = 'tools/cr/M158_codehealth.md'
NOT_A_TRACKER = 'tools/cr/README.md'

# A Chromium source brave patches with a plaster, its plaster, and its patch.
FOO = 'chrome/browser/foo.cc'
FOO_PLASTER = f'rewrite/{FOO}.yaml'
FOO_PATCH = 'patches/chrome-browser-foo.cc.patch'

# A raw patch: one with no plaster behind it.
RAW_PATCH = 'patches/chrome-browser-bar.cc.patch'

# The subject of a version commit, and of any other commit.
VERSION_SUBJECT = 'Update from Chromium 157.0.8103.15 to Chromium 158.0.8127.2.'
OTHER_SUBJECT = '[cr158] Some change'

# The Chromium version brave and the Chromium checkout are on, in the tests
# regenerating patches from their plaster.
CHROMIUM_VERSION = '158.0.8127.2'

# Pinslist timestamps, in increasing order.
OLDER_TIMESTAMP = 1793219453
NEWER_TIMESTAMP = 1793260000


def _plaster_body(replacement: str) -> str:
    """A plaster for `FOO`, rewriting its upstream symbol to `replacement`."""
    return (
        'substitutions:\n'
        '  - description: Rewrite the thing brave owns\n'
        '    regex:\n'
        '      re_pattern: ChromeThing\n'
        f'      replace: {replacement}\n'
    )


def _raw_patch(*, index: str, change: str, context: str) -> str:
    """A raw patch rewriting a symbol to `change`, next to a `context` line."""
    return (
        'diff --git a/chrome/browser/bar.cc b/chrome/browser/bar.cc\n'
        f'index {index} 100644\n'
        '--- a/chrome/browser/bar.cc\n'
        '+++ b/chrome/browser/bar.cc\n'
        '@@ -1,4 +1,4 @@ namespace {\n'
        ' // bar.cc\n'
        '-  ChromeThing thing;\n'
        f'+  {change} thing;\n'
        f' {context}\n'
        ' // end of bar.cc\n'
    )


def _package_json(*, version: str = '1.0.0', tag: str) -> str:
    """A `package.json` with brave's `version` and the Chromium `tag`.

    The two are far enough apart for git to merge changes to them separately.
    """
    return (
        '{\n'
        '  "name": "brave-core",\n'
        f'  "version": "{version}",\n'
        '  "description": "Brave browser",\n'
        '  "main": "index.js",\n'
        '  "type": "module",\n'
        '  "config": {\n'
        '    "projects": {\n'
        '      "chrome": {\n'
        '        "dir": "src",\n'
        f'        "tag": "{tag}",\n'
        '        "repository": {\n'
        '          "url": "https://github.com/chromium/chromium"\n'
        '        }\n'
        '      }\n'
        '    }\n'
        '  }\n'
        '}\n'
    )


def _pinslist(*, timestamp: int, root: str = 'BraveRootCA') -> str:
    """A pinslist file stamped with `timestamp`, listing the `root` CA.

    The root is far enough from the timestamp for git to merge changes to the
    two separately.
    """
    return (
        'constexpr std::string_view kBraveCerts = R"brave_certs(\n'
        f'# Last updated: stamped at {timestamp}\n'
        'PinsListTimestamp\n'
        f'{timestamp}\n'
        '\n'
        '# =====BEGIN BRAVE ROOTS ASC=====\n'
        '#From https://example.com/repository/\n'
        '#\n'
        '#\n'
        f'{root}\n'
        '-----BEGIN CERTIFICATE-----\n'
        'MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n'
        '-----END CERTIFICATE-----\n'
        ')brave_certs";\n'
    )


class SingleConflictTest(unittest.TestCase):
    """`_SingleConflict` splits a file around its only conflicting hunk."""

    def test_splits_around_the_hunk(self):
        conflict = conflict_resolution._SingleConflict.parse(
            'before\n'
            '<<<<<<< HEAD\n'
            'rebased\n'
            '=======\n'
            'commit\n'
            '>>>>>>> abc123 (Some change)\n'
            'after\n'
        )

        self.assertEqual(conflict.before, 'before\n')
        self.assertEqual(conflict.rebased, 'rebased\n')
        self.assertEqual(conflict.commit, 'commit\n')
        self.assertEqual(conflict.after, 'after\n')
        self.assertEqual(
            conflict.resolve(conflict.commit), 'before\ncommit\nafter\n'
        )

    def test_leaves_the_ancestor_out_of_a_diff3_hunk(self):
        conflict = conflict_resolution._SingleConflict.parse(
            '<<<<<<< HEAD\n'
            'rebased\n'
            '||||||| parent of abc123 (Some change)\n'
            'ancestor\n'
            '=======\n'
            'commit\n'
            '>>>>>>> abc123 (Some change)\n'
        )

        self.assertEqual(conflict.rebased, 'rebased\n')
        self.assertEqual(conflict.commit, 'commit\n')
        self.assertEqual(conflict.resolve(conflict.rebased), 'rebased\n')

    def test_keeps_multiline_sides(self):
        conflict = conflict_resolution._SingleConflict.parse(
            '<<<<<<< HEAD\nr1\nr2\n=======\nc1\nc2\nc3\n>>>>>>> abc123\n'
        )

        self.assertEqual(conflict.rebased, 'r1\nr2\n')
        self.assertEqual(conflict.commit, 'c1\nc2\nc3\n')

    def test_rejects_a_file_without_conflicts(self):
        self.assertIsNone(
            conflict_resolution._SingleConflict.parse('no conflicts\n')
        )

    def test_rejects_a_file_with_two_hunks(self):
        hunk = '<<<<<<< HEAD\nrebased\n=======\ncommit\n>>>>>>> abc123\n'
        self.assertIsNone(
            conflict_resolution._SingleConflict.parse(f'{hunk}between\n{hunk}')
        )


class ChangedLinesTest(unittest.TestCase):
    """`_changed_lines` keeps only what a patch adds and removes."""

    def test_leaves_out_headers_and_context(self):
        self.assertEqual(
            conflict_resolution._changed_lines(
                _raw_patch(
                    index='1111111..2222222',
                    change='BraveThing',
                    context='// context',
                )
            ),
            ['-  ChromeThing thing;', '+  BraveThing thing;'],
        )

    def test_covers_every_file_in_the_patch(self):
        first = _raw_patch(index='1..2', change='First', context='// a')
        second = _raw_patch(index='3..4', change='Second', context='// b')

        self.assertEqual(
            conflict_resolution._changed_lines(first + second),
            [
                '-  ChromeThing thing;',
                '+  First thing;',
                '-  ChromeThing thing;',
                '+  Second thing;',
            ],
        )

    def test_is_blind_to_context_and_index_changes(self):
        self.assertEqual(
            conflict_resolution._changed_lines(
                _raw_patch(index='1..2', change='Brave', context='// old')
            ),
            conflict_resolution._changed_lines(
                _raw_patch(index='3..4', change='Brave', context='// new')
            ),
        )


class VersionBumpChoiceTest(unittest.TestCase):
    """The side each file of the version commit settles on."""

    @staticmethod
    def _conflict(rebased: str, commit: str):
        return conflict_resolution._SingleConflict(
            before='', after='', rebased=rebased, commit=commit
        )

    def test_chrome_tag_takes_the_commit_when_newer(self):
        conflict = self._conflict(
            '        "tag": "157.0.8103.30",\n',
            '        "tag": "158.0.8127.2",\n',
        )
        self.assertEqual(
            conflict_resolution._commit_chrome_tag(conflict), conflict.commit
        )

    def test_chrome_tag_is_left_when_the_commit_is_not_newer(self):
        for commit_tag in ('157.0.8103.30', '157.0.8103.2'):
            with self.subTest(commit_tag=commit_tag):
                conflict = self._conflict(
                    '        "tag": "157.0.8103.30",\n',
                    f'        "tag": "{commit_tag}",\n',
                )
                self.assertIsNone(
                    conflict_resolution._commit_chrome_tag(conflict)
                )

    def test_chrome_tag_is_left_when_a_side_is_more_than_the_tag(self):
        conflict = self._conflict(
            '        "dir": "src2",\n        "tag": "157.0.8103.30",\n',
            '        "dir": "src",\n        "tag": "158.0.8127.2",\n',
        )
        self.assertIsNone(conflict_resolution._commit_chrome_tag(conflict))

    def test_pinslist_takes_the_newer_timestamp(self):
        older = f'# Last updated: then\nPinsListTimestamp\n{OLDER_TIMESTAMP}\n'
        newer = f'# Last updated: now\nPinsListTimestamp\n{NEWER_TIMESTAMP}\n'
        for rebased, commit in ((older, newer), (newer, older)):
            with self.subTest(rebased=rebased):
                self.assertEqual(
                    conflict_resolution._newer_pinslist_timestamp(
                        self._conflict(rebased, commit)
                    ),
                    newer,
                )

    def test_pinslist_is_left_when_a_side_is_not_the_timestamp(self):
        conflict = self._conflict(
            f'# Last updated: then\nPinsListTimestamp\n{OLDER_TIMESTAMP}\n',
            'SomeOtherRootCA\n',
        )
        self.assertIsNone(
            conflict_resolution._newer_pinslist_timestamp(conflict)
        )


class _RebaseStopTestCase(unittest.TestCase):
    """Builds brave histories that stop a rebase on a conflict."""

    def setUp(self):
        self.repo = FakeChromiumRepo()
        self.repo.setup()
        self.addCleanup(self.repo.cleanup)
        # `Repositories.load` memoises the `.repositories.cfg` it read, which
        # is this test's own.
        plaster.Repositories._instance = None
        self.addCleanup(setattr, plaster.Repositories, '_instance', None)
        # The tests run from `tools/cr` in the fake brave repo, which git
        # removes when checking out a branch without any file under it.
        self.commit({'tools/cr/.keep': ''}, 'Keep tools/cr')

    def git(self, *args: str) -> str:
        """Runs a git command in the fake brave repo."""
        # pylint: disable=protected-access
        return self.repo._run_git_command(list(args), self.repo.brave)

    def commit(self, files: dict[str, str | None], subject: str) -> str:
        """Commits `files` to brave, where None deletes a file."""
        for path, contents in files.items():
            if contents is None:
                self.repo.delete_file(path, self.repo.brave)
            else:
                self.repo.write_and_stage_file(path, contents, self.repo.brave)
        return self.repo.commit(subject, self.repo.brave)

    def stop_rebase(
        self,
        *,
        ancestor: dict[str, str | None],
        base: dict[str, str | None],
        commit: dict[str, str | None],
        subject: str = OTHER_SUBJECT,
    ) -> conflict_resolution.ConflictedStep:
        """Rebases a commit onto a base that changed the same files.

        Both the base and the commit, with `subject`, are made on top of
        `ancestor`. Returns the step the rebase stops on.
        """
        self.commit(ancestor, 'Add the files')
        self.git('checkout', '--quiet', '-b', 'feature')
        self.commit(commit, subject)
        self.git('checkout', '--quiet', DEFAULT_BRANCH)
        self.commit(base, 'Change the files upstream')
        self.git('checkout', '--quiet', 'feature')
        with self.assertRaises(subprocess.CalledProcessError):
            self.git('rebase', DEFAULT_BRANCH)

        step = conflict_resolution.ConflictedStep.load()
        self.assertIsNotNone(step)
        return step

    def read(self, path: str) -> str:
        """The working tree contents of a brave file."""
        return (self.repo.brave / path).read_bytes().decode('utf-8')

    def conflicted(self) -> list[str]:
        """The brave files still conflicted in the index."""
        return self.git('diff', '--name-only', '--diff-filter=U').splitlines()


class ConflictedStepTest(_RebaseStopTestCase):
    """What `ConflictedStep` reads off a stopped rebase."""

    def test_load_reads_the_stop(self):
        step = self.stop_rebase(
            ancestor={'a.txt': 'ancestor\n', 'b.txt': 'ancestor\n'},
            base={'a.txt': 'base\n', 'b.txt': 'base\n'},
            commit={'a.txt': 'commit\n', 'b.txt': 'commit\n'},
        )

        # The branch only moves once the rebase finishes.
        self.assertEqual(step.commit, self.git('rev-parse', 'feature'))
        self.assertEqual(step.subject, OTHER_SUBJECT)
        self.assertEqual(step.onto, self.git('rev-parse', DEFAULT_BRANCH))
        self.assertEqual(step.orig_head, step.commit)
        self.assertEqual(step.paths, ('a.txt', 'b.txt'))

    def test_read_returns_each_stage(self):
        step = self.stop_rebase(
            ancestor={'a.txt': 'ancestor\n'},
            base={'a.txt': 'base\n'},
            commit={'a.txt': 'commit\n'},
        )

        self.assertEqual(step.read('a.txt', Stage.ANCESTOR), 'ancestor\n')
        self.assertEqual(step.read('a.txt', Stage.REBASED), 'base\n')
        self.assertEqual(step.read('a.txt', Stage.COMMIT), 'commit\n')

    def test_read_returns_none_for_the_side_that_deleted_the_file(self):
        step = self.stop_rebase(
            ancestor={'a.txt': 'ancestor\n'},
            base={'a.txt': None},
            commit={'a.txt': 'commit\n'},
        )

        self.assertIsNone(step.read('a.txt', Stage.REBASED))
        self.assertEqual(step.read('a.txt', Stage.COMMIT), 'commit\n')

    def test_load_returns_none_without_conflicts(self):
        self.assertIsNone(conflict_resolution.ConflictedStep.load())


class ResolveTest(_RebaseStopTestCase):
    """`ConflictedStep.resolve` answers what it can, and leaves the rest."""

    def test_answers_only_the_files_a_rule_knows(self):
        step = self.stop_rebase(
            ancestor={
                'a.txt': 'ancestor\n',
                RAW_PATCH: _raw_patch(
                    index='1..2', change='BraveThing', context='// ancestor'
                ),
            },
            base={
                'a.txt': 'base\n',
                RAW_PATCH: _raw_patch(
                    index='3..4', change='BraveThing', context='// base'
                ),
            },
            commit={
                'a.txt': 'commit\n',
                RAW_PATCH: _raw_patch(
                    index='5..6', change='BraveThing', context='// commit'
                ),
            },
        )
        conflicted_a = self.read('a.txt')

        resolutions = step.resolve()

        self.assertEqual(
            resolutions,
            [Resolution(path=RAW_PATCH, rule=Rule.CONTEXT_ONLY_PATCH)],
        )
        self.assertEqual(self.read('a.txt'), conflicted_a)
        # Answers are written into the working tree, but not staged.
        self.assertEqual(self.conflicted(), ['a.txt', RAW_PATCH])


class PlasterRuleTest(_RebaseStopTestCase):
    """A conflicted patch with a plaster is regenerated by its plaster."""

    def setUp(self):
        super().setUp()
        self.repo.write_and_stage_file(
            FOO,
            '// foo.cc\n  ChromeThing thing;\n// end of foo.cc\n',
            self.repo.chromium,
        )
        self.repo.commit('Add foo.cc', self.repo.chromium)
        # Patches are only regenerated against a checkout on brave's version.
        self.repo.add_tag(CHROMIUM_VERSION)
        self.repo.update_brave_version(CHROMIUM_VERSION)

    def plastered(self, replacement: str) -> dict[str, str]:
        """The plaster rewriting `FOO` to `replacement`, and its patch."""
        self.repo.write_file(
            FOO_PLASTER, _plaster_body(replacement), self.repo.brave
        )
        plaster.PlasterFile(plaster.PLASTER_FILES_PATH / f'{FOO}.yaml').apply()
        return {
            FOO_PLASTER: _plaster_body(replacement),
            FOO_PATCH: self.read(FOO_PATCH),
        }

    def test_regenerates_the_patch(self):
        ancestor = self.plastered('BraveThing')
        commit = self.plastered('BraveOtherThing')
        # The base's patch only differs in the blob hashes it records.
        base = {FOO_PATCH: ancestor[FOO_PATCH].replace('index ', 'index 0')}
        step = self.stop_rebase(ancestor=ancestor, base=base, commit=commit)

        resolutions = step.resolve()

        self.assertEqual(
            resolutions, [Resolution(path=FOO_PATCH, rule=Rule.PLASTER)]
        )
        self.assertEqual(self.read(FOO_PATCH), commit[FOO_PATCH])

    def test_leaves_the_patch_when_chromium_is_on_another_version(self):
        ancestor = self.plastered('BraveThing')
        commit = self.plastered('BraveOtherThing')
        base = {FOO_PATCH: ancestor[FOO_PATCH].replace('index ', 'index 0')}
        step = self.stop_rebase(ancestor=ancestor, base=base, commit=commit)
        conflicted_patch = self.read(FOO_PATCH)
        self.repo.add_tag('158.0.8130.1')

        with self.assertLogs(level='WARNING') as logs:
            self.assertEqual(step.resolve(), [])

        self.assertEqual(self.read(FOO_PATCH), conflicted_patch)
        self.assertEqual(len(logs.output), 1)
        self.assertIn(
            f'checked out at 158.0.8130.1, but the rebase is at '
            f'{CHROMIUM_VERSION}',
            logs.output[0],
        )

    def test_leaves_the_patch_when_its_plaster_is_conflicted_too(self):
        ancestor = self.plastered('BraveThing')
        base = self.plastered('BraveBaseThing')
        commit = self.plastered('BraveOtherThing')
        step = self.stop_rebase(ancestor=ancestor, base=base, commit=commit)
        self.assertEqual(step.paths, (FOO_PATCH, FOO_PLASTER))

        self.assertEqual(step.resolve(), [])

    def test_leaves_the_patch_when_its_plaster_fails(self):
        """The base only changed the patch's context here, but a patch with a
        plaster is only ever regenerated, never taken from the commit."""
        ancestor = self.plastered('BraveThing')
        commit = {
            # Nothing in the source matches this plaster anymore.
            FOO_PLASTER: _plaster_body('BraveThing').replace(
                'ChromeThing', 'MissingThing'
            ),
            FOO_PATCH: ancestor[FOO_PATCH].replace('index ', 'index 1'),
        }
        base = {FOO_PATCH: ancestor[FOO_PATCH].replace('index ', 'index 0')}
        step = self.stop_rebase(ancestor=ancestor, base=base, commit=commit)
        conflicted_patch = self.read(FOO_PATCH)

        self.assertEqual(step.resolve(), [])
        self.assertEqual(self.read(FOO_PATCH), conflicted_patch)


class CodeHealthTrackerRuleTest(_RebaseStopTestCase):
    """A tracker the base doesn't have is taken from the branch."""

    def test_takes_the_branch_version_of_its_own_tracker(self):
        """Rebasing the tracker's later commits without the one adding it is
        the simplest way to have a rebase stop on a tracker the base (here, the
        default branch) doesn't have."""
        self.git('checkout', '--quiet', '-b', 'feature')
        added = self.commit(
            {TRACKER: '- [ ] one\n', NOT_A_TRACKER: 'one\n'}, 'Add a tracker'
        )
        self.commit({TRACKER: '- [x] one\n', NOT_A_TRACKER: 'two\n'}, 'Tick it')
        self.commit({TRACKER: '- [x] one\n- [ ] two\n'}, 'Add an item')
        with self.assertRaises(subprocess.CalledProcessError):
            self.git('rebase', '--onto', DEFAULT_BRANCH, added)
        step = conflict_resolution.ConflictedStep.load()

        resolutions = step.resolve()

        self.assertEqual(
            resolutions,
            [Resolution(path=TRACKER, rule=Rule.CODEHEALTH_TRACKER)],
        )
        self.assertEqual(self.read(TRACKER), '- [x] one\n- [ ] two\n')
        self.assertIn(NOT_A_TRACKER, self.conflicted())

    def test_leaves_a_tracker_the_base_has(self):
        step = self.stop_rebase(
            ancestor={TRACKER: '- [ ] one\n'},
            base={TRACKER: '- [x] one\n'},
            commit={TRACKER: '- [ ] one, reworded\n'},
        )

        self.assertEqual(step.resolve(), [])


class VersionBumpRuleTest(_RebaseStopTestCase):
    """The version commit settles on the newer version bump."""

    def test_takes_the_commit_chrome_tag(self):
        step = self.stop_rebase(
            ancestor={
                versioning.PACKAGE_FILE: _package_json(tag='157.0.8103.15')
            },
            base={
                versioning.PACKAGE_FILE: _package_json(
                    version='1.0.1', tag='157.0.8103.30'
                )
            },
            commit={versioning.PACKAGE_FILE: _package_json(tag='158.0.8127.2')},
            subject=VERSION_SUBJECT,
        )

        resolutions = step.resolve()

        self.assertEqual(
            resolutions,
            [Resolution(path=versioning.PACKAGE_FILE, rule=Rule.VERSION_BUMP)],
        )
        # The base's own change merged cleanly, and is kept.
        self.assertEqual(
            self.read(versioning.PACKAGE_FILE),
            _package_json(version='1.0.1', tag='158.0.8127.2'),
        )

    def stop_on_pinslist(
        self, *, base_stamp: int, commit_stamp: int
    ) -> conflict_resolution.ConflictedStep:
        """Stops the version commit on the pinslist timestamps given.

        The base also changes the root CA, which merges cleanly.
        """
        return self.stop_rebase(
            ancestor={
                versioning.PINSLIST_TIMESTAMP_FILE: _pinslist(
                    timestamp=1793000000
                )
            },
            base={
                versioning.PINSLIST_TIMESTAMP_FILE: _pinslist(
                    timestamp=base_stamp, root='BaseRootCA'
                )
            },
            commit={
                versioning.PINSLIST_TIMESTAMP_FILE: _pinslist(
                    timestamp=commit_stamp
                )
            },
            subject=VERSION_SUBJECT,
        )

    def assert_settled_on_newer_timestamp(
        self, step: conflict_resolution.ConflictedStep
    ) -> None:
        self.assertEqual(
            step.resolve(),
            [
                Resolution(
                    path=versioning.PINSLIST_TIMESTAMP_FILE,
                    rule=Rule.VERSION_BUMP,
                )
            ],
        )
        self.assertEqual(
            self.read(versioning.PINSLIST_TIMESTAMP_FILE),
            _pinslist(timestamp=NEWER_TIMESTAMP, root='BaseRootCA'),
        )

    def test_takes_the_commit_pinslist_timestamp_when_newer(self):
        self.assert_settled_on_newer_timestamp(
            self.stop_on_pinslist(
                base_stamp=OLDER_TIMESTAMP, commit_stamp=NEWER_TIMESTAMP
            )
        )

    def test_keeps_the_base_pinslist_timestamp_when_newer(self):
        self.assert_settled_on_newer_timestamp(
            self.stop_on_pinslist(
                base_stamp=NEWER_TIMESTAMP, commit_stamp=OLDER_TIMESTAMP
            )
        )

    def test_leaves_a_file_with_another_conflict(self):
        step = self.stop_rebase(
            ancestor={
                versioning.PINSLIST_TIMESTAMP_FILE: _pinslist(
                    timestamp=1793000000
                )
            },
            base={
                versioning.PINSLIST_TIMESTAMP_FILE: _pinslist(
                    timestamp=OLDER_TIMESTAMP, root='BaseRootCA'
                )
            },
            commit={
                versioning.PINSLIST_TIMESTAMP_FILE: _pinslist(
                    timestamp=NEWER_TIMESTAMP, root='CommitRootCA'
                )
            },
            subject=VERSION_SUBJECT,
        )

        self.assertEqual(step.resolve(), [])

    def test_leaves_a_tag_the_base_bumped_further(self):
        step = self.stop_rebase(
            ancestor={
                versioning.PACKAGE_FILE: _package_json(tag='158.0.8100.1')
            },
            base={versioning.PACKAGE_FILE: _package_json(tag='158.0.8130.1')},
            commit={versioning.PACKAGE_FILE: _package_json(tag='158.0.8127.2')},
            subject=VERSION_SUBJECT,
        )

        self.assertEqual(step.resolve(), [])

    def test_leaves_the_conflicts_of_any_other_commit(self):
        step = self.stop_rebase(
            ancestor={
                versioning.PACKAGE_FILE: _package_json(tag='157.0.8103.15')
            },
            base={versioning.PACKAGE_FILE: _package_json(tag='157.0.8103.30')},
            commit={versioning.PACKAGE_FILE: _package_json(tag='158.0.8127.2')},
            subject=OTHER_SUBJECT,
        )

        self.assertEqual(step.resolve(), [])


class ContextOnlyPatchRuleTest(_RebaseStopTestCase):
    """A raw patch the base only re-anchored is taken from the commit."""

    def test_takes_the_commit_version(self):
        commit_patch = _raw_patch(
            index='5..6', change='BraveThing', context='// for the commit'
        )
        step = self.stop_rebase(
            ancestor={
                RAW_PATCH: _raw_patch(
                    index='1..2', change='BraveThing', context='// ancestor'
                )
            },
            base={
                RAW_PATCH: _raw_patch(
                    index='3..4', change='BraveThing', context='// for the base'
                )
            },
            commit={RAW_PATCH: commit_patch},
        )

        resolutions = step.resolve()

        self.assertEqual(
            resolutions,
            [Resolution(path=RAW_PATCH, rule=Rule.CONTEXT_ONLY_PATCH)],
        )
        self.assertEqual(self.read(RAW_PATCH), commit_patch)

    def test_leaves_a_patch_whose_changes_the_base_changed(self):
        step = self.stop_rebase(
            ancestor={
                RAW_PATCH: _raw_patch(
                    index='1..2', change='BraveThing', context='// ancestor'
                )
            },
            base={
                RAW_PATCH: _raw_patch(
                    index='3..4', change='BaseThing', context='// for the base'
                )
            },
            commit={
                RAW_PATCH: _raw_patch(
                    index='5..6', change='BraveThing', context='// commit'
                )
            },
        )

        self.assertEqual(step.resolve(), [])

    def test_leaves_a_patch_added_on_both_sides(self):
        step = self.stop_rebase(
            ancestor={'a.txt': 'ancestor\n'},
            base={
                RAW_PATCH: _raw_patch(
                    index='3..4', change='BraveThing', context='// for the base'
                )
            },
            commit={
                RAW_PATCH: _raw_patch(
                    index='5..6', change='BraveThing', context='// commit'
                )
            },
        )

        self.assertEqual(step.resolve(), [])

    def test_leaves_files_that_are_not_patches(self):
        step = self.stop_rebase(
            ancestor={'patches/notes.txt': 'ancestor\n'},
            base={'patches/notes.txt': 'base\n'},
            commit={'patches/notes.txt': 'commit\n'},
        )

        self.assertEqual(step.resolve(), [])


class CrBranchRebaseTest(_RebaseStopTestCase):
    """A `brockit rebase` of a `cr` branch, with the conflicts it always has."""

    def test_rebase_resolves_its_conflicts_with_fixed_answers(self):
        """The base moved on to a newer minor version, which the version commit
        conflicts with, and squashing the tracker's pinned commits reorders
        them around a fix that ticks an item in it. The rebase goes through
        on its own, without the user's editor (an editor that fails would
        stop it)."""
        tracker_subject = '[cr158] Version CodeHealth tracker.'
        fix_subject = '[cr158] Fix one'
        fork = self.commit(
            {
                versioning.PACKAGE_FILE: _package_json(tag='157.0.8103.15'),
                versioning.PINSLIST_TIMESTAMP_FILE: _pinslist(
                    timestamp=1793000000
                ),
            },
            'Add the files',
        )
        self.git('checkout', '--quiet', '-b', 'feature')
        self.commit(
            {
                versioning.PACKAGE_FILE: _package_json(tag='158.0.8127.2'),
                versioning.PINSLIST_TIMESTAMP_FILE: _pinslist(
                    timestamp=OLDER_TIMESTAMP
                ),
            },
            VERSION_SUBJECT,
        )
        self.commit({TRACKER: '- [ ] one\n- [ ] two\n'}, tracker_subject)
        self.commit(
            {TRACKER: '- [x] one\n- [ ] two\n', 'fix.txt': 'fix\n'},
            fix_subject,
        )
        self.commit({TRACKER: '- [x] one\n- [x] two\n'}, tracker_subject)
        self.git('checkout', '--quiet', DEFAULT_BRANCH)
        self.commit(
            {
                versioning.PACKAGE_FILE: _package_json(tag='157.0.8103.30'),
                versioning.PINSLIST_TIMESTAMP_FILE: _pinslist(
                    timestamp=NEWER_TIMESTAMP
                ),
            },
            'Update from Chromium 157.0.8103.15 to Chromium 157.0.8103.30.',
        )
        self.git('checkout', '--quiet', 'feature')

        console = ConsoleCapture().install()
        self.addCleanup(console.uninstall)
        with (
            patch.dict(os.environ, {'GIT_EDITOR': 'false'}),
            patch.object(brockit, 'VsCodeIpcConnection'),
        ):
            brockit.Rebase().execute(
                from_ref=fork, to_ref=DEFAULT_BRANCH, squash_minor_bumps=True
            )

        self.assertFalse(repository.brave.is_rebase_in_progress())
        self.assertEqual(
            self.git(
                'log', '--reverse', '--format=%s', f'{DEFAULT_BRANCH}..HEAD'
            ).splitlines(),
            [VERSION_SUBJECT, tracker_subject, fix_subject],
        )
        self.assertEqual(
            self.read(versioning.PACKAGE_FILE),
            _package_json(tag='158.0.8127.2'),
        )
        self.assertEqual(
            self.read(versioning.PINSLIST_TIMESTAMP_FILE),
            _pinslist(timestamp=NEWER_TIMESTAMP),
        )
        self.assertEqual(self.read(TRACKER), '- [x] one\n- [x] two\n')
        self.assertIn(
            f'{Rule.CODEHEALTH_TRACKER.value}:\n    ✔ {TRACKER}', console.text
        )
        self.assertIn(
            f'{Rule.VERSION_BUMP.value}:\n'
            f'    ✔ {versioning.PINSLIST_TIMESTAMP_FILE}\n'
            f'    ✔ {versioning.PACKAGE_FILE}',
            console.text,
        )


if __name__ == '__main__':
    unittest.main()
