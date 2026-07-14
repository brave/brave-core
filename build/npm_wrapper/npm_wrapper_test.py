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


def write_package_json(project_dir, package_manager_name):
    with open(os.path.join(project_dir, npm_wrapper.PACKAGE_JSON),
              'w',
              encoding='utf-8') as package_json:
        package_json.write("""{
  "devEngines": {
    "packageManager": {
      "name": "%s"
    }
  }
}
""" % package_manager_name)


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


class TestNodeModulesCleanup(unittest.TestCase):

    def test_clean_npm_node_modules(self):
        with tempfile.TemporaryDirectory() as tmp:
            nm = os.path.join(tmp, 'node_modules')
            os.makedirs(nm)
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
        cmd = npm_wrapper.translate_to_pnpm(['ci'])
        self.assertEqual(cmd, ['/bin/pnpm', 'install', '--frozen-lockfile'])

    def test_install_no_fund_ignored(self):
        cmd = npm_wrapper.translate_to_pnpm(['--no-fund', 'install'])
        self.assertEqual(cmd, ['/bin/pnpm', 'install'])

    def test_install_package_becomes_add(self):
        cmd = npm_wrapper.translate_to_pnpm(['install', 'left-pad'])
        self.assertEqual(cmd, ['/bin/pnpm', 'add', 'left-pad'])

    def test_install_ignore_scripts(self):
        cmd = npm_wrapper.translate_to_pnpm(['install', '--ignore-scripts'])
        self.assertEqual(cmd, ['/bin/pnpm', 'install', '--ignore-scripts'])

    def test_prefix_maps_to_dir(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['--prefix', 'src/brave', 'run', 'build'])
        self.assertEqual(cmd,
                         ['/bin/pnpm', '--dir', 'src/brave', 'run', 'build'])

    def test_prefix_equals_maps_to_dir(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['--prefix=src/brave', 'run', 'build'])
        self.assertEqual(cmd,
                         ['/bin/pnpm', '--dir', 'src/brave', 'run', 'build'])

    def test_run_strips_separator(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['run', 'build', '--', '--target=foo'])
        self.assertEqual(cmd, ['/bin/pnpm', 'run', 'build', '--target=foo'])

    def test_run_keeps_positional_before_separator(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['run', 'build', 'Release', '--', '--gn=build_sparkle:true'])
        self.assertEqual(cmd, [
            '/bin/pnpm', 'run', 'build', 'Release', '--gn=build_sparkle:true'
        ])

    def test_run_script_alias(self):
        cmd = npm_wrapper.translate_to_pnpm(['run-script', 'build'])
        self.assertEqual(cmd, ['/bin/pnpm', 'run', 'build'])

    def test_cache_clean_to_store_prune(self):
        cmd = npm_wrapper.translate_to_pnpm(['cache', 'clean', '--force'])
        self.assertEqual(cmd, ['/bin/pnpm', 'store', 'prune'])

    def test_version_passthrough_args(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['version', '--no-git-tag-version', 'patch'])
        self.assertEqual(
            cmd, ['/bin/pnpm', 'version', '--no-git-tag-version', 'patch'])

    def test_silent_reporter(self):
        cmd = npm_wrapper.translate_to_pnpm(
            ['--silent', 'run', 'list_affected_tests'])
        self.assertEqual(
            cmd,
            ['/bin/pnpm', '--reporter=silent', 'run', 'list_affected_tests'])

    def test_proxy_flags(self):
        cmd = npm_wrapper.translate_to_pnpm([
            'install', '--proxy', 'http://proxy', '--https-proxy',
            'https://proxy'
        ])
        self.assertEqual(cmd, [
            '/bin/pnpm', 'install', '--proxy', 'http://proxy', '--https-proxy',
            'https://proxy'
        ])

    def test_value_flag_without_value_is_preserved(self):
        cmd = npm_wrapper.translate_to_pnpm(['install', '--proxy'])
        self.assertEqual(cmd, ['/bin/pnpm', 'install', '--proxy'])

    def test_audit(self):
        cmd = npm_wrapper.translate_to_pnpm(['audit', '--json'])
        self.assertEqual(cmd, ['/bin/pnpm', 'audit', '--json'])

    def test_version_flag(self):
        cmd = npm_wrapper.translate_to_pnpm(['--version'])
        self.assertEqual(cmd, ['/bin/pnpm', '--version'])


class TestGating(unittest.TestCase):

    def test_should_use_pnpm_with_package_manager_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            write_package_json(tmp, 'pnpm')
            self.assertTrue(npm_wrapper.should_use_pnpm(tmp))

    def test_should_not_use_pnpm_with_npm_package_manager_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            write_package_json(tmp, 'npm')
            self.assertFalse(npm_wrapper.should_use_pnpm(tmp))

    def test_fails_without_package_json(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(RuntimeError,
                                        'failed to read package manager name'):
                npm_wrapper.should_use_pnpm(tmp)

    def test_does_not_use_pnpm_without_package_manager(self):
        with tempfile.TemporaryDirectory() as tmp:
            with open(os.path.join(tmp, npm_wrapper.PACKAGE_JSON),
                      'w',
                      encoding='utf-8') as package_json:
                package_json.write('{"devEngines": {}}')
            self.assertFalse(npm_wrapper.should_use_pnpm(tmp))

    def test_fails_with_invalid_package_json(self):
        with tempfile.TemporaryDirectory() as tmp:
            with open(os.path.join(tmp, npm_wrapper.PACKAGE_JSON),
                      'w',
                      encoding='utf-8') as package_json:
                package_json.write('{')
            with self.assertRaisesRegex(RuntimeError,
                                        'failed to read package manager name'):
                npm_wrapper.should_use_pnpm(tmp)


class TestMainPassthrough(unittest.TestCase):

    def test_global_install_passthrough(self):
        with mock.patch.object(npm_wrapper, 'run_real_npm',
                               return_value=0) as real_npm:
            rc = npm_wrapper.main(['install', '-g', 'pnpm'])
            self.assertEqual(rc, 0)
            real_npm.assert_called_once_with(['install', '-g', 'pnpm'])

    def test_global_install_passthrough_with_pnpm_package_manager(self):
        with tempfile.TemporaryDirectory() as tmp:
            write_package_json(tmp, 'pnpm')
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

    def test_npm_package_manager_passthrough(self):
        with tempfile.TemporaryDirectory() as tmp:
            write_package_json(tmp, 'npm')
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

    def test_missing_package_manager_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            orig = os.getcwd()
            try:
                os.chdir(tmp)
                with self.assertRaisesRegex(
                        RuntimeError, 'failed to read package manager name'):
                    npm_wrapper.main(['install'])
            finally:
                os.chdir(orig)

    def test_install_cleans_node_modules(self):
        with tempfile.TemporaryDirectory() as tmp:
            nm = os.path.join(tmp, 'node_modules')
            os.makedirs(nm)
            write_package_json(tmp, 'pnpm')
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
