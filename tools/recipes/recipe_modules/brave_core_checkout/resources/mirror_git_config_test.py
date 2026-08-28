#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for mirror_git_config.py."""

import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import mirror_git_config
from mirror_git_config import (
    build_git_config,
    default_global_config,
    install,
    list_mirror_projects,
    main,
    uninstall,
    upstream_urls_for,
)


def _write(path: Path, text: str) -> None:
    """Write *text* to *path*, byte-identically on every platform.

    Mirrors what the script itself does (see tools/cr/python_dos_and_donts.md):
    explicit UTF-8, and `newline=''` so `\\n` in *text* is never translated to
    `\\r\\n` on Windows.
    """
    path.write_text(text, encoding='utf-8', newline='')


def _read(path: Path) -> str:
    """Read *path* as UTF-8 with no newline translation (see `_write`)."""
    return path.read_bytes().decode('utf-8')


class TestUpstreamUrlsFor(unittest.TestCase):
    """Unit tests for deriving a mirror's upstream URLs from its project name."""

    def test_simple_repo(self):
        self.assertEqual(
            upstream_urls_for('mirror/boringssl.googlesource.com/boringssl'),
            ('https://boringssl.googlesource.com/boringssl',
             'https://boringssl.googlesource.com/boringssl.git'))

    def test_nested_path(self):
        self.assertEqual(
            upstream_urls_for('mirror/chromium.googlesource.com/chromium/src'),
            ('https://chromium.googlesource.com/chromium/src',
             'https://chromium.googlesource.com/chromium/src.git'))

    def test_not_a_mirror_project(self):
        self.assertIsNone(upstream_urls_for('some-other-project'))

    def test_mirror_parent_itself(self):
        self.assertIsNone(upstream_urls_for('mirror'))

    def test_host_with_no_path(self):
        self.assertIsNone(upstream_urls_for('mirror/example.com'))


class TestBuildGitConfig(unittest.TestCase):
    """Unit tests for rendering the git config text."""

    def test_empty_input_is_empty_output(self):
        self.assertEqual(build_git_config([], 'bot'), '')

    def test_one_project(self):
        config = build_git_config(
            ['mirror/boringssl.googlesource.com/boringssl'], 'bot')
        self.assertEqual(
            config, '[url "ssh://bot@gerrit-ssh.brave.com:29418/mirror/'
            'boringssl.googlesource.com/boringssl"]\n'
            '\tinsteadOf = https://boringssl.googlesource.com/boringssl\n'
            '\tinsteadOf = https://boringssl.googlesource.com/boringssl.git\n')

    def test_skips_unrecognised_project_and_logs(self):
        with self.assertLogs(level='WARNING') as logs:
            config = build_git_config([
                'not-a-mirror', 'mirror/boringssl.googlesource.com/boringssl'
            ], 'bot')
        self.assertNotIn('not-a-mirror', config)
        self.assertIn('not-a-mirror', '\n'.join(logs.output))
        self.assertIn('boringssl.googlesource.com/boringssl', config)

    def test_two_projects_get_exact_distinct_urls(self):
        # Sibling repos whose names share a prefix (`foo` / `foo-bar`) must
        # each carry their own exact upstream strings, not a shorter shared
        # prefix that could be ambiguous to a reader of the generated file.
        config = build_git_config(
            ['mirror/example.com/foo', 'mirror/example.com/foo-bar'], 'bot')
        self.assertIn('insteadOf = https://example.com/foo\n', config)
        self.assertIn('insteadOf = https://example.com/foo.git\n', config)
        self.assertIn('insteadOf = https://example.com/foo-bar\n', config)
        self.assertIn('insteadOf = https://example.com/foo-bar.git\n', config)


