#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the recipe engine's DEPS resolution and property binding."""

# White-box tests: they exercise engine internals (`_Engine`,
# `_instantiate_module`, `_run_steps`, ...), so protected-access is
# intentional.
# pylint: disable=protected-access

import contextlib
import os
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
import engine
from recipe_api import RecipeApi
from recipe_test_api import RecipeTestApi


@contextlib.contextmanager
def _real_git_cache():
    """Point `GIT_CACHE_PATH` at a real directory for the duration of a test.

    Instantiating `chromium_checkout` outside a simulated run also
    instantiates `git_cache`, which validates `GIT_CACHE_PATH` against the
    real environment as soon as it's constructed. Tests that only exercise the
    engine's generic DEPS/test_api wiring don't care about the cache itself,
    so this keeps them from depending on whatever (if anything) the host
    happens to have configured.
    """
    with tempfile.TemporaryDirectory() as tmp:
        with mock.patch.dict(os.environ, {'GIT_CACHE_PATH': tmp}):
            yield


class DepsResolutionTest(unittest.TestCase):
    """The engine wires a module's DEPS onto its injection site."""

    def test_wires_deps_onto_injection_site(self):
        with _real_git_cache():
            cc = engine._Engine()._instantiate_module('chromium_checkout', [])
        self.assertTrue(hasattr(cc.m, 'depot_tools'))
        self.assertTrue(hasattr(cc.m, 'step'))
        # A module can reach itself via self.m.<own_name>.
        self.assertIs(cc.m.chromium_checkout, cc)

    def test_instances_are_cached_and_shared(self):
        eng = engine._Engine()
        with _real_git_cache():
            cc = eng._instantiate_module('chromium_checkout', [])
            dt = eng._instantiate_module('depot_tools', [])
        # chromium_checkout and depot_tools share one `step` instance.
        self.assertIs(cc.m.step, dt.m.step)


class TestApiInjectionTest(unittest.TestCase):
    """Every module is given its own TEST_API, in production runs too.

    A module reaches it as `self.test_api` to describe what its steps should
    return under simulation (`step_test_data=`), so it has to be there whether
    or not the run is simulated.
    """

    def test_module_gets_its_own_test_api(self):
        with _real_git_cache():
            cc = engine._Engine()._instantiate_module('chromium_checkout', [])
        self.assertIsInstance(cc.test_api, RecipeTestApi)
        # It is the chromium_checkout test api, not the base class.
        self.assertTrue(hasattr(cc.test_api, 'with_git_cache'))

    def test_module_without_a_test_api_gets_the_base(self):
        context = engine._Engine()._instantiate_module('context', [])
        self.assertIs(type(context.test_api), RecipeTestApi)

    def test_deps_are_wired_onto_m(self):
        cache = {}
        cc = engine.instantiate_test_module('chromium_checkout', [], cache)
        # chromium_checkout's DEPS test apis are reachable via .m.
        self.assertTrue(hasattr(cc.m, 'env'))
        self.assertTrue(hasattr(cc.m, 'path'))
        # Cached instances are shared.
        self.assertIs(engine.instantiate_test_module('path', [], cache),
                      cc.m.path)

    def test_root_api_exposes_dep_helpers(self):
        root = engine.build_root_test_api(['platform', 'step'])
        # api.platform.name(...) and api.step.data(...) resolve to the module
        # test apis.
        self.assertEqual(
            root.platform.name('mac').mod_data['platform']['name'], 'mac')
        self.assertIn('s', root.step.data('s', retcode=2).step_data)


class FindTestApiClassTest(unittest.TestCase):
    """_find_test_api_class allows at most one RecipeTestApi subclass."""

    def test_defaults_to_base_when_absent(self):
        module = types.ModuleType('fake')
        self.assertIs(engine._find_test_api_class(module, 'fake'),
                      RecipeTestApi)

    def test_finds_the_single_subclass(self):
        module = types.ModuleType('fake')

        class MyTestApi(RecipeTestApi):
            pass

        MyTestApi.__module__ = 'fake'
        module.MyTestApi = MyTestApi
        self.assertIs(engine._find_test_api_class(module, 'fake'), MyTestApi)

    def test_two_classes_raises(self):
        module = types.ModuleType('fake')

        class TestApiA(RecipeTestApi):
            pass

        class TestApiB(RecipeTestApi):
            pass

        TestApiA.__module__ = TestApiB.__module__ = 'fake'
        module.TestApiA = TestApiA
        module.TestApiB = TestApiB
        with self.assertRaises(RuntimeError):
            engine._find_test_api_class(module, 'fake')


