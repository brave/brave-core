#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Unit tests for the tools/cr/bootstrap pure helpers."""

from __future__ import annotations

import contextlib
import io
import os
from pathlib import Path
import platform
import shutil
import stat
import tempfile
import unittest
from unittest import mock

import bootstrap
import launcher


def _make_executable(directory: Path, name: str) -> Path:
    """Create an executable file `name` under `directory` (POSIX exec bits)."""
    directory.mkdir(parents=True, exist_ok=True)
    exe = directory / name
    exe.write_text('#!/bin/sh\n', encoding='utf-8', newline='')
    exe.chmod(exe.stat().st_mode | stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH)
    return exe


def _make_wrapper_dir(directory: Path, *tools: str) -> Path:
    """Create a routing-wrapper dir: the `npm_wrapper.py` sentinel plus `tools`.

    Mirrors `build/npm_wrapper`, whose presence on `$PATH` ahead of our shims
    would loop a naive `npm` fallback back into itself.

    TODO(https://brave.dev/b/57477): remove with the rest of the `npm_wrapper`
    special-casing once `build/npm_wrapper` is gone.
    """
    directory.mkdir(parents=True, exist_ok=True)
    (directory / 'npm_wrapper.py').write_text('#',
                                              encoding='utf-8',
                                              newline='')
    for tool in tools:
        _make_executable(directory, tool)
    return directory


class PosixBlockTest(unittest.TestCase):
    """Exercises the bash/zsh managed-block helpers."""

    DIR = Path('/home/dev/src/brave/tools/cr/bootstrap')

    def test_apply_block_appends_single_block(self):
        result = bootstrap.apply_block('existing line\n', self.DIR)
        self.assertEqual(result.count(bootstrap.BEGIN_MARKER), 1)
        self.assertEqual(result.count(bootstrap.END_MARKER), 1)
        self.assertIn('existing line\n', result)
        self.assertIn(f'export PATH="{self.DIR.as_posix()}:$PATH"', result)

    def test_apply_block_is_idempotent(self):
        once = bootstrap.apply_block('existing line\n', self.DIR)
        twice = bootstrap.apply_block(once, self.DIR)
        self.assertEqual(once, twice)
        self.assertEqual(twice.count(bootstrap.BEGIN_MARKER), 1)

    def test_apply_block_repoints_on_new_dir(self):
        first = bootstrap.apply_block('', Path('/old/bootstrap'))
        second = bootstrap.apply_block(first, Path('/new/bootstrap'))
        self.assertEqual(second.count(bootstrap.BEGIN_MARKER), 1)
        self.assertNotIn('/old/bootstrap', second)
        self.assertIn('/new/bootstrap', second)

    def test_remove_block_round_trips(self):
        base = 'line one\nline two\n'
        self.assertEqual(
            bootstrap.remove_block(bootstrap.apply_block(base, self.DIR)),
            base)

    def test_remove_block_no_op_without_block(self):
        text = 'nothing to see here\n'
        self.assertEqual(bootstrap.remove_block(text), text)

    def test_apply_block_on_empty(self):
        result = bootstrap.apply_block('', self.DIR)
        self.assertTrue(result.startswith(bootstrap.BEGIN_MARKER))
        self.assertTrue(result.endswith('\n'))


class FishDropInTest(unittest.TestCase):
    """Exercises the fish conf.d drop-in helper."""

    def test_prepends_path_without_fish_add_path(self):
        content = bootstrap.fish_drop_in(Path('/a/b/bootstrap'))
        # Prepends to $PATH and drops any prior occurrence (lands first, once).
        self.assertIn(
            'set -gx PATH "/a/b/bootstrap" '
            '(string match --invert -- "/a/b/bootstrap" $PATH)', content)
        # Must NOT use fish_add_path: it persists universal state that would
        # survive uninstall (deleting this file).
        self.assertNotIn('fish_add_path', content)
        self.assertIn('Managed by', content)

    def test_drop_in_sorts_after_version_managers(self):
        # fish sources conf.d/*.fish alphabetically; our drop-in must sort
        # *after* version-manager snippets (e.g. fnm) so it prepends last and
        # wins. Guard the name against a regression to an earlier-sorting one.
        name = bootstrap._FISH_DROP_IN.name
        self.assertGreater(name, 'fnm.fish')
        self.assertGreater(name, 'nvm.fish')
        # Upgrades must clean up the old, too-early name.
        self.assertLess(bootstrap._FISH_DROP_IN_LEGACY.name, 'fnm.fish')


class WindowsPathTest(unittest.TestCase):
    """Exercises the Windows PATH add/remove helpers."""

    DIR = Path(r'C:\dev\src\brave\tools\cr\bootstrap')

    def test_add_prepends_first(self):
        result = bootstrap.add_windows_entry(r'C:\Windows;C:\Tools', self.DIR)
        self.assertEqual(result, rf'{self.DIR};C:\Windows;C:\Tools')

    def test_add_moves_existing_to_front(self):
        # Present (lowercased, trailing slash) but not first -> moved to front
        # exactly once, in canonical form.
        current = rf'C:\Windows;{str(self.DIR).lower()}\\;C:\Tools'
        self.assertEqual(bootstrap.add_windows_entry(current, self.DIR),
                         rf'{self.DIR};C:\Windows;C:\Tools')

    def test_add_is_idempotent(self):
        once = bootstrap.add_windows_entry(r'C:\Windows', self.DIR)
        self.assertEqual(bootstrap.add_windows_entry(once, self.DIR), once)

    def test_add_to_empty(self):
        self.assertEqual(bootstrap.add_windows_entry('', self.DIR),
                         str(self.DIR))

    def test_remove_drops_entry(self):
        current = rf'C:\Windows;{self.DIR};C:\Tools'
        self.assertEqual(bootstrap.remove_windows_entry(current, self.DIR),
                         r'C:\Windows;C:\Tools')

    def test_remove_no_op(self):
        current = r'C:\Windows;C:\Tools'
        self.assertEqual(bootstrap.remove_windows_entry(current, self.DIR),
                         current)


