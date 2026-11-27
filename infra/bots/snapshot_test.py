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

import ast
import contextlib
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(
    0,
    os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'config'
    ),
)

import generated_output
import snapshot
import testing_support
from lib.config import (
    BuildersRegistry,
    TargetsRegistry,
    ConfigError,
    GnArgsRegistry,
    builder_config,
)


def _make_registries():
    gn_args_registry = GnArgsRegistry()
    gn_args_registry.config(name='linux', args={'target_os': 'linux'})
    gn_args_registry.config(name='x64', args={'target_cpu': 'x64'})
    targets_registry = TargetsRegistry()
    targets_registry.binaries.console_test_launcher(
        name='a_test', label='//a:a_test'
    )
    targets_registry.tests.gtest_test(name='a_test', args=['--a'])
    targets_registry.compile_target(name='chrome', label='//chrome:chrome')
    targets_registry.mixin(name='b', args=['--b'])
    builders_registry = BuildersRegistry(gn_args_registry, targets_registry)
    builders_registry.builder(
        name='test-builder',
        builder_group='brave.test',
        recipe='brave_builder',
        siso_remote_jobs=500,
        siso_reapi_priority=2,
        builder_spec=builder_config.builder_spec(
            gclient_config=builder_config.gclient_config(
                target_os=['linux', 'android'],
                custom_deps={'src/b': None, 'src/a': 'https://a.example/a.git'},
                custom_vars={'checkout_x': True},
            ),
            chromium_config=builder_config.chromium_config(
                target_platform=builder_config.target_platform.LINUX,
                target_arch=builder_config.target_arch.INTEL,
                target_bits=64,
            ),
        ),
        gn_args=gn_args_registry.config(
            configs=['linux', 'x64'], args={'is_asan': True}
        ),
        targets=targets_registry.bundle(
            targets=['a_test'],
            additional_compile_targets=['chrome'],
            mixins=['b'],
        ),
    )
    return builders_registry, gn_args_registry