class TestGitConfigRewritesUrls(unittest.TestCase):
    """End-to-end: the generated config actually redirects git, correctly."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)

    def tearDown(self):
        self._tmp.cleanup()

    def _make_bare_repo_with_commit(self, path: Path, message: str) -> str:
        subprocess.run(['git', 'init', '-q', '--bare', str(path)], check=True)
        subprocess.run(['git', 'symbolic-ref', 'HEAD', 'refs/heads/main'],
                       cwd=path,
                       check=True)
        work = path.parent / f'{path.name}.work'
        subprocess.run(['git', 'clone', '-q',
                        str(path), str(work)],
                       check=True,
                       capture_output=True)
        subprocess.run([
            'git', '-c', 'user.email=t@t.com', '-c', 'user.name=t', 'commit',
            '-q', '--allow-empty', '-m', message
        ],
                       cwd=work,
                       check=True)
        subprocess.run(['git', 'push', '-q', 'origin', 'HEAD:main'],
                       cwd=work,
                       check=True)
        return subprocess.run(['git', 'rev-parse', 'HEAD'],
                              cwd=work,
                              capture_output=True,
                              text=True,
                              check=True).stdout.strip()

    def test_prefix_sharing_siblings_resolve_to_their_own_repo(self):
        # `foo` and `foo-bar`'s upstream URLs share a string prefix, but each
        # must resolve to its own repo, and a fetch of either form (bare or
        # `.git`-suffixed) must reach that repo's tip, not the other one. git
        # does not chain `insteadOf` rewrites, so the config here points
        # `[url]` sections straight at local bare repos instead of a real
        # Gerrit mirror, which a real ssh:// URL would need a second hop to
        # reach.
        foo_target = self.tmp / 'foo-target'
        foobar_target = self.tmp / 'foobar-target'
        foo_head = self._make_bare_repo_with_commit(foo_target, 'foo')
        foobar_head = self._make_bare_repo_with_commit(foobar_target, 'foobar')
        self.assertNotEqual(foo_head, foobar_head)

        targets = {
            str(foo_target): upstream_urls_for('mirror/example.com/foo'),
            str(foobar_target): upstream_urls_for(
                'mirror/example.com/foo-bar'),
        }
        config = ''.join(f'[url "{target}"]\n' +
                         ''.join(f'\tinsteadOf = {url}\n' for url in urls)
                         for target, urls in targets.items())
        generated_cfg = self.tmp / 'generated.gitconfig'
        _write(generated_cfg, config)

        env = {
            **os.environ, 'GIT_CONFIG_GLOBAL': str(generated_cfg),
            'GIT_CONFIG_NOSYSTEM': '1'
        }
        for url, expected_head in (
            ('https://example.com/foo', foo_head),
            ('https://example.com/foo.git', foo_head),
            ('https://example.com/foo-bar', foobar_head),
            ('https://example.com/foo-bar.git', foobar_head),
        ):
            result = subprocess.run(['git', 'ls-remote', url, 'main'],
                                    capture_output=True,
                                    text=True,
                                    check=True,
                                    env=env)
            self.assertEqual(result.stdout.split()[0], expected_head,
                             f'{url} resolved to the wrong repo')


class TestDefaultGlobalConfig(unittest.TestCase):
    """Unit tests for resolving which file `install`/`uninstall` manage."""

    def test_honors_git_config_global_env_override(self):
        with mock.patch.dict(os.environ,
                             {'GIT_CONFIG_GLOBAL': '/tmp/custom.gitconfig'}):
            self.assertEqual(default_global_config(),
                             Path('/tmp/custom.gitconfig'))

    def test_falls_back_to_home_gitconfig(self):
        with mock.patch.dict(os.environ, {}, clear=True), \
             mock.patch.object(Path, 'home', return_value=Path('/home/x')):
            self.assertEqual(default_global_config(),
                             Path('/home/x/.gitconfig'))


class TestInstall(unittest.TestCase):
    """Unit tests for pointing the global config's [include] at a path."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.global_config = Path(self._tmp.name) / 'gitconfig'

    def tearDown(self):
        self._tmp.cleanup()

    def test_creates_missing_file_with_include_block(self):
        install(Path('/a/mirrors.gitconfig'), self.global_config)
        text = _read(self.global_config)
        self.assertIn('[include]', text)
        self.assertIn('path = /a/mirrors.gitconfig', text)

    def test_preserves_existing_content(self):
        _write(self.global_config, '[user]\n\tname = Test\n')
        install(Path('/a/mirrors.gitconfig'), self.global_config)
        text = _read(self.global_config)
        self.assertIn('[user]\n\tname = Test', text)
        self.assertIn('path = /a/mirrors.gitconfig', text)

    def test_second_call_replaces_rather_than_accumulates(self):
        install(Path('/a/first.gitconfig'), self.global_config)
        install(Path('/b/second.gitconfig'), self.global_config)
        text = _read(self.global_config)
        self.assertNotIn('/a/first.gitconfig', text)
        self.assertEqual(text.count('[include]'), 1)
        self.assertIn('path = /b/second.gitconfig', text)

    def test_does_not_disturb_a_users_own_include_section(self):
        _write(self.global_config, '[include]\n\tpath = ~/own.gitconfig\n')
        install(Path('/a/mirrors.gitconfig'), self.global_config)
        text = _read(self.global_config)
        self.assertIn('path = ~/own.gitconfig', text)
        self.assertIn('path = /a/mirrors.gitconfig', text)