class FindExistingBootstrapTest(unittest.TestCase):
    """Exercises install detection via the marker shim on `$PATH`."""

    def setUp(self):
        self._saved_path = os.environ.get('PATH', '')

    def tearDown(self):
        os.environ['PATH'] = self._saved_path

    def test_none_when_not_on_path(self):
        os.environ['PATH'] = ''
        self.assertIsNone(bootstrap.find_existing_bootstrap())

    @unittest.skipIf(platform.system() == 'Windows',
                     'POSIX exec-bit lookup; Windows uses PATHEXT')
    def test_found_when_shim_on_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            shim_path = Path(tmp) / bootstrap._INSTALL_MARKER
            shim_path.write_text('#!/bin/sh\n', encoding='utf-8', newline='')
            shim_path.chmod(shim_path.stat().st_mode | stat.S_IXUSR)
            os.environ['PATH'] = tmp
            self.assertEqual(bootstrap.find_existing_bootstrap(),
                             str(shim_path))

    def test_installed_dir_falls_back_to_own_dir(self):
        os.environ['PATH'] = ''
        self.assertEqual(bootstrap.installed_bootstrap_dir(),
                         bootstrap.BOOTSTRAP_DIR)

    @unittest.skipIf(platform.system() == 'Windows',
                     'POSIX exec-bit lookup; Windows uses PATHEXT')
    def test_installed_dir_is_parent_of_on_path_shim(self):
        # uninstall must target wherever the live shim actually resides, which
        # may differ from this checkout's BOOTSTRAP_DIR.
        with tempfile.TemporaryDirectory() as tmp:
            other = Path(tmp).resolve() / 'other-checkout'
            other.mkdir()
            shim_path = other / bootstrap._INSTALL_MARKER
            shim_path.write_text('#!/bin/sh\n', encoding='utf-8', newline='')
            shim_path.chmod(shim_path.stat().st_mode | stat.S_IXUSR)
            os.environ['PATH'] = str(other)
            self.assertEqual(bootstrap.installed_bootstrap_dir(), other)


class ResolveVpython3Test(unittest.TestCase):
    """Exercises `launcher._resolve_vpython3` interpreter selection."""

    def test_resolves_to_a_vpython3_interpreter(self):
        # pylint: disable=protected-access
        # Whether found on $PATH or via the vendored depot_tools fallback, the
        # resolved interpreter is always a vpython3 (vpython3.bat on Windows).
        # shutil.which() mirrors the case of the matched PATHEXT entry (often
        # ".BAT" on Windows), so compare case-insensitively.
        resolved = launcher._resolve_vpython3(Path('/home/dev/src/brave'))
        self.assertIn(resolved.name.lower(), ('vpython3', 'vpython3.bat'))




class ResolveSystemBinaryTest(unittest.TestCase):
    """Exercises `launcher._resolve_system_binary`'s self-exclusion.

    The shim directory sits first on `$PATH`; the resolver must skip it so the
    node/npm shims never recurse into themselves and instead find the real
    system tool.
    """

    def _make_exe(self, directory: Path, name: str) -> Path:
        directory.mkdir(parents=True, exist_ok=True)
        exe = directory / name
        exe.write_text('#!/bin/sh\n', encoding='utf-8', newline='')
        exe.chmod(exe.stat().st_mode | stat.S_IEXEC | stat.S_IXGRP
                  | stat.S_IXOTH)
        return exe

    def test_skips_excluded_shim_dir(self):
        if platform.system() == 'Windows':
            self.skipTest('POSIX exec-bit semantics')
        with tempfile.TemporaryDirectory() as tmp:
            shim = Path(tmp) / 'shim'
            real = Path(tmp) / 'real'
            self._make_exe(shim, 'node')  # would be picked without exclusion
            real_node = self._make_exe(real, 'node')
            old_path = os.environ.get('PATH', '')
            os.environ['PATH'] = os.pathsep.join([str(shim), str(real)])
            try:
                # pylint: disable=protected-access
                resolved = launcher._resolve_system_binary('node',
                                                           exclude_dir=shim)
            finally:
                os.environ['PATH'] = old_path
            self.assertEqual(Path(resolved).resolve(), real_node.resolve())

    def test_returns_none_when_absent(self):
        with tempfile.TemporaryDirectory() as tmp:
            old_path = os.environ.get('PATH', '')
            os.environ['PATH'] = str(Path(tmp) / 'empty')
            try:
                # pylint: disable=protected-access
                self.assertIsNone(
                    launcher._resolve_system_binary('definitely-not-a-tool'))
            finally:
                os.environ['PATH'] = old_path


