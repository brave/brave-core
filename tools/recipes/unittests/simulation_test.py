#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the simulation runtime: SimFS, step runner, and expectations."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
import post_process as pp
import simulation
from recipe_test_api import RecipeTestApi, StepTestData, TestData


class SimFSTest(unittest.TestCase):

    def test_file_and_ancestor_semantics(self):
        fs = simulation.SimFS(files=['/b/s/brave-browser/src/chrome/VERSION'])
        self.assertTrue(fs.is_file('/b/s/brave-browser/src/chrome/VERSION'))
        self.assertTrue(fs.exists('/b/s/brave-browser/src/chrome/VERSION'))
        # Ancestors of a seeded file are directories that exist.
        self.assertTrue(fs.is_dir('/b/s/brave-browser/src'))
        # The file itself is not a directory.
        self.assertFalse(fs.is_dir('/b/s/brave-browser/src/chrome/VERSION'))
        self.assertFalse(fs.exists('/b/s/nope'))

    def test_mkdir_mutates(self):
        fs = simulation.SimFS()
        self.assertFalse(fs.is_dir('/b/s/out'))
        fs.add_dir('/b/s/out')
        self.assertTrue(fs.is_dir('/b/s/out'))


def _run(runner, step):
    """Hand *runner* a step, as the `step` module would."""
    runner.step_test_data(step['name'], None)
    return runner.run(step, {'stdin': None, 'stdout': None, 'stderr': None})


class SimulationStepRunnerTest(unittest.TestCase):

    def test_records_the_step_and_returns_its_retcode(self):
        runner = simulation.SimulationStepRunner()
        self.assertEqual(_run(runner, {
            'name': 'go',
            'cmd': ['echo', 'hi']
        }), 0)
        self.assertEqual(runner.recorded_steps[0], {
            'name': 'go',
            'cmd': ['echo', 'hi']
        })

    def test_seeded_retcode_is_returned_and_recorded(self):
        runner = simulation.SimulationStepRunner(
            TestData() + RecipeTestApi.step_data('go', retcode=1))
        self.assertEqual(_run(runner, {'name': 'go', 'cmd': ['false']}), 1)
        self.assertEqual(runner.recorded_steps[0]['retcode'], 1)

    def test_stdin_is_recorded_but_the_other_handles_are_not(self):
        runner = simulation.SimulationStepRunner()
        runner.step_test_data('cat', None)
        runner.run({
            'name': 'cat',
            'cmd': ['cat'],
            'stdin': 'fed to the step'
        }, {
            'stdin': 'fed to the step',
            'stdout': '/path/to/tmp/',
            'stderr': None
        })
        self.assertEqual(runner.recorded_steps[0], {
            'name': 'cat',
            'cmd': ['cat'],
            'stdin': 'fed to the step'
        })

    def test_a_steps_own_default_data_is_used(self):
        runner = simulation.SimulationStepRunner()

        def default():
            data = StepTestData()
            data.retcode = 7
            return data

        runner.step_test_data('go', default)
        self.assertEqual(runner.run({
            'name': 'go',
            'cmd': ['do-thing']
        }, {}), 7)


class SubprocessStepRunnerTest(unittest.TestCase):

    def test_nothing_is_simulated(self):
        # The production runner reports "not simulated" for every lookup, so a
        # placeholder rendering for a real step touches the real filesystem.
        data = simulation.SubprocessStepRunner().step_test_data('go', None)
        self.assertFalse(data.enabled)
        self.assertFalse(data.stdout.enabled)


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
                'files': ['brave-browser/src/chrome/VERSION'],
                'dirs': []
            },
        }
        ctx = simulation.TestContext.from_test_data(td)
        self.assertEqual(ctx.platform, 'mac')
        self.assertEqual(ctx.env['K'], 'V')
        self.assertEqual(ctx.which_map['gclient'], '/g')
        # Relative seed resolves under the simulated workspace token.
        self.assertTrue(
            ctx.fs.is_file('[WORKSPACE]/brave-browser/src/chrome/VERSION'))


class ExpectationTest(unittest.TestCase):

    def test_stabilize_tokens(self):
        # `[WORKSPACE]`/`[HOME]` need no rewriting -- `recipe_modules/path/
        # api.py` builds them as literal `config_types.Path` tokens from
        # construction, so an already-tokenized string passes through as-is.
        self.assertEqual(simulation.stabilize('[WORKSPACE]/out/x'),
                         '[WORKSPACE]/out/x')
        self.assertEqual(simulation.stabilize('[HOME]/.cache'),
                         '[HOME]/.cache')
        # RECIPES_ROOT is the one remaining real machine path: it's still
        # rewritten, since resource scripts genuinely live there on disk.
        real_path = f'{simulation.RECIPES_ROOT}/recipe_modules/file'
        self.assertEqual(simulation.stabilize(real_path),
                         '[RECIPES_ROOT]/recipe_modules/file')

    def test_build_steps_success_result(self):
        runner = simulation.SimulationStepRunner()
        _run(runner, {'name': 'a', 'cmd': ['[WORKSPACE]/out/tool']})
        steps = simulation.build_steps(runner, None)
        self.assertEqual(steps['a']['cmd'], ['[WORKSPACE]/out/tool'])
        # A successful run's $result carries no failure key.
        self.assertEqual(steps[pp.RESULT_STEP], {'name': '$result'})

    def test_build_steps_failure_result_stabilizes_reason(self):
        runner = simulation.SimulationStepRunner()
        failure = {
            'humanReason': f'boom at {simulation.RECIPES_ROOT}/recipe_modules/file'
        }
        steps = simulation.build_steps(runner, failure)
        # An infra failure carries only humanReason (paths stabilized).
        self.assertEqual(
            steps[pp.RESULT_STEP], {
                'name': '$result',
                'failure': {
                    'humanReason': 'boom at [RECIPES_ROOT]/recipe_modules/file'
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
