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
        # Whether found on $PATH or via the chromium-bundled fallback, the
        # resolved interpreter is always a vpython3 (vpython3.bat on Windows).
        resolved = launcher._resolve_vpython3(Path('/home/dev/src/brave'))
        self.assertIn(resolved.name, ('vpython3', 'vpython3.bat'))


class FindBraveCheckoutTest(unittest.TestCase):
    """Exercises `launcher.find_brave_checkout`.

    The node/npm shims resolve the checkout from the conventional
    `<workspace>/src/brave` layout (not git), so a single rule covers being
    inside the checkout, inside a nested DEPS repo, and above `src/brave`.
    """

    def _make_workspace(self, root: Path) -> Path:
        brave = root / 'src' / 'brave'
        (brave / 'tools' / 'cr' / 'bootstrap').mkdir(parents=True)
        (brave / 'tools' / 'cr' / 'bootstrap' / 'launcher.py').write_text(
            '#', encoding='utf-8', newline='')
        return brave

    def test_resolves_from_inside_the_checkout(self):
        with tempfile.TemporaryDirectory() as tmp:
            brave = self._make_workspace(Path(tmp))
            inside = brave / 'components' / 'foo'
            inside.mkdir(parents=True)
            self.assertEqual(
                launcher.find_brave_checkout(inside).resolve(),
                brave.resolve())

    def test_resolves_from_nested_deps_repo(self):
        with tempfile.TemporaryDirectory() as tmp:
            brave = self._make_workspace(Path(tmp))
            nested = brave / 'vendor' / 'web-discovery-project' / 'src'
            nested.mkdir(parents=True)
            self.assertEqual(
                launcher.find_brave_checkout(nested).resolve(),
                brave.resolve())

    def test_resolves_from_above_src_brave(self):
        # The `npm run init` case: cwd is the workspace root, src/brave a child.
        with tempfile.TemporaryDirectory() as tmp:
            brave = self._make_workspace(Path(tmp))
            workspace = Path(tmp)
            self.assertEqual(
                launcher.find_brave_checkout(workspace).resolve(),
                brave.resolve())

    def test_resolves_from_sibling_chromium_dir(self):
        # A chromium dir like src/chrome resolves to the sibling src/brave.
        with tempfile.TemporaryDirectory() as tmp:
            brave = self._make_workspace(Path(tmp))
            chrome = Path(tmp) / 'src' / 'chrome'
            chrome.mkdir(parents=True)
            self.assertEqual(
                launcher.find_brave_checkout(chrome).resolve(),
                brave.resolve())

    def test_none_outside_any_workspace(self):
        with tempfile.TemporaryDirectory() as tmp:
            outside = Path(tmp) / 'elsewhere'
            outside.mkdir()
            self.assertIsNone(launcher.find_brave_checkout(outside))

    def test_requires_sentinel(self):
        with tempfile.TemporaryDirectory() as tmp:
            # A bare src/brave without the bootstrap sentinel is not a checkout.
            (Path(tmp) / 'src' / 'brave').mkdir(parents=True)
            self.assertIsNone(launcher.find_brave_checkout(Path(tmp)))


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
            self.assertEqual(
                launcher.resolve_invocation(f'node-{key}',
                                            self._checkout(root), False),
                [str(node)])

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
        """Create an empty tarball_installer.py so the bootstrap is attempted."""
        installer = checkout / 'tools' / 'cr' / 'tarball_installer.py'
        installer.parent.mkdir(parents=True, exist_ok=True)
        installer.write_text('', encoding='utf-8', newline='')

    def test_bootstraps_missing_node_then_resolves(self):
        # A missing node target with a known self_update_extra_dep_entry triggers a download
        # (install_extra_deps.py), after which the vendored node resolves.
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
        # Only node/npm carry an self_update_extra_dep_entry; a missing vpython tool is never
        # bootstrapped (it ships in the repo, it is not downloaded).
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
        # After TOOL, the identical flag is the tool's — forwarded, not acted on.
        self.assertEqual(self._split(['brockit', '--allow-fallback', 'foo']),
                         (False, 'brockit', ['--allow-fallback', 'foo']))

    def test_bare_tool_has_no_args(self):
        self.assertEqual(self._split(['brockit']), (False, 'brockit', []))

    def test_own_help_renders_without_crashing(self):
        # argparse expands help strings through `% params`; a bare `%` (the
        # `%PATH` typo) raised ValueError. The `%%` escape must survive it and
        # render a literal `%PATH`.
        help_text = launcher.build_parser().format_help()
        self.assertIn('%PATH', help_text)


if __name__ == '__main__':
    unittest.main()