class IsWrapperDirTest(unittest.TestCase):
    """Exercises `launcher._is_wrapper_dir` sentinel detection.

    TODO(https://brave.dev/b/57477): remove with the rest of the `npm_wrapper`
    special-casing once `build/npm_wrapper` is gone.
    """

    def test_true_when_sentinel_present(self):
        with tempfile.TemporaryDirectory() as tmp:
            wrapper = _make_wrapper_dir(Path(tmp) / 'npm_wrapper')
            self.assertTrue(launcher._is_wrapper_dir(wrapper))

    def test_false_for_plain_dir(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertFalse(launcher._is_wrapper_dir(Path(tmp)))

    def test_false_for_missing_dir(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertFalse(launcher._is_wrapper_dir(Path(tmp) / 'nope'))

    def test_recognizes_every_declared_sentinel(self):
        # Contract guard: each declared sentinel marks a dir as a wrapper.
        self.assertTrue(launcher._WRAPPER_SENTINELS)
        for sentinel in launcher._WRAPPER_SENTINELS:
            with tempfile.TemporaryDirectory() as tmp:
                (Path(tmp) / sentinel).write_text('#',
                                                  encoding='utf-8',
                                                  newline='')
                self.assertTrue(launcher._is_wrapper_dir(Path(tmp)), sentinel)


@unittest.skipIf(platform.system() == 'Windows',
                 'POSIX exec-bit lookup; Windows uses PATHEXT')
class SystemBinarySkipsWrapperTest(unittest.TestCase):
    """`_resolve_system_binary` must never fall back into a routing wrapper.

    In CI the `$PATH` layout is [npm_wrapper, our shims, system] -- npm_wrapper
    first. When our `npm` shim finds no checkout-local binary and falls back, it
    must resolve the real system `npm` and skip npm_wrapper; otherwise the
    wrapper would re-invoke our shim, which would fall back to the wrapper
    again, ping-ponging forever. The special-casing is `npm`-only (the wrapper
    shadows `npm` alone).

    TODO(https://brave.dev/b/57477): remove with the rest of the `npm_wrapper`
    special-casing once `build/npm_wrapper` is gone.
    """

    def setUp(self):
        self._saved_path = os.environ.get('PATH', '')

    def tearDown(self):
        os.environ['PATH'] = self._saved_path

    def _set_path(self, *dirs: Path) -> None:
        os.environ['PATH'] = os.pathsep.join(str(d) for d in dirs)

    def test_skips_wrapper_and_finds_real_binary(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            shim = root / 'shim'
            shim.mkdir()
            wrapper = _make_wrapper_dir(root / 'wrapper', 'npm')
            real = _make_executable(root / 'system', 'npm')
            self._set_path(shim, wrapper, root / 'system')
            resolved = launcher._resolve_system_binary('npm', exclude_dir=shim)
            self.assertEqual(Path(resolved).resolve(), real.resolve())

    def test_returns_none_when_only_wrapper_has_it(self):
        # The tool exists only in the wrapper: refuse to loop -> resolve to
        # nothing so the caller reports no system binary and stops.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            shim = root / 'shim'
            shim.mkdir()
            wrapper = _make_wrapper_dir(root / 'wrapper', 'npm')
            self._set_path(shim, wrapper)
            self.assertIsNone(
                launcher._resolve_system_binary('npm', exclude_dir=shim))

    def test_skips_both_shim_and_wrapper(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            shim = root / 'shim'
            _make_executable(shim, 'npm')  # our own shim, excluded by dir
            wrapper = _make_wrapper_dir(root / 'wrapper', 'npm')
            real = _make_executable(root / 'system', 'npm')
            self._set_path(shim, wrapper, root / 'system')
            resolved = launcher._resolve_system_binary('npm', exclude_dir=shim)
            self.assertEqual(Path(resolved).resolve(), real.resolve())

    def test_plain_dir_with_tool_is_not_skipped(self):
        # Only wrapper dirs are excluded; a normal dir that merely holds `npm`
        # (no sentinel) is used as usual.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            shim = root / 'shim'
            shim.mkdir()
            plain = _make_executable(root / 'plain', 'npm')
            self._set_path(shim, root / 'plain')
            resolved = launcher._resolve_system_binary('npm', exclude_dir=shim)
            self.assertEqual(Path(resolved).resolve(), plain.resolve())

    def test_non_npm_tool_does_not_skip_wrapper(self):
        # The special-casing is npm-only. A non-npm tool (here pnpm) is resolved
        # normally, even out of a wrapper dir -- proving the guard's scope. In
        # practice npm_wrapper only ever shadows `npm`, so this never bites; the
        # test pins the scope so the guard can't silently widen.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            shim = root / 'shim'
            shim.mkdir()
            wrapper = _make_wrapper_dir(root / 'wrapper', 'pnpm')
            self._set_path(shim, wrapper)
            resolved = launcher._resolve_system_binary('pnpm',
                                                       exclude_dir=shim)
            self.assertEqual(
                Path(resolved).resolve(), (wrapper / 'pnpm').resolve())

    def test_wrapper_before_and_after_shim_both_skipped(self):
        # Defensive: a wrapper dir anywhere on $PATH is skipped, not just first.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            shim = root / 'shim'
            shim.mkdir()
            first = _make_wrapper_dir(root / 'w1', 'npm')
            second = _make_wrapper_dir(root / 'w2', 'npm')
            real = _make_executable(root / 'system', 'npm')
            self._set_path(first, shim, second, root / 'system')
            resolved = launcher._resolve_system_binary('npm', exclude_dir=shim)
            self.assertEqual(Path(resolved).resolve(), real.resolve())


@unittest.skipIf(platform.system() == 'Windows',
                 'POSIX exec-bit lookup; Windows uses PATHEXT')
class ResolveInvocationWrapperFallbackTest(unittest.TestCase):
    """End-to-end: with npm_wrapper ahead of our shims on `$PATH`, an `npm`
    fallback from `resolve_invocation` lands on the real system binary, never
    the wrapper -- so our shims can coexist with a higher-priority npm_wrapper
    without ping-ponging.

    TODO(https://brave.dev/b/57477): remove with the rest of the `npm_wrapper`
    special-casing once `build/npm_wrapper` is gone.
    """

    def setUp(self):
        self._saved_path = os.environ.get('PATH', '')

    def tearDown(self):
        os.environ['PATH'] = self._saved_path

    def _own_shim_dir(self) -> Path:
        # resolve_invocation's fallback excludes launcher.py's own directory;
        # the CI layout puts that real shim dir between wrapper and system.
        return Path(launcher.__file__).parent.resolve()

    def _ci_layout(self, root: Path, *, real_npm: bool = True):
        # [npm_wrapper, our real shim dir, system] -- npm_wrapper highest.
        wrapper = _make_wrapper_dir(root / 'npm_wrapper', 'npm')
        system = root / 'system'
        system.mkdir(parents=True, exist_ok=True)
        real = _make_executable(system, 'npm') if real_npm else None
        os.environ['PATH'] = os.pathsep.join(
            [str(wrapper),
             str(self._own_shim_dir()),
             str(system)])
        return real

    def test_npm_fallback_resolves_real_npm_not_wrapper(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            real = self._ci_layout(root)
            # A checkout with no local npm target forces the fallback path.
            checkout = root / 'src' / 'brave'
            invocation = launcher.resolve_invocation('npm', checkout, True)
            self.assertIsNotNone(invocation)
            self.assertEqual(Path(invocation[0]).resolve(), real.resolve())

    def test_npm_fallback_none_when_only_wrapper(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._ci_layout(root, real_npm=False)
            checkout = root / 'src' / 'brave'
            # No real npm anywhere but the wrapper -> refuse to loop -> None.
            self.assertIsNone(
                launcher.resolve_invocation('npm', checkout, True))


class FindShimTargetTest(unittest.TestCase):
    """Exercises `launcher.find_shim_target` token-to-entry resolution."""

    def test_qualified_name_is_used_as_is(self):
        # A shim that knows its platform passes a key listed verbatim (the
        # `.bat` variants pass `node-win` / `npm-win`).
        self.assertEqual(launcher.find_shim_target('node-win'),
                         launcher.SHIM_TARGETS['node-win'])
        self.assertEqual(launcher.find_shim_target('npm-mac_arm64'),
                         launcher.SHIM_TARGETS['npm-mac_arm64'])

    def test_listed_token_is_used_as_is(self):
        self.assertEqual(launcher.find_shim_target('brockit'),
                         launcher.SHIM_TARGETS['brockit'])

    def test_git_cr_token_resolves_to_cmd(self):
        # git resolves `git cr` to the `git-cr` shim, which passes `git-cr`
        # verbatim; it must map to the git_cr command entry point.
        target = launcher.find_shim_target('git-cr')
        self.assertEqual(target, launcher.SHIM_TARGETS['git-cr'])
        self.assertTrue(target.path.endswith('alias/cmd.py'))

    def test_bare_family_gets_host_suffix(self):
        # A POSIX shim passes the bare family; the launcher appends the host.
        key = launcher.host_platform_key()
        if key is None:
            self.skipTest('unsupported host platform')
        self.assertEqual(launcher.find_shim_target('node'),
                         launcher.SHIM_TARGETS[f'node-{key}'])

    def test_pnpm_is_a_single_cross_platform_node_shim(self):
        # pnpm lives in the one node_modules tree (no per-platform suffix), so
        # the bare token resolves directly and runs as a node script that
        # self-updates from the node_modules EXTRA_DEPS entry.
        target = launcher.find_shim_target('pnpm')
        self.assertEqual(target, launcher.SHIM_TARGETS['pnpm'])
        self.assertEqual(target.runtime, 'node')
        self.assertTrue(target.path.endswith('node_modules/pnpm/bin/pnpm.mjs'))
        self.assertEqual(target.self_update_extra_dep_entry,
                         'src/brave/third_party/node/node_modules')

    def test_unknown_token_raises(self):
        with self.assertRaises(launcher.UnknownShimError):
            launcher.find_shim_target('bogus')


class ResolveInvocationTest(unittest.TestCase):
    """Exercises `launcher.resolve_invocation` against a faked checkout tree."""

    def _key(self) -> str:
        key = launcher.host_platform_key()
        if key is None:
            self.skipTest('unsupported host platform')
        return key

    def _checkout(self, root: Path) -> Path:
        # resolve_invocation treats checkout.parent.parent as the workspace
        # root, joining the src/brave-prefixed target paths onto it.
        return root / 'src' / 'brave'

    def _make_target(self, root: Path, key: str) -> Path:
        target = root / launcher.SHIM_TARGETS[key].path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text('', encoding='utf-8', newline='')
        return target

    def test_resolves_repo_node(self):
        with tempfile.TemporaryDirectory() as tmp:
            root, key = Path(tmp), self._key()
            node = self._make_target(root, f'node-{key}')
            invocation = launcher.resolve_invocation(f'node-{key}',
                                                     self._checkout(root),
                                                     False)
            self.assertEqual(invocation, [str(node)])

    def test_npm_runs_through_node_on_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            root, key = Path(tmp), self._key()
            npm_cli = self._make_target(root, f'npm-{key}')
            # npm runs as `<node from $PATH> npm-cli.js` — never bare npm.
            with mock.patch.object(launcher.shutil,
                                   'which',
                                   return_value='/usr/bin/node'):
                invocation = launcher.resolve_invocation(
                    f'npm-{key}', self._checkout(root), True)
            self.assertEqual(invocation, ['/usr/bin/node', str(npm_cli)])

    def test_vpython_tool_runs_through_vpython3(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._make_target(root, 'brockit')
            invocation = launcher.resolve_invocation('brockit',
                                                     self._checkout(root),
                                                     False)
            self.assertIsNotNone(invocation)
            # brockit is launched as: <vpython3> <brockit.py>.
            self.assertEqual(len(invocation), 2)
            self.assertTrue(invocation[1].endswith('brockit.py'))

    def test_git_cr_runs_through_vpython3(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._make_target(root, 'git-cr')
            invocation = launcher.resolve_invocation('git-cr',
                                                     self._checkout(root),
                                                     False)
            self.assertIsNotNone(invocation)
            # git-cr is launched as: <vpython3> <cmd.py>.
            self.assertEqual(len(invocation), 2)
            self.assertTrue(invocation[1].endswith('cmd.py'))

    def test_falls_back_to_system_when_allowed(self):
        with tempfile.TemporaryDirectory() as tmp:
            # No checkout-local node, but fallback is allowed → system node.
            with mock.patch.object(launcher.shutil,
                                   'which',
                                   return_value='/usr/bin/node'):
                invocation = launcher.resolve_invocation(
                    'node', self._checkout(Path(tmp)), True)
            self.assertEqual(invocation, ['/usr/bin/node'])

    def test_none_without_target_and_no_fallback(self):
        with tempfile.TemporaryDirectory() as tmp:
            # Known tool, no checkout-local target, no fallback → None.
            self.assertIsNone(
                launcher.resolve_invocation(f'node-{self._key()}',
                                            self._checkout(Path(tmp)), False))

    def test_unknown_tool_always_raises(self):
        # A token that is not a shim is rejected outright — fallback or not.
        with self.assertRaises(launcher.UnknownShimError):
            launcher.resolve_invocation('bogus', None, False)
        with self.assertRaises(launcher.UnknownShimError):
            launcher.resolve_invocation('bogus', None, True)

    def _make_installer(self, checkout: Path) -> None:
        """Create an empty tarball_installer.py so the bootstrap is
        attempted."""
        installer = checkout / 'tools' / 'cr' / 'tarball_installer.py'
        installer.parent.mkdir(parents=True, exist_ok=True)
        installer.write_text('', encoding='utf-8', newline='')

    def test_bootstraps_missing_node_then_resolves(self):
        # A missing node target with a known self_update_extra_dep_entry
        # triggers a download (install_extra_deps.py), after which the
        # vendored node resolves.
        with tempfile.TemporaryDirectory() as tmp:
            root, key = Path(tmp), self._key()
            checkout = self._checkout(root)
            self._make_installer(checkout)
            dep = launcher.SHIM_TARGETS[
                f'node-{key}'].self_update_extra_dep_entry

            def _fake_download(_argv):
                # Simulate the installer deploying the node target.
                self._make_target(root, f'node-{key}')
                return 0

            with mock.patch.object(launcher.SelfUpdater,
                                   '_load_extra_deps',
                                   return_value=self._fake_extra_deps(
                                       dep, deployed=False)):
                with mock.patch.object(launcher.subprocess,
                                       'call',
                                       side_effect=_fake_download) as call:
                    with contextlib.redirect_stderr(io.StringIO()):
                        invocation = launcher.resolve_invocation(
                            f'node-{key}', checkout, False)
            call.assert_called_once()
            self.assertEqual(
                invocation,
                [str(root / launcher.SHIM_TARGETS[f'node-{key}'].path)])

    def test_missing_node_falls_back_when_download_deploys_nothing(self):
        # The bootstrap is attempted but deploys no node; with fallback allowed
        # the system node is used instead.
        with tempfile.TemporaryDirectory() as tmp:
            root, key = Path(tmp), self._key()
            checkout = self._checkout(root)
            self._make_installer(checkout)
            dep = launcher.SHIM_TARGETS[
                f'node-{key}'].self_update_extra_dep_entry
            with mock.patch.object(launcher.SelfUpdater,
                                   '_load_extra_deps',
                                   return_value=self._fake_extra_deps(
                                       dep, deployed=False)):
                with mock.patch.object(launcher.subprocess,
                                       'call',
                                       return_value=1) as call:
                    with mock.patch.object(launcher.shutil,
                                           'which',
                                           return_value='/usr/bin/node'):
                        with contextlib.redirect_stderr(io.StringIO()):
                            invocation = launcher.resolve_invocation(
                                'node', checkout, True)
            call.assert_called_once()
            self.assertEqual(invocation, ['/usr/bin/node'])

    def test_no_bootstrap_without_installer(self):
        # With no install_extra_deps.py present the bootstrap is a no-op, and
        # with no fallback the result is None.
        with tempfile.TemporaryDirectory() as tmp:
            root, key = Path(tmp), self._key()
            with mock.patch.object(launcher.subprocess, 'call') as call:
                self.assertIsNone(
                    launcher.resolve_invocation(f'node-{key}',
                                                self._checkout(root), False))
            call.assert_not_called()

    def test_no_bootstrap_when_checkout_lacks_extra_deps(self):
        # An older/divergent checkout whose install_extra_deps.py predates
        # `extra_deps.py` must NOT be handed this shim's baked-in key (its
        # installer may use different EXTRA_DEPS keys): no bootstrap is tried,
        # and with fallback allowed the system tool is used.
        if launcher.host_platform_key() is None:
            self.skipTest('no node shim on this host')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            checkout = self._checkout(root)
            self._make_installer(checkout)  # installer present, extra_deps not
            with mock.patch.object(launcher.subprocess, 'call') as call:
                with mock.patch.object(launcher.shutil,
                                       'which',
                                       return_value='/usr/bin/node'):
                    invocation = launcher.resolve_invocation(
                        'node', checkout, True)
            call.assert_not_called()
            self.assertEqual(invocation, ['/usr/bin/node'])

    def test_vpython_tool_not_bootstrapped(self):
        # Only node/npm carry an self_update_extra_dep_entry; a missing
        # vpython tool is never bootstrapped (it ships in the repo, it is
        # not downloaded).
        with tempfile.TemporaryDirectory() as tmp:
            with mock.patch.object(launcher.subprocess, 'call') as call:
                launcher.resolve_invocation('brockit',
                                            self._checkout(Path(tmp)), False)
            call.assert_not_called()

    def _fake_extra_deps(self, dep: str, deployed: bool) -> mock.Mock:
        """A stand-in `extra_deps` module reporting `deployed` for `dep`."""
        module = mock.Mock()
        module.EXTRA_DEPS = {dep: {'objects': [{}]}}
        module.check_extra_deps_installed.return_value = deployed
        return module

    def test_stale_version_triggers_bootstrap_even_when_present(self):
        # The node binary exists, but the sidecar/version check says it is not
        # the pinned version -> bootstrap runs anyway (a bare is_file() would
        # wrongly skip it).
        with tempfile.TemporaryDirectory() as tmp:
            root, key = Path(tmp), self._key()
            checkout = self._checkout(root)
            self._make_installer(checkout)
            self._make_target(root, f'node-{key}')  # present but "stale"
            dep = launcher.SHIM_TARGETS[
                f'node-{key}'].self_update_extra_dep_entry
            module = self._fake_extra_deps(dep, deployed=False)
            with mock.patch.object(launcher.SelfUpdater,
                                   '_load_extra_deps',
                                   return_value=module):
                with mock.patch.object(launcher.subprocess,
                                       'call',
                                       return_value=0) as call:
                    with contextlib.redirect_stderr(io.StringIO()):
                        invocation = launcher.resolve_invocation(
                            f'node-{key}', checkout, False)
            call.assert_called_once()
            self.assertEqual(
                invocation,
                [str(root / launcher.SHIM_TARGETS[f'node-{key}'].path)])

    def test_pinned_version_deployed_skips_bootstrap(self):
        # The version check says the pinned node is deployed -> no bootstrap.
        with tempfile.TemporaryDirectory() as tmp:
            root, key = Path(tmp), self._key()
            checkout = self._checkout(root)
            self._make_installer(checkout)
            self._make_target(root, f'node-{key}')
            dep = launcher.SHIM_TARGETS[
                f'node-{key}'].self_update_extra_dep_entry
            module = self._fake_extra_deps(dep, deployed=True)
            with mock.patch.object(launcher.SelfUpdater,
                                   '_load_extra_deps',
                                   return_value=module):
                with mock.patch.object(launcher.subprocess, 'call') as call:
                    invocation = launcher.resolve_invocation(
                        f'node-{key}', checkout, False)
            call.assert_not_called()
            self.assertEqual(
                invocation,
                [str(root / launcher.SHIM_TARGETS[f'node-{key}'].path)])

    def test_load_extra_deps_reads_checkout_module(self):
        # `SelfUpdater._load_extra_deps` loads the module by path from the
        # governing checkout (not the launcher's own), stdlib-only.
        with tempfile.TemporaryDirectory() as tmp:
            checkout = self._checkout(Path(tmp))
            module_path = checkout / 'tools' / 'cr' / 'extra_deps.py'
            module_path.parent.mkdir(parents=True, exist_ok=True)
            module_path.write_text(
                'EXTRA_DEPS = {"x": 1}\n'
                'def check_extra_deps_installed(root, path):\n'
                '    return True\n',
                encoding='utf-8',
                newline='')
            module = launcher.SelfUpdater(checkout, 'src/x')._load_extra_deps()
            self.assertIsNotNone(module)
            self.assertEqual(module.EXTRA_DEPS, {'x': 1})

    def test_load_extra_deps_missing_returns_none(self):
        with tempfile.TemporaryDirectory() as tmp:
            updater = launcher.SelfUpdater(self._checkout(Path(tmp)), 'src/x')
            self.assertIsNone(updater._load_extra_deps())

    def test_needs_update_false_when_entry_absent_from_table(self):
        # An entry the checkout's EXTRA_DEPS does not pin raises KeyError from
        # check_extra_deps_installed; needs_update() swallows it and self-update
        # is not attempted.
        updater = launcher.SelfUpdater(Path('/ws/src/brave'), 'src/absent')
        module = mock.Mock()
        module.check_extra_deps_installed.side_effect = KeyError('src/absent')
        with mock.patch.object(launcher.SelfUpdater,
                               '_load_extra_deps',
                               return_value=module):
            self.assertFalse(updater.needs_update())


class ResolveCheckoutTest(unittest.TestCase):
    """Exercises `launcher._resolve_checkout`: env-vs-cwd precedence, and the
    cwd-upward search for the first ancestor `src/brave` carrying our
    sentinel (by layout, not git) -- covering being inside the checkout,
    inside a nested DEPS repo, above `src/brave`, and beside it as a sibling
    chromium dir.
    """

    ENV_VAR = launcher._CHECKOUT_ENV_VAR

    def setUp(self):
        self._saved = os.environ.get(self.ENV_VAR)
        os.environ.pop(self.ENV_VAR, None)

    def tearDown(self):
        if self._saved is None:
            os.environ.pop(self.ENV_VAR, None)
        else:
            os.environ[self.ENV_VAR] = self._saved

    def _make_checkout(self, root: Path) -> Path:
        checkout = root / 'src' / 'brave'
        sentinel = checkout / 'tools' / 'cr' / 'bootstrap' / 'launcher.py'
        sentinel.parent.mkdir(parents=True, exist_ok=True)
        sentinel.write_text('', encoding='utf-8', newline='')
        return checkout

    def _resolve_from_cwd(self, start: Path) -> Path | None:
        with mock.patch.object(launcher.Path, 'cwd', return_value=start):
            return launcher._resolve_checkout()

    def test_resolves_from_inside_the_checkout(self):
        with tempfile.TemporaryDirectory() as tmp:
            checkout = self._make_checkout(Path(tmp))
            inside = checkout / 'components' / 'foo'
            inside.mkdir(parents=True)
            self.assertEqual(
                self._resolve_from_cwd(inside).resolve(), checkout.resolve())

    def test_resolves_from_nested_deps_repo(self):
        with tempfile.TemporaryDirectory() as tmp:
            checkout = self._make_checkout(Path(tmp))
            nested = checkout / 'vendor' / 'web-discovery-project' / 'src'
            nested.mkdir(parents=True)
            self.assertEqual(
                self._resolve_from_cwd(nested).resolve(), checkout.resolve())

    def test_resolves_from_above_src_brave(self):
        # The `npm run init` case: cwd is the workspace root, src/brave a child.
        with tempfile.TemporaryDirectory() as tmp:
            checkout = self._make_checkout(Path(tmp))
            self.assertEqual(
                self._resolve_from_cwd(Path(tmp)).resolve(),
                checkout.resolve())

    def test_resolves_from_sibling_chromium_dir(self):
        # A chromium dir like src/chrome resolves to the sibling src/brave.
        with tempfile.TemporaryDirectory() as tmp:
            checkout = self._make_checkout(Path(tmp))
            chrome = Path(tmp) / 'src' / 'chrome'
            chrome.mkdir(parents=True)
            self.assertEqual(
                self._resolve_from_cwd(chrome).resolve(), checkout.resolve())

    def test_none_outside_any_workspace(self):
        with tempfile.TemporaryDirectory() as tmp:
            outside = Path(tmp) / 'elsewhere'
            outside.mkdir()
            self.assertIsNone(self._resolve_from_cwd(outside))

    def test_requires_sentinel(self):
        with tempfile.TemporaryDirectory() as tmp:
            # A bare src/brave without the bootstrap sentinel is not a checkout.
            (Path(tmp) / 'src' / 'brave').mkdir(parents=True)
            self.assertIsNone(self._resolve_from_cwd(Path(tmp)))

    def test_prefers_env_over_cwd(self):
        # A valid recorded checkout wins even when the cwd resolves to a
        # different one -- it names the checkout that started this chain of
        # shim invocations, which nested calls must keep resolving to.
        with tempfile.TemporaryDirectory() as tmp:
            env_checkout = self._make_checkout(Path(tmp) / 'env')
            cwd_checkout = self._make_checkout(Path(tmp) / 'cwd')
            os.environ[self.ENV_VAR] = str(env_checkout)
            checkout = self._resolve_from_cwd(cwd_checkout)
        self.assertEqual(checkout, env_checkout)

    def test_falls_back_to_cwd_when_env_unset(self):
        # No (valid) recorded checkout -- this is the first invocation in the
        # chain, so resolve from the cwd instead.
        with tempfile.TemporaryDirectory() as tmp:
            cwd_checkout = self._make_checkout(Path(tmp))
            checkout = self._resolve_from_cwd(cwd_checkout)
        # `_resolve_checkout()` always resolves the cwd before searching, so
        # the returned checkout is in resolved (long-path, on Windows) form.
        self.assertEqual(checkout, cwd_checkout.resolve())

    def test_falls_back_to_cwd_when_recorded_checkout_is_stale(self):
        # The recorded value no longer carries our sentinel (e.g. deleted) --
        # don't trust it blindly, fall back to the cwd like the env var was
        # never set.
        with tempfile.TemporaryDirectory() as tmp:
            os.environ[self.ENV_VAR] = str(
                Path(tmp) / 'stale' / 'src' / 'brave')
            cwd_checkout = self._make_checkout(Path(tmp) / 'cwd')
            checkout = self._resolve_from_cwd(cwd_checkout)
        # See test_falls_back_to_cwd_when_env_unset: the cwd fallback path
        # always returns a resolved path.
        self.assertEqual(checkout, cwd_checkout.resolve())

    def test_none_when_neither_resolves(self):
        with tempfile.TemporaryDirectory() as tmp:
            outside = Path(tmp) / 'elsewhere'
            outside.mkdir()
            self.assertIsNone(self._resolve_from_cwd(outside))

    def test_none_when_env_var_empty(self):
        os.environ[self.ENV_VAR] = ''
        with tempfile.TemporaryDirectory() as tmp:
            outside = Path(tmp) / 'elsewhere'
            outside.mkdir()
            self.assertIsNone(self._resolve_from_cwd(outside))


class MainPropagatesCheckoutEnvTest(unittest.TestCase):
    """End-to-end: `main` records the resolved checkout in `$BRAVE_LAUNCHER_
    CHECKOUT` for the subprocess it launches, so a grandchild shim invocation
    made from outside any checkout (an npm/pnpm lifecycle script running in a
    package manager's temp extraction dir, say) still resolves it -- instead of
    failing with "no checkout-local binary found and no <tool> on $PATH".
    """

    ENV_VAR = launcher._CHECKOUT_ENV_VAR

    def setUp(self):
        self._saved = os.environ.get(self.ENV_VAR)

    def tearDown(self):
        if self._saved is None:
            os.environ.pop(self.ENV_VAR, None)
        else:
            os.environ[self.ENV_VAR] = self._saved

    def _make_checkout(self, root: Path) -> Path:
        checkout = root / 'src' / 'brave'
        sentinel = checkout / 'tools' / 'cr' / 'bootstrap' / 'launcher.py'
        sentinel.parent.mkdir(parents=True, exist_ok=True)
        sentinel.write_text('', encoding='utf-8', newline='')
        return checkout

    def test_child_env_carries_the_resolved_checkout(self):
        key = launcher.host_platform_key()
        if key is None:
            self.skipTest('unsupported host platform')
        with tempfile.TemporaryDirectory() as tmp:
            # `_resolve_checkout()` resolves the cwd before searching, so
            # `root` is resolved here to match the (possibly long-form, on
            # Windows) path `main()` will actually record and build paths
            # from.
            root = Path(tmp).resolve()
            checkout = self._make_checkout(root)
            node = root / launcher.SHIM_TARGETS[f'node-{key}'].path
            node.parent.mkdir(parents=True, exist_ok=True)
            node.write_text('', encoding='utf-8', newline='')

            os.environ.pop(self.ENV_VAR, None)
            seen = {}

            def _capture(argv, env=None):
                seen['argv'] = argv
                seen['env'] = env
                return 0

            with mock.patch.object(launcher.Path, 'cwd',
                                   return_value=checkout):
                with mock.patch.object(launcher.sys, 'argv',
                                       ['launcher.py', 'node', 'build.js']):
                    with mock.patch.object(launcher.subprocess,
                                           'call',
                                           side_effect=_capture) as call:
                        return_code = launcher.main()
            self.assertEqual(return_code, 0)
            call.assert_called_once()
            self.assertEqual(seen['argv'], [str(node), 'build.js'])
            self.assertEqual(seen['env'][self.ENV_VAR], str(checkout))
            # The launcher's own environment is left untouched.
            self.assertNotIn(self.ENV_VAR, os.environ)

    def _capture_written_env_keys(self):
        """Patches `os.environ.copy` to return a dict recording `__setitem__`
        calls; returns the dict (populated once `main()` calls `copy()`)."""

        class _RecordingDict(dict):

            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.written_keys = []

            def __setitem__(self, key_, value):
                self.written_keys.append(key_)
                super().__setitem__(key_, value)

        recorded = {}

        def _fake_copy():
            recorded['env'] = _RecordingDict(os.environ)
            return recorded['env']

        return recorded, mock.patch.object(launcher.os.environ,
                                           'copy',
                                           side_effect=_fake_copy)

    def test_does_not_rewrite_when_already_set(self):
        # `env` (os.environ.copy()) already carries forward whatever this
        # process itself inherited, so when the var is already set there is
        # nothing new to write -- avoid the redundant assignment. Proven via a
        # dict subclass that records writes.
        key = launcher.host_platform_key()
        if key is None:
            self.skipTest('unsupported host platform')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            checkout = self._make_checkout(root)
            node = root / launcher.SHIM_TARGETS[f'node-{key}'].path
            node.parent.mkdir(parents=True, exist_ok=True)
            node.write_text('', encoding='utf-8', newline='')
            os.environ[self.ENV_VAR] = str(checkout)

            recorded, patch_copy = self._capture_written_env_keys()
            with patch_copy:
                with mock.patch.object(launcher.sys, 'argv',
                                       ['launcher.py', 'node', 'build.js']):
                    with mock.patch.object(launcher.subprocess,
                                           'call',
                                           return_value=0):
                        return_code = launcher.main()
            self.assertEqual(return_code, 0)
            self.assertNotIn(self.ENV_VAR, recorded['env'].written_keys)

    def test_does_not_rewrite_a_non_matching_but_already_set_env_var(self):
        # Presence is all that matters, not the value -- an already-set var
        # is left alone even when it names a different checkout than the one
        # cwd resolves to here (an ancestor's record wins over this process's
        # own cwd-based resolution for what gets forwarded).
        key = launcher.host_platform_key()
        if key is None:
            self.skipTest('unsupported host platform')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            checkout = self._make_checkout(root)
            node = root / launcher.SHIM_TARGETS[f'node-{key}'].path
            node.parent.mkdir(parents=True, exist_ok=True)
            node.write_text('', encoding='utf-8', newline='')
            os.environ[self.ENV_VAR] = '/some/other/checkout'

            recorded, patch_copy = self._capture_written_env_keys()
            with patch_copy:
                with mock.patch.object(launcher.Path,
                                       'cwd',
                                       return_value=checkout):
                    with mock.patch.object(
                            launcher.sys, 'argv',
                        ['launcher.py', 'node', 'build.js']):
                        with mock.patch.object(launcher.subprocess,
                                               'call',
                                               return_value=0):
                            return_code = launcher.main()
            self.assertEqual(return_code, 0)
            self.assertNotIn(self.ENV_VAR, recorded['env'].written_keys)
            self.assertEqual(recorded['env'][self.ENV_VAR],
                             '/some/other/checkout')

    def test_no_env_var_set_when_no_checkout_resolved(self):
        # A system-binary fallback outside any checkout has nothing to record.
        os.environ.pop(self.ENV_VAR, None)
        with tempfile.TemporaryDirectory() as tmp:
            outside = Path(tmp) / 'elsewhere'
            outside.mkdir()
            with mock.patch.object(launcher.Path, 'cwd', return_value=outside):
                with mock.patch.object(launcher.shutil,
                                       'which',
                                       return_value='/usr/bin/node'):
                    seen = {}

                    def _capture(_argv, env=None):
                        seen['env'] = env
                        return 0

                    with mock.patch.object(
                            launcher.sys, 'argv',
                        ['launcher.py', '--allow-fallback', 'node']):
                        with mock.patch.object(launcher.subprocess,
                                               'call',
                                               side_effect=_capture):
                            return_code = launcher.main()
        self.assertEqual(return_code, 0)
        self.assertNotIn(self.ENV_VAR, seen['env'])

    def test_nested_invocation_outside_checkout_resolves_via_env(self):
        # The exact regression this guards (brave/brave-browser#56529): a
        # `pnpm install` lifecycle script for a git-hosted dependency (e.g.
        # figma-api-exporter) runs from pnpm's own store tmp extraction dir --
        # nowhere near any checkout by cwd -- but with the checkout env var
        # inherited from the pnpm invocation that spawned it. Before this fix,
        # that lifecycle script's `pnpm install` failed with "no
        # checkout-local binary found and no pnpm on $PATH."
        key = launcher.host_platform_key()
        if key is None:
            self.skipTest('unsupported host platform')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / 'workspace'
            pnpm_target = root / launcher.SHIM_TARGETS['pnpm'].path
            pnpm_target.parent.mkdir(parents=True, exist_ok=True)
            pnpm_target.write_text('', encoding='utf-8', newline='')
            checkout = root / 'src' / 'brave'
            sentinel = checkout / 'tools' / 'cr' / 'bootstrap' / 'launcher.py'
            sentinel.parent.mkdir(parents=True, exist_ok=True)
            sentinel.write_text('', encoding='utf-8', newline='')
            os.environ[self.ENV_VAR] = str(checkout)

            # cwd is pnpm's own store tmp dir, e.g.
            # ~/.local/share/pnpm/store/v11/tmp/_tmp_DAPDJk -- a sibling tree
            # with no `src/brave` anywhere above it.
            pnpm_store_tmp = (Path(tmp) / '.local' / 'share' / 'pnpm' /
                              'store' / 'v11' / 'tmp' / '_tmp_DAPDJk')
            pnpm_store_tmp.mkdir(parents=True)

            with mock.patch.object(launcher.Path,
                                   'cwd',
                                   return_value=pnpm_store_tmp):
                with mock.patch.object(launcher.shutil,
                                       'which',
                                       return_value='/usr/bin/node'):
                    with mock.patch.object(
                            launcher.sys, 'argv',
                        ['launcher.py', '--allow-fallback', 'pnpm']):
                        with mock.patch.object(launcher.subprocess,
                                               'call',
                                               return_value=0) as call:
                            return_code = launcher.main()
            self.assertEqual(return_code, 0)
            call.assert_called_once()
            args, _ = call.call_args
            self.assertEqual(args[0], ['/usr/bin/node', str(pnpm_target)])

    def test_reproduces_the_original_failure_without_the_env_var(self):
        # Contrast case: strip the env var (as if this were the pre-fix
        # launcher, or the var simply never made it down) and the exact
        # reported failure comes back -- proving the fix, not a tautology.
        os.environ.pop(self.ENV_VAR, None)
        with tempfile.TemporaryDirectory() as tmp:
            pnpm_store_tmp = (Path(tmp) / '.local' / 'share' / 'pnpm' /
                              'store' / 'v11' / 'tmp' / '_tmp_DAPDJk')
            pnpm_store_tmp.mkdir(parents=True)

            with mock.patch.object(launcher.Path,
                                   'cwd',
                                   return_value=pnpm_store_tmp):
                with mock.patch.object(launcher.shutil,
                                       'which',
                                       return_value=None):
                    with mock.patch.object(
                            launcher.sys, 'argv',
                        ['launcher.py', '--allow-fallback', 'pnpm']):
                        with contextlib.redirect_stderr(
                                io.StringIO()) as stderr:
                            return_code = launcher.main()
        self.assertEqual(return_code, 1)
        self.assertIn(
            'pnpm: no checkout-local binary found and no pnpm on $PATH.',
            stderr.getvalue())


class MultiRepoSelfUpdaterTest(unittest.TestCase):
    """`SelfUpdater` resolves the `extra_deps` table, the sidecar tree, and the
    installer in the *target* checkout it is given -- never in the checkout
    `launcher.py` itself lives in. One installed shim serves many checkouts, so
    it must always talk to the tree governing the current directory.
    """

    NODE = 'src/brave/third_party/node/node-linux-x64'

    # `tools/cr` of the checkout launcher.py lives in (its "own" checkout).
    _OWN_CR = Path(launcher.__file__).resolve().parents[1]

    def _target_cr(self, root: Path) -> Path:
        cr = root / 'src' / 'brave' / 'tools' / 'cr'
        cr.mkdir(parents=True, exist_ok=True)
        return cr

    def test_load_extra_deps_reads_the_target_checkout(self):
        # The target pins a made-up table; the loaded module must be that one,
        # not the launcher's own (which pins the real node entries).
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (self._target_cr(root) / 'extra_deps.py').write_text(
                'EXTRA_DEPS = {"src/only/in/target": 1}\n',
                encoding='utf-8',
                newline='')
            checkout = root / 'src' / 'brave'
            module = launcher.SelfUpdater(checkout, 'x')._load_extra_deps()
            self.assertEqual(module.EXTRA_DEPS, {'src/only/in/target': 1})
            self.assertNotIn(self.NODE, module.EXTRA_DEPS)

    def test_deploy_runs_the_target_checkout_installer(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (self._target_cr(root) / 'tarball_installer.py').write_text(
                '', encoding='utf-8', newline='')
            checkout = root / 'src' / 'brave'
            with mock.patch.object(launcher.subprocess, 'call',
                                   return_value=0) as call:
                launcher.SelfUpdater(checkout, 'src/dep').deploy()
            argv = call.call_args.args[0]
            self.assertEqual(
                Path(argv[1]),
                checkout / 'tools' / 'cr' / 'tarball_installer.py')
            self.assertEqual(argv[2], 'src/dep')
            # Emphatically not the launcher's own installer.
            self.assertNotEqual(Path(argv[1]),
                                self._OWN_CR / 'tarball_installer.py')

    def test_needs_update_reads_the_target_sidecar_tree(self):
        # Copy the real `extra_deps` into a target checkout with node NOT
        # deployed: needs_update() is True even though the launcher's own
        # checkout has node deployed -- so the sidecar lookup is rooted at the
        # target's workspace root, not the launcher's.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            checkout = root / 'src' / 'brave'
            # `extra_deps.py` loads the sibling `EXTRA_DEPS` data file, so the
            # target checkout needs both.
            shutil.copy(self._OWN_CR / 'extra_deps.py',
                        self._target_cr(root) / 'extra_deps.py')
            shutil.copy(self._OWN_CR.parent.parent / 'EXTRA_DEPS',
                        checkout / 'EXTRA_DEPS')
            updater = launcher.SelfUpdater(checkout, self.NODE)
            self.assertTrue(updater.needs_update())

            # Seed the target's sidecar for the pinned object -> now deployed.
            module = updater._load_extra_deps()
            obj = module.EXTRA_DEPS[self.NODE]['objects'][0]
            dest = root / self.NODE
            dest.mkdir(parents=True, exist_ok=True)
            module.sidecar_path(dest, obj['object_name'],
                                '_hash').write_text(obj['sha256sum'] + '\n')
            self.assertFalse(updater.needs_update())


class ArgumentForwardingTest(unittest.TestCase):
    """Exercises `launcher.build_parser` argv splitting.

    The launcher must never swallow a tool's own arguments: it parses only its
    leading options and the TOOL token, forwarding everything from TOOL onward
    verbatim (regression coverage for `brockit gen-rust-toolchain --help`).
    """

    def _split(self, argv: list[str]) -> tuple[bool, str, list[str]]:
        parsed = launcher.build_parser().parse_args(argv)
        return parsed.allow_fallback, parsed.tool, parsed.tool_args

    def test_forwards_help_to_tool(self):
        # The bug: `--help` after TOOL was intercepted by the launcher instead
        # of reaching the tool.
        self.assertEqual(self._split(['gen-rust-toolchain', '--help']),
                         (False, 'gen-rust-toolchain', ['--help']))
        self.assertEqual(self._split(['brockit', '-h']),
                         (False, 'brockit', ['-h']))

    def test_forwards_tool_flags_verbatim(self):
        self.assertEqual(self._split(['brockit', 'lift', '--to=1.2.3.4']),
                         (False, 'brockit', ['lift', '--to=1.2.3.4']))

    def test_leading_allow_fallback_is_consumed(self):
        # Before TOOL, `--allow-fallback` is the launcher's own flag.
        self.assertEqual(self._split(['--allow-fallback', 'node', 'build.js']),
                         (True, 'node', ['build.js']))

    def test_allow_fallback_after_tool_is_forwarded(self):
        # After TOOL, the identical flag is the tool's — forwarded, not
        # acted on.
        self.assertEqual(self._split(['brockit', '--allow-fallback', 'foo']),
                         (False, 'brockit', ['--allow-fallback', 'foo']))

    def test_bare_tool_has_no_args(self):
        self.assertEqual(self._split(['brockit']), (False, 'brockit', []))

    def test_own_help_renders_without_crashing(self):
        help_text = launcher.build_parser().format_help()
        self.assertIn('$PATH', help_text)
        self.assertNotIn('%', help_text)


class BatShimLauncherResolutionTest(unittest.TestCase):
    """Guards the Windows shims against the `%~dp0`-is-cwd regression.

    When a shim is invoked as a bare `node`/`npm`/`pnpm` by another process
    (e.g. npm running a package's lifecycle scripts), cmd can expand `%~dp0` to
    the current directory, so a bare `python3 "%~dp0launcher.py"` pointed at the
    cwd and failed with `can't open file '...\\launcher.py'`. Each Windows shim
    must fall back to resolving its own name on `%PATH%` (`%~dp$PATH:0`) so
    `launcher.py` is found beside the shim, not in the cwd. Covers both `.bat`
    and `.cmd` variants (npm/pnpm ship `.cmd`, which callers spawn by name).
    """

    _WIN_SHIMS = ('node.bat', 'npm.bat', 'npm.cmd', 'pnpm.cmd', 'brockit.bat',
                  'plaster.bat', 'git-cr.bat')

    def _read(self, name: str) -> str:
        path = Path(launcher.__file__).resolve().parent / name
        return path.read_text(encoding='utf-8')

    def test_every_win_shim_exists(self):
        for name in self._WIN_SHIMS:
            self.assertTrue(
                (Path(launcher.__file__).resolve().parent / name).is_file(),
                name)

    def test_win_shims_resolve_launcher_via_path_fallback(self):
        for name in self._WIN_SHIMS:
            text = self._read(name)
            self.assertIn('launcher.py', text, name)
            # Try `%~dp0` first, but fall back to a %PATH% search for our own
            # name when launcher.py is not beside `%~dp0` (i.e. it was the cwd).
            self.assertIn('if not exist "%_dir%launcher.py"', text, name)
            self.assertIn('%~dp$PATH:0', text, name)

    def test_win_shims_do_not_run_launcher_straight_from_dp0(self):
        # The fragile form this bug was about: python3 invoking `%~dp0launcher`
        # directly, with no %PATH% fallback.
        for name in self._WIN_SHIMS:
            text = self._read(name)
            self.assertNotIn('python3 "%~dp0launcher.py"', text, name)


if __name__ == '__main__':
    unittest.main()