class FindApiClassTest(unittest.TestCase):
    """_find_api_class requires exactly one RecipeApi subclass."""

    def test_zero_classes_raises(self):
        module = types.ModuleType('fake')
        with self.assertRaises(RuntimeError):
            engine._find_api_class(module, 'fake')

    def test_exactly_one_class(self):
        module = types.ModuleType('fake')

        class OnlyApi(RecipeApi):
            pass

        OnlyApi.__module__ = 'fake'
        module.OnlyApi = OnlyApi
        self.assertIs(engine._find_api_class(module, 'fake'), OnlyApi)

    def test_two_classes_raises(self):
        module = types.ModuleType('fake')

        class ApiA(RecipeApi):
            pass

        class ApiB(RecipeApi):
            pass

        ApiA.__module__ = ApiB.__module__ = 'fake'
        module.ApiA = ApiA
        module.ApiB = ApiB
        with self.assertRaises(RuntimeError):
            engine._find_api_class(module, 'fake')


class RunStepsBindingTest(unittest.TestCase):
    """_run_steps decodes input into typed PROPERTIES/ENV_PROPERTIES messages.

    Uses a recipe's real compiled proto messages, so it also exercises that the
    `PB` package builds and imports.
    """

    @classmethod
    def setUpClass(cls):
        engine._ensure_protos()
        # pylint: disable=import-outside-toplevel,import-error
        from PB.recipes.brave.toolchains.rust.package_rust import (
            InputProperties)
        # `git_cache` is the only module declaring ENV_PROPERTIES.
        from PB.recipe_modules.brave.git_cache.properties import EnvProperties
        cls.InputProperties = InputProperties
        cls.EnvProperties = EnvProperties

    def _bind(self, properties, environ, props_def, env_def):
        """Return the positional args _run_steps would pass to RunSteps."""
        captured = []
        engine._run_steps(lambda *args: captured.extend(args), 'API',
                          properties, environ, props_def, env_def)
        return captured

    def test_no_defs_passes_only_api(self):
        self.assertEqual(self._bind({}, {}, None, None), ['API'])

    def test_properties_decoded_and_reserved_keys_stripped(self):
        args = self._bind(
            {
                'chromium_ref': 'main',
                'brave_subrevision': 3,
                '$hidden': 'ignored',
            }, {}, self.InputProperties, None)
        self.assertEqual(args[0], 'API')
        self.assertEqual(args[1].chromium_ref, 'main')
        self.assertEqual(args[1].brave_subrevision, 3)

    def test_env_properties_uppercased_and_unknown_ignored(self):
        args = self._bind({}, {
            'git_cache_path': '/c',
            'PATH': '/bin'
        }, None, self.EnvProperties)
        self.assertEqual(args[1].GIT_CACHE_PATH, '/c')

    def test_both_defs_pass_properties_then_env(self):
        args = self._bind({'chromium_ref': 'x'}, {'GIT_CACHE_PATH': '/c'},
                          self.InputProperties, self.EnvProperties)
        self.assertEqual(len(args), 3)
        self.assertEqual(args[1].chromium_ref, 'x')
        self.assertEqual(args[2].GIT_CACHE_PATH, '/c')

    def test_non_message_properties_def_raises(self):
        with self.assertRaises(TypeError):
            self._bind({}, {}, dict, None)


class RunRecipeTest(unittest.TestCase):
    """run_recipe maps the slash path to a module and requires RunSteps."""

    def test_slash_path_mapped_and_missing_run_steps_raises(self):
        fake_recipe = types.SimpleNamespace(DEPS=[])
        import_module = mock.Mock(return_value=fake_recipe)
        with mock.patch.object(engine.importlib, 'import_module',
                               import_module):
            with self.assertRaises(RuntimeError):
                engine.run_recipe('group/sub/my_recipe', {})
        import_module.assert_called_once_with('recipes.group.sub.my_recipe')