class ComputeFreshOutputTest(unittest.TestCase):
    def test_produces_five_files_per_builder(self):
        builders_registry, gn_args_registry = _make_registries()
        fresh = snapshot.compute_fresh_output(
            builders_registry, gn_args_registry
        )
        self.assertEqual(
            set(fresh),
            {
                'test-builder/gn-args.json',
                'test-builder/properties.json',
                'test-builder/sync.json',
                'test-builder/targets.json',
                'test-builder/targets/brave.test.json',
            },
        )

    def test_gn_args_json_matches_resolve(self):
        builders_registry, gn_args_registry = _make_registries()
        fresh = snapshot.compute_fresh_output(
            builders_registry, gn_args_registry
        )
        self.assertEqual(
            json.loads(fresh['test-builder/gn-args.json']),
            gn_args_registry.resolve('test-builder'),
        )

    def test_spec_disagreeing_with_gn_args_is_an_error(self):
        gn_args_registry = GnArgsRegistry()
        builders_registry = BuildersRegistry(
            gn_args_registry, TargetsRegistry()
        )
        builders_registry.builder(
            name='mismatched',
            builder_group='brave.test',
            builder_spec=builder_config.builder_spec(
                gclient_config=builder_config.gclient_config(),
                chromium_config=builder_config.chromium_config(
                    target_platform=builder_config.target_platform.ANDROID
                ),
            ),
            gn_args=gn_args_registry.config(
                args={'target_os': 'linux', 'target_cpu': 'x64'}
            ),
            targets=TargetsRegistry().bundle(),
        )
        with self.assertRaisesRegex(ConfigError, 'does not match'):
            snapshot.compute_fresh_output(builders_registry, gn_args_registry)

    def test_output_reads_back(self):
        builders_registry, gn_args_registry = _make_registries()
        output_dir = testing_support.use_output_dir(self)
        snapshot.write_snapshot(builders_registry, gn_args_registry, output_dir)
        output = generated_output.GeneratedBuilder('test-builder').read()
        self.assertEqual(
            output.builder_spec,
            {
                'builder_group': 'brave.test',
                # Unset fields (build_config, host_platform) are left out.
                'chromium_config': {
                    'target_arch': 'intel',
                    'target_bits': 64,
                    'target_platform': 'linux',
                },
                'gclient_config': {
                    'target_os': ['android', 'linux'],
                    'target_cpu': [],
                    # Sorted by path.
                    'custom_deps': {
                        'src/a': 'https://a.example/a.git',
                        'src/b': None,
                    },
                    'custom_vars': {'checkout_x': True},
                },
            },
        )
        self.assertEqual(
            output.gn_args, gn_args_registry.resolve('test-builder')
        )

    def test_properties_json_is_the_builds_input_properties(self):
        builders_registry, gn_args_registry = _make_registries()
        properties = json.loads(
            snapshot.compute_fresh_output(builders_registry, gn_args_registry)[
                'test-builder/properties.json'
            ]
        )
        self.assertEqual(properties['builder_group'], 'brave.test')
        self.assertEqual(properties['recipe'], 'brave_builder')
        self.assertIn(
            'builder_config', properties['$brave_tests_builder_config']
        )
        self.assertEqual(
            properties['$brave_siso'], {'remote_jobs': 500, 'reapi_priority': 2}
        )

    def test_properties_json_needs_a_builder_group(self):
        gn_args_registry = GnArgsRegistry()
        builders_registry = BuildersRegistry(
            gn_args_registry, TargetsRegistry()
        )
        builders_registry.builder(
            name='ungrouped',
            builder_spec=builder_config.builder_spec(
                gclient_config=builder_config.gclient_config(),
                chromium_config=builder_config.chromium_config(
                    target_platform=builder_config.target_platform.LINUX
                ),
            ),
            gn_args=gn_args_registry.config(
                args={'target_os': 'linux', 'target_cpu': 'x64'}
            ),
            targets=TargetsRegistry().bundle(),
        )
        with self.assertRaisesRegex(ConfigError, 'builder_group must be set'):
            snapshot.compute_fresh_output(builders_registry, gn_args_registry)

    def test_properties_json_leaves_out_an_unset_recipe(self):
        builders_registry, gn_args_registry = _make_registries()
        builders_registry.builder(
            name='no-recipe',
            builder_group='brave.test',
            builder_spec=builder_config.builder_spec(
                gclient_config=builder_config.gclient_config(),
                chromium_config=builder_config.chromium_config(
                    target_platform=builder_config.target_platform.LINUX
                ),
            ),
            gn_args=gn_args_registry.config(configs=['linux', 'x64']),
            targets=TargetsRegistry().bundle(),
        )
        fresh = snapshot.compute_fresh_output(
            builders_registry, gn_args_registry
        )
        self.assertNotIn(
            'recipe', json.loads(fresh['no-recipe/properties.json'])
        )

    def test_sync_and_targets_json(self):
        builders_registry, gn_args_registry = _make_registries()
        fresh = snapshot.compute_fresh_output(
            builders_registry, gn_args_registry
        )
        self.assertEqual(
            json.loads(fresh['test-builder/sync.json']),
            {
                'target_os': ['android', 'linux'],
                'target_cpu': [],
            },
        )
        self.assertEqual(
            json.loads(fresh['test-builder/targets.json']),
            {
                'compile': ['chrome', 'a_test'],
                'tests': ['a_test'],
            },
        )
        # The test's arguments, then its bundle's mixin's.
        self.assertEqual(
            json.loads(fresh['test-builder/targets/brave.test.json']),
            {
                'test-builder': {
                    'additional_compile_targets': ['chrome'],
                    'gtest_tests': [
                        {
                            'name': 'a_test',
                            'test': 'a_test',
                            'args': ['--a', '--b'],
                        }
                    ],
                }
            },
        )

    def test_tests_take_the_filter_files_that_apply_to_them(self):
        src_dir = testing_support.use_src_dir(self)
        for path in (
            'brave/test/filters/a_test.filter',
            'brave/test/filters/generated/a_test-linux.filter',
            'brave/test/filters/a_test-linux-x64.filter',
            'brave/test/filters/a_test-linux-asan.filter',
            # Not this build's sanitiser, nor its suite.
            'brave/test/filters/a_test-linux-msan.filter',
            'brave/test/filters/other_test.filter',
            'testing/buildbot/filters/linux.asan.a_test.filter',
        ):
            (src_dir / path).parent.mkdir(parents=True, exist_ok=True)
            (src_dir / path).write_text('')
        builders_registry, gn_args_registry = _make_registries()
        fresh = snapshot.compute_fresh_output(
            builders_registry, gn_args_registry
        )
        (test,) = json.loads(fresh['test-builder/targets/brave.test.json'])[
            'test-builder'
        ]['gtest_tests']
        self.assertEqual(
            test['args'],
            [
                '--a',
                '--b',
                '--test-launcher-filter-file='
                '../../brave/test/filters/a_test.filter;'
                '../../brave/test/filters/generated/a_test-linux.filter;'
                '../../brave/test/filters/a_test-linux-x64.filter;'
                '../../brave/test/filters/a_test-linux-asan.filter;'
                '../../testing/buildbot/filters/linux.asan.a_test.filter',
            ],
        )

    def test_no_builders_means_no_output(self):
        builders_registry = BuildersRegistry(
            GnArgsRegistry(), TargetsRegistry()
        )
        self.assertEqual(
            snapshot.compute_fresh_output(builders_registry, GnArgsRegistry()),
            {},
        )


