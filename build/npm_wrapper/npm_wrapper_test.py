#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import tempfile
import unittest
from unittest import mock

import npm_wrapper


class TestProjectDir(unittest.TestCase):

    def test_resolve_cwd(self):
        self.assertEqual(npm_wrapper.resolve_project_dir(None), os.getcwd())

    def test_resolve_relative_prefix(self):
        with tempfile.TemporaryDirectory() as tmp:
            sub = os.path.join(tmp, 'proj')
            os.makedirs(sub)
            orig = os.getcwd()
            try:
                os.chdir(tmp)
                self.assertEqual(npm_wrapper.resolve_project_dir('proj'), sub)
            finally:
                os.chdir(orig)


class TestNodeModulesHeuristics(unittest.TestCase):

    def test_pnpm_layout(self):
        with tempfile.TemporaryDirectory() as tmp:
            nm = os.path.join(tmp, 'node_modules')
            os.makedirs(os.path.join(nm, '.pnpm'))
            self.assertTrue(npm_wrapper.is_pnpm_node_modules(nm))
            self.assertFalse(npm_wrapper.is_npm_node_modules(nm))

    def test_npm_layout(self):
        with tempfile.TemporaryDirectory() as tmp:
            nm = os.path.join(tmp, 'node_modules')
            os.makedirs(nm)
            open(os.path.join(nm, '.package-lock.json'), 'w',
                 encoding='utf-8').close()
            self.assertTrue(npm_wrapper.is_npm_node_modules(nm))
            self.assertFalse(npm_wrapper.is_pnpm_node_modules(nm))

    def test_clean_npm_node_modules(self):
        with tempfile.TemporaryDirectory() as tmp:
            nm = os.path.join(tmp, 'node_modules')
            os.makedirs(nm)
            open(os.path.join(nm, '.package-lock.json'), 'w',
                 encoding='utf-8').close()
            npm_wrapper.maybe_clean_node_modules(tmp)
            self.assertFalse(os.path.exists(nm))

    def test_skip_clean_pnpm_layout(self):
        with tempfile.TemporaryDirectory() as tmp:
            nm = os.path.join(tmp, 'node_modules')
            os.makedirs(os.path.join(nm, '.pnpm'))
            npm_wrapper.maybe_clean_node_modules(tmp)
            self.assertTrue(os.path.isdir(nm))


class TestTranslateToPnpm(unittest.TestCase):

    def setUp(self):
        self._pnpm = mock.patch.object(npm_wrapper,
                                       'find_pnpm',
                                       return_value='/bin/pnpm')
        self._pnpm.start()

    def tearDown(self):
        self._pnpm.stop()

    def test_ci_frozen_lockfile(self):
        cmd = npm_wrapper.translate_to_pnpm(['ci'], '/proj')
        self.assertEqual(cmd, ['/bin/pnpm', 'install', '--frozen-lockfile'])

    def test_install_no_fund_ignored(self):
        cmd = npm_wrapper.translate_to_pnpm(['--no-fund', 'install'], '/proj')
        self.assertEqual(cmd, ['/bin/pnpm', 'install'])

    def test_install_package_becomes_add(self):
        cmd = npm_wrapper.translate_to_pnpm(['install', 'left-pad'], '/proj')
        self.assertEqual(cmd, ['/bin/pnpm', 'add', 'left-pad'])

    def test_install_ignore_scripts(self):
        cmd = npm_wrapper.translate_to_pnpm(['install', '--ignore-scripts'],
                                            '/proj')
        self.assertEqual(cmd, ['/bin/pnpm', 'install', '--ignore-scripts'])

    def test_prefix_maps_to_dir(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['--prefix', 'src/brave', 'run', 'build'], '/abs/brave')
        self.assertEqual(cmd,
                         ['/bin/pnpm', '--dir', 'src/brave', 'run', 'build'])

    def test_prefix_equals_maps_to_dir(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['--prefix=src/brave', 'run', 'build'], '/abs/brave')
        self.assertEqual(cmd,
                         ['/bin/pnpm', '--dir', 'src/brave', 'run', 'build'])

    def test_run_strips_separator(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['run', 'build', '--', '--target=foo'], '/proj')
        self.assertEqual(cmd, ['/bin/pnpm', 'run', 'build', '--target=foo'])

    def test_run_keeps_positional_before_separator(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['run', 'build', 'Release', '--', '--gn=build_sparkle:true'],
            '/proj')
        self.assertEqual(cmd, [
            '/bin/pnpm', 'run', 'build', 'Release', '--gn=build_sparkle:true'
        ])

    def test_run_script_alias(self):
        cmd = npm_wrapper.translate_to_pnpm(['run-script', 'build'], '/proj')
        self.assertEqual(cmd, ['/bin/pnpm', 'run', 'build'])

    def test_cache_clean_to_store_prune(self):
        cmd = npm_wrapper.translate_to_pnpm(['cache', 'clean', '--force'],
                                            '/proj')
        self.assertEqual(cmd, ['/bin/pnpm', 'store', 'prune'])

    def test_version_passthrough_args(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['version', '--no-git-tag-version', 'patch'], '/proj')
        self.assertEqual(
            cmd, ['/bin/pnpm', 'version', '--no-git-tag-version', 'patch'])

    def test_silent_reporter(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['--silent', 'run', 'list_affected_tests'], '/proj')
        self.assertEqual(
            cmd,
            ['/bin/pnpm', '--reporter=silent', 'run', 'list_affected_tests'])

    def test_proxy_flags(self):
        cmd = npm_wrapper.translate_to_pnpm([
            'install', '--proxy', 'http://proxy', '--https-proxy',
            'https://proxy'
        ], '/proj')
        self.assertEqual(cmd, [
            '/bin/pnpm', 'install', '--proxy', 'http://proxy', '--https-proxy',
            'https://proxy'
        ])

    def test_value_flag_without_value_is_preserved(self):
        cmd = npm_wrapper.translate_to_pnpm(['install', '--proxy'], '/proj')
        self.assertEqual(cmd, ['/bin/pnpm', 'install', '--proxy'])

    def test_audit(self):
        cmd = npm_wrapper.translate_to_pnpm(['audit', '--json'], '/proj')
        self.assertEqual(cmd, ['/bin/pnpm', 'audit', '--json'])

    def test_version_flag(self):
        cmd = npm_wrapper.translate_to_pnpm(['--version'], '/proj')
        self.assertEqual(cmd, ['/bin/pnpm', '--version'])


