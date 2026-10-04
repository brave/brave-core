#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for mirror_git_config.py."""

import contextlib
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import mirror_git_config
from mirror_git_config import (
    build_git_config,
    build_output,
    default_global_configs,
    list_mirror_projects,
    main,
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
            (
                'https://boringssl.googlesource.com/boringssl',
                'https://boringssl.googlesource.com/boringssl.git',
            ),
        )

    def test_nested_path(self):
        self.assertEqual(
            upstream_urls_for('mirror/chromium.googlesource.com/chromium/src'),
            (
                'https://chromium.googlesource.com/chromium/src',
                'https://chromium.googlesource.com/chromium/src.git',
            ),
        )

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
            ['mirror/boringssl.googlesource.com/boringssl'], 'bot'
        )
        self.assertEqual(
            config,
            '[url "ssh://bot@gerrit-ssh.brave.com:29418/mirror/'
            'boringssl.googlesource.com/boringssl"]\n'
            '\tinsteadOf = https://boringssl.googlesource.com/boringssl\n'
            '\tinsteadOf = https://boringssl.googlesource.com/boringssl.git\n',
        )

    def test_skips_unrecognised_project_and_logs(self):
        with self.assertLogs(level='WARNING') as logs:
            config = build_git_config(
                ['not-a-mirror', 'mirror/boringssl.googlesource.com/boringssl'],
                'bot',
            )
        self.assertNotIn('not-a-mirror', config)
        self.assertIn('not-a-mirror', '\n'.join(logs.output))
        self.assertIn('boringssl.googlesource.com/boringssl', config)

    def test_two_projects_get_exact_distinct_urls(self):
        # Sibling repos whose names share a prefix (`foo` / `foo-bar`) must
        # each carry their own exact upstream strings, not a shorter shared
        # prefix that could be ambiguous to a reader of the generated file.
        config = build_git_config(
            ['mirror/example.com/foo', 'mirror/example.com/foo-bar'], 'bot'
        )
        self.assertIn('insteadOf = https://example.com/foo\n', config)
        self.assertIn('insteadOf = https://example.com/foo.git\n', config)
        self.assertIn('insteadOf = https://example.com/foo-bar\n', config)
        self.assertIn('insteadOf = https://example.com/foo-bar.git\n', config)