class TestUninstall(unittest.TestCase):
    """Unit tests for undoing `install`."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.global_config = self.tmp / 'gitconfig'

    def tearDown(self):
        self._tmp.cleanup()

    def test_missing_global_config_returns_none(self):
        self.assertIsNone(uninstall(self.global_config))

    def test_no_managed_block_returns_none_and_leaves_file_untouched(self):
        _write(self.global_config, '[user]\n\tname = Test\n')
        self.assertIsNone(uninstall(self.global_config))
        self.assertEqual(_read(self.global_config), '[user]\n\tname = Test\n')

    def test_removes_block_and_deletes_target_file(self):
        target = self.tmp / 'mirrors.gitconfig'
        _write(target, '[url "x"]\n\tinsteadOf = y\n')
        install(target, self.global_config)

        removed = uninstall(self.global_config)

        self.assertEqual(removed, target)
        self.assertFalse(target.exists())
        self.assertNotIn('[include]', _read(self.global_config))

    def test_missing_target_file_is_not_an_error(self):
        target = self.tmp / 'already-gone.gitconfig'
        install(target, self.global_config)  # never created on disk

        removed = uninstall(self.global_config)

        self.assertEqual(removed, target)
        self.assertNotIn('[include]', _read(self.global_config))

    def test_preserves_surrounding_content(self):
        _write(self.global_config, '[user]\n\tname = Test\n')
        install(self.tmp / 'mirrors.gitconfig', self.global_config)
        uninstall(self.global_config)
        self.assertEqual(
            _read(self.global_config).strip(), '[user]\n\tname = Test')


class TestListMirrorProjects(unittest.TestCase):
    """Unit tests for the `gerrit ls-projects` query wrapper."""

    def test_success_parses_and_sorts_lines(self):
        out = 'mirror/b.googlesource.com/b\nmirror/a.googlesource.com/a\n\n'
        with mock.patch.object(mirror_git_config,
                               '_run',
                               return_value=subprocess.CompletedProcess(
                                   [], 0, stdout=out, stderr='')) as run:
            projects = list_mirror_projects('bot')
        self.assertEqual(
            projects,
            ['mirror/a.googlesource.com/a', 'mirror/b.googlesource.com/b'])
        argv = run.call_args.args
        self.assertEqual(argv[:4],
                         ('ssh', '-p', '29418', 'bot@gerrit-ssh.brave.com'))
        self.assertEqual(argv[4:],
                         ('gerrit', 'ls-projects', '--prefix', 'mirror/'))

    def test_query_failure_raises(self):
        with mock.patch.object(mirror_git_config,
                               '_run',
                               return_value=subprocess.CompletedProcess(
                                   [], 255, stdout='', stderr='denied')):
            with self.assertRaises(RuntimeError):
                list_mirror_projects('bot')


class TestMain(unittest.TestCase):
    """Integration tests for the CLI entry point."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)

    def tearDown(self):
        self._tmp.cleanup()

    def test_install_writes_config_and_includes_it(self):
        output = self.tmp / 'mirrors.gitconfig'
        global_config = self.tmp / 'gitconfig'
        with mock.patch.object(
                mirror_git_config,
                'list_mirror_projects',
                return_value=['mirror/boringssl.googlesource.com/boringssl'
                              ]), mock.patch('sys.argv', [
                                  'mirror_git_config.py', 'install', '--user',
                                  'bot', '--output',
                                  str(output), '--global-config',
                                  str(global_config)
                              ]):
            self.assertEqual(main(), 0)
        self.assertIn(
            'insteadOf = https://boringssl.googlesource.com/boringssl\n',
            _read(output))
        self.assertIn(f'path = {output.as_posix()}', _read(global_config))

    def test_install_defaults_global_config_from_env(self):
        output = self.tmp / 'mirrors.gitconfig'
        global_config = self.tmp / 'env.gitconfig'
        with mock.patch.object(mirror_git_config,
                               'list_mirror_projects',
                               return_value=[]), \
             mock.patch.dict(os.environ,
                             {'GIT_CONFIG_GLOBAL': str(global_config)}), \
             mock.patch('sys.argv', [
                'mirror_git_config.py', 'install', '--user', 'bot',
                '--output', str(output)
             ]):
            self.assertEqual(main(), 0)
        self.assertIn(f'path = {output.as_posix()}', _read(global_config))

    def test_uninstall_removes_include_and_deletes_file(self):
        output = self.tmp / 'mirrors.gitconfig'
        global_config = self.tmp / 'gitconfig'
        with mock.patch.object(
                mirror_git_config,
                'list_mirror_projects',
                return_value=['mirror/boringssl.googlesource.com/boringssl'
                              ]), mock.patch('sys.argv', [
                                  'mirror_git_config.py', 'install', '--user',
                                  'bot', '--output',
                                  str(output), '--global-config',
                                  str(global_config)
                              ]):
            main()

        with mock.patch('sys.argv', [
                'mirror_git_config.py', 'uninstall', '--global-config',
                str(global_config)
        ]):
            self.assertEqual(main(), 0)

        self.assertFalse(output.exists())
        self.assertNotIn('[include]', _read(global_config))

    def test_uninstall_with_nothing_installed_is_a_no_op(self):
        global_config = self.tmp / 'gitconfig'
        with mock.patch('sys.argv', [
                'mirror_git_config.py', 'uninstall', '--global-config',
                str(global_config)
        ]):
            self.assertEqual(main(), 0)


if __name__ == '__main__':
    unittest.main()