class WorkspaceTest(unittest.TestCase):
    """The engine seeds its workspace; the `path` module derives job paths."""

    def setUp(self):
        # _Engine chdirs into an explicit workspace; restore cwd afterwards so
        # it doesn't leak into other tests (some read Path.cwd()).
        self._prev_cwd = Path.cwd()
        self.addCleanup(os.chdir, self._prev_cwd)

    def test_workspace_seeded_into_path_module(self):
        with tempfile.TemporaryDirectory() as tmp:
            eng = engine._Engine(workspace=tmp)
            path = eng._instantiate_module('path', [])
            workspace = Path(tmp).resolve()
            self.assertEqual(path.workspace, workspace)
            self.assertEqual(path.chromium_src, workspace / 'b/src')
            self.assertEqual(path.brave_core, workspace / 'b/src/brave')
            self.assertEqual(path.out, workspace / 'out')
            self.assertEqual(Path.cwd(), workspace)
            os.chdir(self._prev_cwd)

    def test_workspace_defaults_to_cwd(self):
        path = engine._Engine()._instantiate_module('path', [])
        self.assertEqual(path.workspace, Path.cwd())

    def test_brave_core_ref_seeded_with_override(self):
        module = engine._Engine(
            brave_core_ref='feature/x')._instantiate_module(
                'brave_core_checkout', [])
        self.assertEqual(getattr(module, '_brave_core_ref'), 'feature/x')

    def test_brave_core_ref_defaults_to_master(self):
        module = engine._Engine()._instantiate_module('brave_core_checkout',
                                                      [])
        self.assertEqual(getattr(module, '_brave_core_ref'), 'master')


class ModulePropertiesTest(unittest.TestCase):
    """The engine binds a module's PROPERTIES/ENV_PROPERTIES into its api.

    Uses the `hello` module (which declares a PROPERTIES message) for the
    end-to-end paths, and compiled messages from package_rust and the
    `git_cache` module for the arg-order / env path via
    `_module_property_args` directly.
    """

    @classmethod
    def setUpClass(cls):
        engine._ensure_protos()
        # pylint: disable=import-outside-toplevel,import-error
        from PB.recipes.brave.toolchains.rust.package_rust import (
            InputProperties)
        # `git_cache` is the only module declaring ENV_PROPERTIES.
        from PB.recipe_modules.brave.git_cache.properties import EnvProperties
        cls.InputProperties = InputProperties
        cls.EnvProperties = EnvProperties

    def test_properties_bound_from_namespaced_block(self):
        eng = engine._Engine()
        eng._properties = {'$hello': {'target': 'Ada'}}
        hello = eng._instantiate_module('hello', [])
        self.assertEqual(hello._target, 'Ada')

    def test_absent_block_yields_proto_defaults(self):
        # No `$hello` block: the message defaults (empty target -> None).
        hello = engine._Engine()._instantiate_module('hello', [])
        self.assertIsNone(hello._target)

    def test_top_level_props_do_not_leak_into_module(self):
        # A non-namespaced top-level property is not a module property.
        eng = engine._Engine()
        eng._properties = {'target': 'Zed'}
        hello = eng._instantiate_module('hello', [])
        self.assertIsNone(hello._target)

    def test_property_feeds_config_end_to_end(self):
        eng = engine._Engine()
        eng._properties = {'$hello': {'target': 'Ada'}}
        hello = eng._instantiate_module('hello', [])
        hello.set_config('default_tool')
        self.assertEqual(hello.c.TARGET, 'Ada')

    def test_arg_order_properties_then_env(self):
        eng = engine._Engine()
        eng._properties = {'$fake': {'chromium_ref': 'x'}}
        eng._environ = {'git_cache_path': '/c'}  # lower: must be upper-cased
        pkg = types.SimpleNamespace(PROPERTIES=self.InputProperties,
                                    ENV_PROPERTIES=self.EnvProperties)
        args = eng._module_property_args('fake', pkg)
        self.assertEqual(len(args), 2)
        self.assertEqual(args[0].chromium_ref, 'x')
        self.assertEqual(args[1].GIT_CACHE_PATH, '/c')

    def test_no_defs_means_no_args(self):
        pkg = types.SimpleNamespace(DEPS=[])
        self.assertEqual(engine._Engine()._module_property_args('fake', pkg),
                         [])

    def test_non_message_properties_raises(self):
        pkg = types.SimpleNamespace(PROPERTIES=dict, DEPS=[])
        with self.assertRaises(TypeError):
            engine._Engine()._module_property_args('fake', pkg)


if __name__ == '__main__':
    unittest.main()