class TestGitConfigRewritesUrls(unittest.TestCase):
    """End-to-end: the generated config actually redirects git, correctly."""

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.tmp = Path(stack.enter_context(tempfile.TemporaryDirectory()))

    def _make_bare_repo_with_commit(self, path: Path, message: str) -> str:
        subprocess.run(['git', 'init', '-q', '--bare', str(path)], check=True)
        subprocess.run(
            ['git', 'symbolic-ref', 'HEAD', 'refs/heads/main'],
            cwd=path,
            check=True,
        )
        work = path.parent / f'{path.name}.work'
        subprocess.run(
            ['git', 'clone', '-q', str(path), str(work)],
            check=True,
            capture_output=True,
        )
        subprocess.run(
            [
                'git',
                '-c',
                'user.email=t@t.com',
                '-c',
                'user.name=t',
                'commit',
                '-q',
                '--allow-empty',
                '-m',
                message,
            ],
            cwd=work,
            check=True,
        )
        subprocess.run(
            ['git', 'push', '-q', 'origin', 'HEAD:main'], cwd=work, check=True
        )
        return subprocess.run(
            ['git', 'rev-parse', 'HEAD'],
            cwd=work,
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()

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

        # Forward slashes: `\` is an escape in a git config section name, so a
        # native Windows path would be mangled.
        targets = {
            foo_target.as_posix(): upstream_urls_for('mirror/example.com/foo'),
            foobar_target.as_posix(): upstream_urls_for(
                'mirror/example.com/foo-bar'
            ),
        }
        config = ''.join(
            f'[url "{target}"]\n'
            + ''.join(f'\tinsteadOf = {url}\n' for url in urls)
            for target, urls in targets.items()
        )
        generated_cfg = self.tmp / 'generated.gitconfig'
        _write(generated_cfg, config)

        env = {
            **os.environ,
            'GIT_CONFIG_GLOBAL': str(generated_cfg),
            'GIT_CONFIG_NOSYSTEM': '1',
        }
        for url, expected_head in (
            ('https://example.com/foo', foo_head),
            ('https://example.com/foo.git', foo_head),
            ('https://example.com/foo-bar', foobar_head),
            ('https://example.com/foo-bar.git', foobar_head),
        ):
            result = subprocess.run(
                ['git', 'ls-remote', url, 'main'],
                capture_output=True,
                text=True,
                check=True,
                env=env,
            )
            self.assertEqual(
                result.stdout.split()[0],
                expected_head,
                f'{url} resolved to the wrong repo',
            )


class TestDefaultGlobalConfigs(unittest.TestCase):
    """Unit tests for resolving which global config(s) get included."""

    def test_honors_git_config_global_env_override(self):
        with mock.patch.dict(
            os.environ, {'GIT_CONFIG_GLOBAL': '/tmp/custom.gitconfig'}
        ):
            self.assertEqual(
                default_global_configs(), [Path('/tmp/custom.gitconfig')]
            )

    def test_falls_back_to_gits_default_global_files(self):
        with (
            mock.patch.dict(os.environ, {}, clear=True),
            mock.patch.object(Path, 'home', return_value=Path('/home/x')),
        ):
            self.assertEqual(
                default_global_configs(),
                [
                    Path('/home/x/.config/git/config'),
                    Path('/home/x/.gitconfig'),
                ],
            )

    def test_honors_xdg_config_home(self):
        with (
            mock.patch.dict(
                os.environ, {'XDG_CONFIG_HOME': '/xdg'}, clear=True
            ),
            mock.patch.object(Path, 'home', return_value=Path('/home/x')),
        ):
            self.assertEqual(
                default_global_configs(),
                [Path('/xdg/git/config'), Path('/home/x/.gitconfig')],
            )

    def test_ignores_env_pointing_at_our_own_output(self):
        # A caller that already exported GIT_CONFIG_GLOBAL to our file must
        # not make the file include itself.
        with (
            mock.patch.dict(
                os.environ,
                {'GIT_CONFIG_GLOBAL': str(mirror_git_config.OUTPUT_PATH)},
                clear=True,
            ),
            mock.patch.object(Path, 'home', return_value=Path('/home/x')),
        ):
            self.assertEqual(
                default_global_configs(),
                [
                    Path('/home/x/.config/git/config'),
                    Path('/home/x/.gitconfig'),
                ],
            )


class TestBuildOutput(unittest.TestCase):
    """The generated file layers the redirects on top of the global config."""

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.tmp = Path(stack.enter_context(tempfile.TemporaryDirectory()))

    def _git_config(self, output: Path, *args: str) -> str:
        env = {
            **os.environ,
            'GIT_CONFIG_GLOBAL': str(output),
            'GIT_CONFIG_NOSYSTEM': '1',
        }
        return subprocess.run(
            ['git', 'config', '--global', *args],
            capture_output=True,
            text=True,
            check=True,
            env=env,
        ).stdout

    def test_includes_global_configs_then_redirects(self):
        self.assertEqual(
            build_output(
                'bot',
                [Path('/a/one'), Path('/b/two')],
                '[url "x"]\n\tinsteadOf = y\n',
            ),
            '# Generated by mirror_git_config.py install --user bot. '
            'Do not edit.\n[include]\n'
            '\tpath = "/a/one"\n\tpath = "/b/two"\n'
            '[url "x"]\n\tinsteadOf = y\n',
        )

    def test_git_sees_global_settings_and_redirects(self):
        global_config = self.tmp / 'a#b;c.gitconfig'
        _write(global_config, '[user]\n\tname = Test\n')
        output = self.tmp / 'out.gitconfig'
        _write(
            output,
            build_output(
                'bot',
                [global_config, self.tmp / 'missing.gitconfig'],
                build_git_config(
                    ['mirror/boringssl.googlesource.com/boringssl'], 'bot'
                ),
            ),
        )
        self.assertEqual(
            self._git_config(output, '--includes', 'user.name').strip(), 'Test'
        )
        self.assertIn(
            'https://boringssl.googlesource.com/boringssl.git',
            self._git_config(
                output,
                '--get-all',
                'url.ssh://bot@gerrit-ssh.brave.com:29418/mirror/'
                'boringssl.googlesource.com/boringssl.insteadOf',
            ),
        )


class TestListMirrorProjects(unittest.TestCase):
    """Unit tests for the anonymous Gerrit REST project listing."""

    @staticmethod
    def _page(*names: str, more: bool = False) -> str:
        page = {name: {'state': 'ACTIVE'} for name in names}
        if more:
            page[names[-1]]['_more_projects'] = True
        return ")]}'\n" + json.dumps(page)

    def test_success_parses_and_sorts_projects(self):
        body = self._page(
            'mirror/b.googlesource.com/b', 'mirror/a.googlesource.com/a'
        )
        with mock.patch.object(
            mirror_git_config, '_fetch', return_value=body
        ) as fetch:
            projects = list_mirror_projects()
        self.assertEqual(
            projects,
            ['mirror/a.googlesource.com/a', 'mirror/b.googlesource.com/b'],
        )
        self.assertEqual(
            fetch.call_args.args,
            ('https://gerrit.brave.com/projects/?p=mirror%2F&S=0',),
        )

    def test_follows_more_projects_pages(self):
        pages = [
            self._page('mirror/a.com/a', 'mirror/b.com/b', more=True),
            self._page('mirror/c.com/c'),
        ]
        with mock.patch.object(
            mirror_git_config, '_fetch', side_effect=pages
        ) as fetch:
            projects = list_mirror_projects()
        self.assertEqual(
            projects, ['mirror/a.com/a', 'mirror/b.com/b', 'mirror/c.com/c']
        )
        self.assertTrue(fetch.call_args.args[0].endswith('&S=2'))

    def test_empty_listing(self):
        with mock.patch.object(
            mirror_git_config, '_fetch', return_value=")]}'\n{}"
        ):
            self.assertEqual(list_mirror_projects(), [])

    def test_connection_failure_raises(self):
        with mock.patch.object(
            mirror_git_config, '_fetch', side_effect=OSError('unreachable')
        ):
            with self.assertRaises(RuntimeError):
                list_mirror_projects()

    def test_malformed_response_raises(self):
        with mock.patch.object(
            mirror_git_config, '_fetch', return_value='<html>not json</html>'
        ):
            with self.assertRaises(RuntimeError):
                list_mirror_projects()


class TestOutputPath(unittest.TestCase):
    """The generated config lands at the brave-core root."""

    def test_is_at_brave_core_root(self):
        root = Path(__file__).resolve().parents[5]
        self.assertTrue((root / 'DEPS').is_file())
        self.assertEqual(
            mirror_git_config.OUTPUT_PATH,
            root / '.gitconfig_gerrit_mirror_redirect',
        )


class TestMain(unittest.TestCase):
    """Integration tests for the CLI entry point."""

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.tmp = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        self.output = self.tmp / '.gitconfig_gerrit_mirror_redirect'
        self.global_config = self.tmp / 'gitconfig'
        _write(self.global_config, '[user]\n\tname = Test\n')
        for patcher in (
            mock.patch.object(mirror_git_config, 'OUTPUT_PATH', self.output),
            mock.patch.dict(
                os.environ, {'GIT_CONFIG_GLOBAL': str(self.global_config)}
            ),
        ):
            patcher.start()
            self.addCleanup(patcher.stop)

    def _main(self, *argv: str, projects: list[str] | None = None):
        """Run `main` with *argv*; return the `list_mirror_projects` mock."""
        with (
            mock.patch.object(
                mirror_git_config,
                'list_mirror_projects',
                return_value=projects or [],
            ) as listing,
            mock.patch('sys.argv', ['mirror_git_config.py', *argv]),
        ):
            self.assertEqual(main(), 0)
        return listing

    def _install(self, projects: list[str], *extra: str, user: str = 'bot'):
        return self._main('install', '--user', user, *extra, projects=projects)

    def test_install_writes_include_and_redirects(self):
        self._install(['mirror/boringssl.googlesource.com/boringssl'])
        text = _read(self.output)
        self.assertIn(f'path = "{self.global_config.as_posix()}"', text)
        self.assertIn(
            '[url "ssh://bot@gerrit-ssh.brave.com:29418/mirror/', text
        )
        self.assertIn(
            'insteadOf = https://boringssl.googlesource.com/boringssl\n', text
        )

    def test_install_never_touches_the_global_config(self):
        self._install(['mirror/boringssl.googlesource.com/boringssl'])
        self.assertEqual(_read(self.global_config), '[user]\n\tname = Test\n')

    def test_install_leaves_existing_install_alone(self):
        self._install(['mirror/a.com/a'])
        listing = self._install(['mirror/b.com/b'])
        listing.assert_not_called()
        self.assertIn('https://a.com/a', _read(self.output))
        self.assertNotIn('https://b.com/b', _read(self.output))

    def test_install_for_another_user_regenerates(self):
        self._install(['mirror/a.com/a'])
        self._install(['mirror/a.com/a'], user='other')
        self.assertIn('ssh://other@gerrit-ssh.brave.com', _read(self.output))
        self.assertNotIn('ssh://bot@', _read(self.output))

    def test_install_regenerates_file_from_older_version(self):
        _write(
            self.output,
            '# Generated by mirror_git_config.py install. Do not edit.\n',
        )
        self._install(['mirror/a.com/a'])
        self.assertIn('ssh://bot@gerrit-ssh.brave.com', _read(self.output))

    def test_install_with_update_overwrites_existing_install(self):
        self._install(['mirror/a.com/a'])
        self._install(['mirror/b.com/b'], '--update')
        self.assertNotIn('https://a.com/a', _read(self.output))
        self.assertIn('https://b.com/b', _read(self.output))

    def test_uninstall_deletes_output(self):
        self._install(['mirror/a.com/a'])
        self._main('uninstall')
        self.assertFalse(self.output.exists())
        self.assertEqual(_read(self.global_config), '[user]\n\tname = Test\n')

    def test_uninstall_with_nothing_installed_is_a_no_op(self):
        self._main('uninstall')
        self.assertFalse(self.output.exists())


if __name__ == '__main__':
    unittest.main()