class TestGating(unittest.TestCase):

    def test_should_use_pnpm_with_lockfile(self):
        with tempfile.TemporaryDirectory() as tmp:
            open(os.path.join(tmp, npm_wrapper.PNPM_LOCKFILE),
                 'w',
                 encoding='utf-8').close()
            self.assertTrue(npm_wrapper.should_use_pnpm(tmp))

    def test_should_not_use_pnpm_without_lockfile(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertFalse(npm_wrapper.should_use_pnpm(tmp))


class TestMainPassthrough(unittest.TestCase):

    def test_global_install_passthrough(self):
        with mock.patch.object(npm_wrapper, 'run_real_npm',
                               return_value=0) as real_npm:
            rc = npm_wrapper.main(['install', '-g', 'pnpm'])
            self.assertEqual(rc, 0)
            real_npm.assert_called_once_with(['install', '-g', 'pnpm'])

    def test_global_install_passthrough_with_lockfile(self):
        with tempfile.TemporaryDirectory() as tmp:
            open(os.path.join(tmp, npm_wrapper.PNPM_LOCKFILE),
                 'w',
                 encoding='utf-8').close()
            orig = os.getcwd()
            try:
                os.chdir(tmp)
                with mock.patch.object(npm_wrapper,
                                       'run_real_npm',
                                       return_value=0) as real_npm:
                    with mock.patch.object(npm_wrapper, 'run_pnpm') as pnpm:
                        rc = npm_wrapper.main(['install', '-g', 'pnpm'])
                self.assertEqual(rc, 0)
                real_npm.assert_called_once_with(['install', '-g', 'pnpm'])
                pnpm.assert_not_called()
            finally:
                os.chdir(orig)

    def test_no_lockfile_passthrough(self):
        with tempfile.TemporaryDirectory() as tmp:
            orig = os.getcwd()
            try:
                os.chdir(tmp)
                with mock.patch.object(npm_wrapper,
                                       'run_real_npm',
                                       return_value=0) as real_npm:
                    rc = npm_wrapper.main(['install'])
                    self.assertEqual(rc, 0)
                    real_npm.assert_called_once_with(['install'])
            finally:
                os.chdir(orig)

    def test_wrapper_disabled(self):
        with mock.patch.dict(os.environ,
                             {npm_wrapper.WRAPPER_DISABLE_ENV: '0'}):
            with mock.patch.object(npm_wrapper, 'run_real_npm',
                                   return_value=0) as real_npm:
                npm_wrapper.main(['install'])
                real_npm.assert_called_once_with(['install'])

    def test_install_cleans_node_modules(self):
        with tempfile.TemporaryDirectory() as tmp:
            nm = os.path.join(tmp, 'node_modules')
            os.makedirs(nm)
            open(os.path.join(tmp, npm_wrapper.PNPM_LOCKFILE),
                 'w',
                 encoding='utf-8').close()
            open(os.path.join(nm, '.package-lock.json'), 'w',
                 encoding='utf-8').close()
            orig = os.getcwd()
            try:
                os.chdir(tmp)
                with mock.patch.object(npm_wrapper,
                                       'find_pnpm',
                                       return_value='/bin/pnpm'):
                    with mock.patch('subprocess.run',
                                    return_value=mock.Mock(returncode=0)):
                        npm_wrapper.main(['install'])
                self.assertFalse(os.path.exists(nm))
            finally:
                os.chdir(orig)


if __name__ == '__main__':
    unittest.main()