class RenderIsolateMapTest(unittest.TestCase):
    def test_lists_every_binary_as_runtest_reads_it(self):
        targets_registry = TargetsRegistry()
        targets_registry.binaries.windowed_test_launcher(
            name='w', label='//t:w'
        )
        targets_registry.binaries.console_test_launcher(name='c', label='//t:c')
        rendered = generated_output.render_isolate_map(targets_registry)
        self.assertTrue(
            rendered.startswith('# Generated by `bots.py snapshot`')
        )
        self.assertEqual(
            ast.literal_eval(rendered),
            {
                'c': {'label': '//t:c', 'type': 'console_test_launcher'},
                'w': {'label': '//t:w', 'type': 'windowed_test_launcher'},
            },
        )


class WriteSnapshotTest(unittest.TestCase):
    """`write_snapshot()` composes `compute_fresh_output()` and
    `write_output()`; see ComputeFreshOutputTest/WriteOutputTest for each
    half's own behaviour."""

    def test_writes_resolved_builders_to_disk(self):
        with tempfile.TemporaryDirectory() as tmp:
            builders_registry, gn_args_registry = _make_registries()

            result = snapshot.write_snapshot(
                builders_registry, gn_args_registry, Path(tmp)
            )

            self.assertEqual(
                sorted(result.changed),
                [
                    'test-builder/gn-args.json',
                    'test-builder/properties.json',
                    'test-builder/sync.json',
                    'test-builder/targets.json',
                    'test-builder/targets/brave.test.json',
                ],
            )
            self.assertEqual(
                json.loads(
                    (Path(tmp) / 'test-builder/gn-args.json').read_text()
                ),
                gn_args_registry.resolve('test-builder'),
            )

    def test_rerun_with_no_changes_touches_nothing(self):
        with tempfile.TemporaryDirectory() as tmp:
            builders_registry, gn_args_registry = _make_registries()

            snapshot.write_snapshot(
                builders_registry, gn_args_registry, Path(tmp)
            )
            result = snapshot.write_snapshot(
                builders_registry, gn_args_registry, Path(tmp)
            )

            self.assertEqual(result.changed, [])
            self.assertEqual(
                sorted(result.unchanged),
                [
                    'test-builder/gn-args.json',
                    'test-builder/properties.json',
                    'test-builder/sync.json',
                    'test-builder/targets.json',
                    'test-builder/targets/brave.test.json',
                ],
            )


class WriteOutputTest(unittest.TestCase):
    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        tmp = stack.enter_context(tempfile.TemporaryDirectory())
        self.output_dir = Path(tmp)

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
        result = snapshot.write_output(
            self.output_dir, {'a/x.json': '{"v": 2}\n'}
        )
        self.assertEqual(result.changed, ['a/x.json'])
        self.assertEqual(
            (self.output_dir / 'a/x.json').read_text(), '{"v": 2}\n'
        )

    def test_stale_file_is_deleted(self):
        snapshot.write_output(
            self.output_dir,
            {
                'a/x.json': '{}\n',
                'b/y.json': '{}\n',
            },
        )
        result = snapshot.write_output(self.output_dir, {'a/x.json': '{}\n'})
        self.assertEqual(result.deleted, ['b/y.json'])
        self.assertFalse((self.output_dir / 'b/y.json').exists())

    def test_stale_builder_directory_loses_all_its_files(self):
        # A whole builder disappearing should drop all three of its files.
        # The now-empty `old-builder/` directory itself is left behind,
        # faithful to upstream's `generate`: it only ever removes files,
        # never prunes directories, so we don't either.
        snapshot.write_output(
            self.output_dir,
            {
                'old-builder/gn-args.json': '{}\n',
                'old-builder/sync.json': '{}\n',
                'old-builder/targets.json': '{}\n',
            },
        )
        result = snapshot.write_output(
            self.output_dir, {'new-builder/gn-args.json': '{}\n'}
        )
        self.assertEqual(
            sorted(result.deleted),
            [
                'old-builder/gn-args.json',
                'old-builder/sync.json',
                'old-builder/targets.json',
            ],
        )
        self.assertEqual(list((self.output_dir / 'old-builder').iterdir()), [])

    def test_dry_run_reports_without_touching_disk(self):
        snapshot.write_output(
            self.output_dir,
            {
                'a/x.json': '{}\n',
                'b/y.json': '{}\n',
            },
        )
        result = snapshot.write_output(
            self.output_dir, {'a/x.json': '{"v": 2}\n'}, dry_run=True
        )
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
