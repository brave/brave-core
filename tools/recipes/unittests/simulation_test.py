#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the simulation runtime: SimFS, step runner, and expectations."""

import os
import subprocess
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
import post_process as pp
import simulation
from recipe_test_api import RecipeTestApi, StepTestData, TestData


class SimFSTest(unittest.TestCase):

    def test_file_and_ancestor_semantics(self):
        fs = simulation.SimFS(files=['/b/s/chromium/src/chrome/VERSION'])
        self.assertTrue(fs.is_file('/b/s/chromium/src/chrome/VERSION'))
        self.assertTrue(fs.exists('/b/s/chromium/src/chrome/VERSION'))
        # Ancestors of a seeded file are directories that exist.
        self.assertTrue(fs.is_dir('/b/s/chromium/src'))
        # The file itself is not a directory.
        self.assertFalse(fs.is_dir('/b/s/chromium/src/chrome/VERSION'))
        self.assertFalse(fs.exists('/b/s/nope'))

    def test_mkdir_mutates(self):
        fs = simulation.SimFS()
        self.assertFalse(fs.is_dir('/b/s/out'))
        fs.add_dir('/b/s/out')
        self.assertTrue(fs.is_dir('/b/s/out'))


class SimulationStepRunnerTest(unittest.TestCase):

    def test_records_and_returns_completed_process(self):
        runner = simulation.SimulationStepRunner()
        result = runner.run({
            'name': 'go',
            'cmd': ['echo', 'hi']
        },
                            check=True,
                            capture_output=False)
        self.assertIsInstance(result, subprocess.CompletedProcess)
        self.assertEqual(result.returncode, 0)
        self.assertEqual(runner.recorded_steps[0]['name'], 'go')

    def test_checked_failure_raises_and_records_retcode(self):
        runner = simulation.SimulationStepRunner(
            {'go': StepTestData(retcode=1)})
        with self.assertRaises(subprocess.CalledProcessError):
            runner.run({
                'name': 'go',
                'cmd': ['false']
            },
                       check=True,
                       capture_output=False)
        # The failing step is still recorded (before the raise).
        self.assertEqual(runner.recorded_steps[0]['retcode'], 1)

    def test_unchecked_failure_does_not_raise(self):
        runner = simulation.SimulationStepRunner(
            {'go': StepTestData(retcode=1)})
        result = runner.run({
            'name': 'go',
            'cmd': ['false']
        },
                            check=False,
                            capture_output=False)
        self.assertEqual(result.returncode, 1)


class TestContextTest(unittest.TestCase):

    def test_from_test_data_reads_mod_data(self):
        td = (RecipeTestApi.empty_test_data())
        td.mod_data = {
            'platform': {
                'name': 'mac'
            },
            'env': {
                'vars': {
                    'K': 'V'
                },
                'which': {
                    'gclient': '/g'
                }
            },
            'path': {
                'files': ['chromium/src/chrome/VERSION'],
                'dirs': []
            },
        }
        ctx = simulation.TestContext.from_test_data(td)
        self.assertEqual(ctx.platform, 'mac')
        self.assertEqual(ctx.env['K'], 'V')
        self.assertEqual(ctx.which_map['gclient'], '/g')
        # Relative seed resolves under the simulated workspace.
        self.assertTrue(ctx.fs.is_file('/b/s/chromium/src/chrome/VERSION'))


class ExpectationTest(unittest.TestCase):

    def test_stabilize_tokens(self):
        ctx = simulation.TestContext()
        self.assertEqual(simulation.stabilize('/b/s/out/x', ctx),
                         '[WORKSPACE]/out/x')
        self.assertEqual(simulation.stabilize('/b/home/.cache', ctx),
                         '[HOME]/.cache')

    def test_build_steps_success_result(self):
        runner = simulation.SimulationStepRunner()
        runner.run({
            'name': 'a',
            'cmd': ['/b/s/out/tool']
        },
                   check=True,
                   capture_output=False)
        steps = simulation.build_steps(runner, None, simulation.TestContext())
        self.assertEqual(steps['a']['cmd'], ['[WORKSPACE]/out/tool'])
        # A successful run's $result carries no failure key.
        self.assertEqual(steps[pp.RESULT_STEP], {'name': '$result'})

    def test_build_steps_failure_result_stabilizes_reason(self):
        runner = simulation.SimulationStepRunner()
        failure = {'humanReason': 'boom at /b/s/out/tool'}
        steps = simulation.build_steps(runner, failure,
                                       simulation.TestContext())
        # An infra failure carries only humanReason (paths stabilized).
        self.assertEqual(
            steps[pp.RESULT_STEP], {
                'name': '$result',
                'failure': {
                    'humanReason': 'boom at [WORKSPACE]/out/tool'
                },
            })

    def test_apply_post_process_filter_and_drop(self):
        steps = {'a': {'name': 'a'}, '$result': {'name': '$result'}}
        # A filtering hook narrows the steps for the written expectation.
        keep_a = RecipeTestApi.post_process(lambda c, s: {'a': s['a']})
        filtered, failed_checks = simulation.apply_post_process(
            keep_a.post_process_hooks, steps)
        self.assertEqual(list(filtered), ['a'])
        self.assertEqual(failed_checks, [])
        # DropExpectation -> None.
        drop = RecipeTestApi.post_process(pp.DropExpectation)
        filtered, _ = simulation.apply_post_process(drop.post_process_hooks,
                                                    steps)
        self.assertIsNone(filtered)

    def test_apply_post_process_rejects_superset(self):
        steps = {'a': {'name': 'a'}, '$result': {'name': '$result'}}
        # A hook that adds a step is not a subset of the recorded steps.
        add = RecipeTestApi.post_process(lambda c, s: {
            **s, 'b': {
                'name': 'b'
            }
        })
        with self.assertRaises(simulation.PostProcessError):
            simulation.apply_post_process(add.post_process_hooks, steps)


if __name__ == '__main__':
    unittest.main()
