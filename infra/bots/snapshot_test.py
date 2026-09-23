#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for snapshot.py: compute_fresh_output() and write_output()'s
staleness handling. `bots.py`'s CLI wiring for the `snapshot` subcommand is
exercised in bots_test.py instead. _load_config()/cmd_snapshot() are
exercised manually, not here, since they mutate real process-global state
(sys.path, sys.modules, the shared lib.config registries)."""

import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(
    0,
    os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                 'config'))

import snapshot
from lib.config import BuildersRegistry, GnArgsRegistry


def _make_registries():
    gn_args_registry = GnArgsRegistry()
    gn_args_registry.config(name='linux', args={'target_os': 'linux'})
    gn_args_registry.config(name='x64', args={'target_cpu': 'x64'})
    builders_registry = BuildersRegistry(gn_args_registry)
    builders_registry.builder(
        name='test-builder',
        sync_config=builders_registry.sync_config(target_os='linux',
                                                  target_cpu='x64'),
        gn_args=gn_args_registry.config(configs=['linux', 'x64'],
                                        args={'is_asan': True}),
        targets=builders_registry.targets(compile=['brave:all'],
                                          tests=['a_test']),
    )
    return builders_registry, gn_args_registry


class ComputeFreshOutputTest(unittest.TestCase):

    def test_produces_three_files_per_builder(self):
        builders_registry, gn_args_registry = _make_registries()
        fresh = snapshot.compute_fresh_output(builders_registry,
                                              gn_args_registry)
        self.assertEqual(
            set(fresh), {
                'test-builder/gn-args.json',
                'test-builder/sync.json',
                'test-builder/targets.json',
            })

    def test_gn_args_json_matches_resolve(self):
        builders_registry, gn_args_registry = _make_registries()
        fresh = snapshot.compute_fresh_output(builders_registry,
                                              gn_args_registry)
        self.assertEqual(json.loads(fresh['test-builder/gn-args.json']),
                         gn_args_registry.resolve('test-builder'))

    def test_sync_and_targets_json(self):
        builders_registry, gn_args_registry = _make_registries()
        fresh = snapshot.compute_fresh_output(builders_registry,
                                              gn_args_registry)
        self.assertEqual(json.loads(fresh['test-builder/sync.json']), {
            'target_os': 'linux',
            'target_cpu': 'x64',
            'gclient_overrides': {},
        })
        self.assertEqual(json.loads(fresh['test-builder/targets.json']), {
            'compile': ['brave:all'],
            'tests': ['a_test'],
        })

    def test_no_builders_means_no_output(self):
        builders_registry = BuildersRegistry(GnArgsRegistry())
        self.assertEqual(
            snapshot.compute_fresh_output(builders_registry, GnArgsRegistry()),
            {})


class WriteSnapshotTest(unittest.TestCase):
    """`write_snapshot()` composes `compute_fresh_output()` and
    `write_output()`; see ComputeFreshOutputTest/WriteOutputTest for each
    half's own behaviour."""

    def test_writes_resolved_builders_to_disk(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        builders_registry, gn_args_registry = _make_registries()

        result = snapshot.write_snapshot(builders_registry, gn_args_registry,
                                         Path(tmp.name))

        self.assertEqual(sorted(result.changed), [
            'test-builder/gn-args.json',
            'test-builder/sync.json',
            'test-builder/targets.json',
        ])
        self.assertEqual(
            json.loads(
                (Path(tmp.name) / 'test-builder/gn-args.json').read_text()),
            gn_args_registry.resolve('test-builder'))

    def test_rerun_with_no_changes_touches_nothing(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        builders_registry, gn_args_registry = _make_registries()

        snapshot.write_snapshot(builders_registry, gn_args_registry,
                                Path(tmp.name))
        result = snapshot.write_snapshot(builders_registry, gn_args_registry,
                                         Path(tmp.name))

        self.assertEqual(result.changed, [])
        self.assertEqual(sorted(result.unchanged), [
            'test-builder/gn-args.json',
            'test-builder/sync.json',
            'test-builder/targets.json',
        ])


class WriteOutputTest(unittest.TestCase):

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.output_dir = Path(tmp.name)

    def test_fresh_install_writes_everything(self):
        fresh = {'a/x.json': '{}\n', 'b/y.json': '{}\n'}
        result = snapshot.write_output(self.output_dir, fresh)
        self.assertEqual(sorted(result.changed), ['a/x.json', 'b/y.json'])
        self.assertEqual(result.unchanged, [])
        self.assertEqual(result.deleted, [])
        self.assertEqual((self.output_dir / 'a/x.json').read_text(), '{}\n')

    def test_rerun_with_no_changes_touches_nothing(self):
        fresh = {'a/x.json': '{}\n'}
        snapshot.write_output(self.output_dir, fresh)
        result = snapshot.write_output(self.output_dir, fresh)
        self.assertEqual(result.changed, [])
        self.assertEqual(result.unchanged, ['a/x.json'])
        self.assertEqual(result.deleted, [])

    def test_changed_content_is_rewritten(self):
        snapshot.write_output(self.output_dir, {'a/x.json': '{"v": 1}\n'})
        result = snapshot.write_output(self.output_dir,
                                       {'a/x.json': '{"v": 2}\n'})
        self.assertEqual(result.changed, ['a/x.json'])
        self.assertEqual((self.output_dir / 'a/x.json').read_text(),
                         '{"v": 2}\n')

    def test_stale_file_is_deleted(self):
        snapshot.write_output(self.output_dir, {
            'a/x.json': '{}\n',
            'b/y.json': '{}\n',
        })
        result = snapshot.write_output(self.output_dir, {'a/x.json': '{}\n'})
        self.assertEqual(result.deleted, ['b/y.json'])
        self.assertFalse((self.output_dir / 'b/y.json').exists())

    def test_stale_builder_directory_loses_all_its_files(self):
        # A whole builder disappearing should drop all three of its files.
        # The now-empty `old-builder/` directory itself is left behind,
        # faithful to lucicfg's own `generate`: it only ever removes files,
        # never prunes directories, so we don't either.
        snapshot.write_output(
            self.output_dir, {
                'old-builder/gn-args.json': '{}\n',
                'old-builder/sync.json': '{}\n',
                'old-builder/targets.json': '{}\n',
            })
        result = snapshot.write_output(self.output_dir,
                                       {'new-builder/gn-args.json': '{}\n'})
        self.assertEqual(sorted(result.deleted), [
            'old-builder/gn-args.json',
            'old-builder/sync.json',
            'old-builder/targets.json',
        ])
        self.assertEqual(list((self.output_dir / 'old-builder').iterdir()), [])

    def test_dry_run_reports_without_touching_disk(self):
        snapshot.write_output(self.output_dir, {
            'a/x.json': '{}\n',
            'b/y.json': '{}\n',
        })
        result = snapshot.write_output(self.output_dir,
                                       {'a/x.json': '{"v": 2}\n'},
                                       dry_run=True)
        self.assertEqual(result.changed, ['a/x.json'])
        self.assertEqual(result.deleted, ['b/y.json'])
        # Nothing was actually touched.
        self.assertEqual((self.output_dir / 'a/x.json').read_text(), '{}\n')
        self.assertTrue((self.output_dir / 'b/y.json').exists())

    def test_first_run_on_missing_directory(self):
        missing = self.output_dir / 'does-not-exist-yet'
        result = snapshot.write_output(missing, {'a/x.json': '{}\n'})
        self.assertEqual(result.changed, ['a/x.json'])
        self.assertTrue((missing / 'a/x.json').exists())


if __name__ == '__main__':
    unittest.main()
