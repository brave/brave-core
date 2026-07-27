#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the config DSL (config.py) and its RecipeApi/engine wiring."""

# White-box tests: they exercise engine internals (`_Engine`,
# `_load_config_ctx`) and RecipeApi config plumbing, so protected-access is
# intentional.
# pylint: disable=protected-access

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
import config
import engine
from config import (BadConf, ConfigContext, ConfigGroup, ConfigList, Dict,
                    Enum, List, Set, Single, Static, config_item_context)
from recipe_api import RecipeApi


def _schema(TARGET='Bob'):
    return ConfigGroup(
        verb=Single(str),
        tool=Single(str, required=True),
        TARGET=Static(str(TARGET)),
    )


class SingleTest(unittest.TestCase):

    def test_typecheck_and_complete(self):
        s = Single(int)
        self.assertFalse(s.complete())  # required, still empty_val
        self.assertTrue(s._is_default())
        s.set_val(5)
        self.assertEqual(s.get_val(), 5)
        self.assertTrue(s.complete())
        self.assertFalse(s._is_default())
        with self.assertRaises(TypeError):
            s.set_val('nope')

    def test_not_required_is_complete_when_empty(self):
        self.assertTrue(Single(int, required=False).complete())

    def test_reset_and_set_from_single(self):
        s = Single(int)
        s.set_val(3)
        other = Single(int)
        other.set_val(s)  # accepts another Single
        self.assertEqual(other.get_val(), 3)
        s.reset()
        self.assertIsNone(s.get_val())


class StaticTest(unittest.TestCase):

    def test_immutable_and_always_complete(self):
        st = Static('x')
        self.assertEqual(st.get_val(), 'x')
        self.assertTrue(st.complete())
        self.assertTrue(st._is_default())
        with self.assertRaises(TypeError):
            st.set_val('y')

    def test_unhashable_value_rejected(self):
        with self.assertRaises(TypeError):
            Static(['a', 'list', 'is', 'mutable'])


class EnumTest(unittest.TestCase):
    # astroid's enum brain misfires on our config.Enum (a plain ConfigBase
    # subclass, not enum.Enum): `Enum('a', 'b')` matches the stdlib functional
    # Enum API, so it infers instances as enum members lacking our methods. The
    # methods exist and the tests pass at runtime.
    # pylint: disable=no-member

    def test_membership_and_type(self):
        e = Enum('a', 'b')
        self.assertFalse(e.complete())
        e.set_val('a')
        self.assertEqual(e.get_val(), 'a')
        self.assertTrue(e.complete())
        with self.assertRaises(ValueError):
            e.set_val('c')
        with self.assertRaises(TypeError):
            e.set_val(1)

    def test_empty_values_rejected(self):
        with self.assertRaises(ValueError):
            Enum()

    def test_reset_and_not_required(self):
        e = Enum('a', required=False)
        self.assertTrue(e.complete())
        e.set_val('a')
        e.reset()
        self.assertIsNone(e.get_val())

    def test_set_from_enum(self):
        src = Enum('a', 'b')
        src.set_val('b')
        dst = Enum('a', 'b')
        dst.set_val(src)
        self.assertEqual(dst.get_val(), 'b')


class ContainerTest(unittest.TestCase):

    def test_dict_value_type_and_jsonish_sorted(self):
        d = Dict(value_type=int)
        d['b'] = 2
        d['a'] = 1
        self.assertTrue(d.complete())
        self.assertFalse(d._is_default())
        self.assertEqual(d.as_jsonish(), {'a': 1, 'b': 2})
        self.assertIn("'a': 1", repr(d))
        self.assertIn("'a': 1", str(d))
        with self.assertRaises(TypeError):
            d['c'] = 'not int'
        del d['a']
        self.assertEqual(len(d), 1)
        d.reset()
        self.assertTrue(d._is_default())

    def test_dict_set_val_typechecks(self):
        d = Dict(value_type=int)
        d.set_val({'a': 1})
        self.assertEqual(d.as_jsonish(), {'a': 1})
        with self.assertRaises(TypeError):
            d.set_val({'a': 'x'})
        other = Dict()
        other.set_val(d)  # accepts another Dict
        self.assertEqual(dict(other), {'a': 1})

    def test_list_typecheck_and_radd(self):
        li = List(int)
        li.append(1)
        li.insert(0, 0)
        self.assertEqual([0] + li, [0, 0, 1])  # __radd__
        li[0] = 9
        self.assertEqual(li.as_jsonish(), [9, 1])
        del li[0]
        with self.assertRaises(TypeError):
            li.append('x')
        li.set_val([3, 4])
        self.assertEqual(li.as_jsonish(), [3, 4])
        with self.assertRaises(TypeError):
            li.set_val(['x'])
        li.reset()
        self.assertTrue(li._is_default())

    def test_set_add_update_discard(self):
        s = Set(int)
        s.add(1)
        s.update([1, 2, 3])  # dedups
        self.assertEqual(s.as_jsonish(), [1, 2, 3])
        self.assertIn(2, s)
        self.assertEqual(len(s), 3)
        s.discard(2)
        self.assertNotIn(2, s)
        with self.assertRaises(TypeError):
            s.add('x')
        s.set_val([7, 8])
        self.assertEqual(s.as_jsonish(), [7, 8])
        with self.assertRaises(TypeError):
            s.set_val(['x'])
        s.reset()
        self.assertTrue(s._is_default())


class ConfigListTest(unittest.TestCase):

    def _factory(self):
        return lambda: ConfigGroup(herp=Single(int), derp=Single(str))

    def test_append_index_add_complete(self):
        cl = ConfigList(self._factory())
        cl.append({'herp': 1})
        cl[0].derp = 'bob'
        self.assertEqual(cl[0].derp, 'bob')
        self.assertTrue(cl.complete())  # both required fields now set
        item = cl.add()
        item.herp = 2
        self.assertEqual(len(cl), 2)
        cl.insert(0, {'herp': 9})
        self.assertEqual(cl[0].herp, 9)
        del cl[0]
        self.assertEqual(len(cl), 2)
        self.assertEqual(cl.as_jsonish()[0]['herp'], 1)

    def test_set_val_and_reset(self):
        cl = ConfigList(self._factory())
        cl.set_val([{'herp': 1}])
        self.assertEqual(cl[0].herp, 1)
        other = ConfigList(self._factory())
        other.set_val(cl)  # accepts another ConfigList
        self.assertEqual(other[0].herp, 1)
        cl.reset()
        self.assertEqual(len(cl), 0)
        self.assertTrue(cl._is_default())

    def test_bad_item_schema(self):
        with self.assertRaises(TypeError):
            ConfigList('not a function')
        with self.assertRaises(TypeError):
            ConfigList(lambda: Single(int))  # not a ConfigGroup


class ConfigGroupTest(unittest.TestCase):

    def test_attr_proxy_and_closed_schema(self):
        c = _schema()
        self.assertEqual(c.TARGET, 'Bob')  # Static unwrapped
        c.verb = 'Hi %s'
        self.assertEqual(c.verb, 'Hi %s')
        with self.assertRaises(AttributeError):
            c.unknown = 1  # cannot add keys (name is not a schema member)
        with self.assertRaises(TypeError):
            c.TARGET = 'x'  # Static rejects assignment

    def test_set_val_extra_keys_and_from_group(self):
        c = _schema()
        c.set_val({'verb': 'V', 'tool': 'echo'})
        self.assertEqual(c.tool, 'echo')
        with self.assertRaises(TypeError):
            c.set_val({'nope': 1})
        # set_val wraps the child's error with the offending key name.
        with self.assertRaises(TypeError) as ctx:
            c.set_val({'tool': 123})
        self.assertIn("'tool'", str(ctx.exception))
        # A group can be seeded from another group (via its jsonish form). Uses
        # a Static-free group: Static members reject assignment, so a group with
        # one cannot be copied wholesale.
        src = ConfigGroup(tool=Single(str))
        src.tool = 'echo'
        dst = ConfigGroup(tool=Single(str))
        dst.set_val(src)
        self.assertEqual(dst.tool, 'echo')

    def test_as_jsonish_hidden_modes(self):
        c = ConfigGroup(
            shown=Single(str, hidden=False),
            auto=Single(str),  # AutoHide: hidden while default
            always=Single(str, hidden=True),
        )
        # Defaults: shown always present, auto hidden (default), always hidden.
        self.assertEqual(set(c.as_jsonish()), {'shown'})
        self.assertEqual(set(c.as_jsonish(include_hidden=True)),
                         {'shown', 'auto', 'always'})
        c.auto = 'now-set'
        self.assertEqual(set(c.as_jsonish()), {'shown', 'auto'})

    def test_delattr_resets(self):
        c = _schema()
        c.verb = 'V'
        del c.verb
        self.assertIsNone(c.verb)

    def test_complete(self):
        c = _schema()
        self.assertFalse(c.complete())  # verb and tool are required
        c.verb = 'Hi %s'
        c.tool = 'echo'
        self.assertTrue(c.complete())  # TARGET (Static) is always complete

    def test_reset(self):
        # A group containing a Static cannot be reset (Static is write-once), so
        # reset() is only meaningful on a Static-free group.
        c = ConfigGroup(verb=Single(str), tool=Single(str))
        c.verb = 'Hi %s'
        c.tool = 'echo'
        self.assertTrue(c.complete())
        c.reset()
        self.assertFalse(c.complete())

    def test_empty_type_map_rejected(self):
        with self.assertRaises(AssertionError):
            ConfigGroup()


class ConfigContextTest(unittest.TestCase):

    def test_config_item_context_returns_context(self):
        self.assertIsInstance(config_item_context(_schema), ConfigContext)

    def test_root_auto_applied_and_includes(self):
        ctx = config_item_context(_schema)

        @ctx(is_root=True)
        def BASE(c):  # pylint: disable=unused-variable
            c.verb = 'Hello %s'

        @ctx()
        def use_echo(c):  # pylint: disable=unused-variable
            c.tool = 'echo'

        @ctx(includes=('use_echo', ))
        def combo(c):  # pylint: disable=unused-variable
            c.verb = 'Yo %s'

        blob = ctx.CONFIG_ITEMS['combo'](None)
        self.assertEqual(blob.tool, 'echo')  # via include
        self.assertEqual(blob.verb, 'Yo %s')  # root ran, then combo overrode

    def test_double_apply_raises_unless_optional(self):
        ctx = config_item_context(_schema)

        @ctx()
        def item(c):  # pylint: disable=unused-variable
            c.tool = 'echo'

        blob = ctx.CONFIG_ITEMS['item'](None)
        with self.assertRaises(BadConf):
            ctx.CONFIG_ITEMS['item'](blob)
        # optional=True makes re-application a silent no-op.
        self.assertIs(ctx.CONFIG_ITEMS['item'](blob, optional=True), blob)

    def test_group_mutual_exclusion(self):
        ctx = config_item_context(_schema)

        @ctx(group='tool')
        def a(c):  # pylint: disable=unused-variable
            c.tool = 'a'

        @ctx(group='tool')
        def b(c):  # pylint: disable=unused-variable
            c.tool = 'b'

        blob = ctx.CONFIG_ITEMS['a'](None)
        with self.assertRaises(BadConf):
            ctx.CONFIG_ITEMS['b'](blob)

    def test_deps_satisfied_and_unfulfilled(self):
        ctx = config_item_context(_schema)

        @ctx(group='g')
        def provider(c):  # pylint: disable=unused-variable
            c.tool = 'echo'

        @ctx(deps=('g', ))
        def consumer(c):  # pylint: disable=unused-variable
            c.verb = 'Hi %s'

        with self.assertRaises(BadConf):
            ctx.CONFIG_ITEMS['consumer'](None)  # 'g' unfulfilled
        blob = ctx.CONFIG_ITEMS['provider'](None)
        ctx.CONFIG_ITEMS['consumer'](blob)  # now satisfied
        self.assertEqual(blob.verb, 'Hi %s')

    def test_final_false_allows_reapplication(self):
        ctx = config_item_context(_schema)

        @ctx()
        def item(c):  # pylint: disable=unused-variable
            c.tool = 'echo'

        blob = ctx.CONFIG_ITEMS['item'](None, final=False)
        # Not recorded, so it can be applied again without BadConf.
        ctx.CONFIG_ITEMS['item'](blob)

    def test_include_propagates_badconf(self):
        ctx = config_item_context(_schema)

        @ctx()
        def boom(c):  # pylint: disable=unused-variable
            raise BadConf('kaboom')

        @ctx(includes=('boom', ))
        def outer(c):  # pylint: disable=unused-variable,unused-argument
            pass

        with self.assertRaises(BadConf) as exc:
            ctx.CONFIG_ITEMS['outer'](None)
        self.assertIn('includes "boom"', str(exc.exception))

    def test_body_returning_value_asserts(self):
        ctx = config_item_context(_schema)

        @ctx()
        def bad(c):  # pylint: disable=unused-variable,unused-argument
            return 'nope'

        with self.assertRaises(AssertionError):
            ctx.CONFIG_ITEMS['bad'](None)

    def test_duplicate_name_and_multiple_roots(self):
        ctx = config_item_context(_schema)

        @ctx()
        def dup(c):  # pylint: disable=unused-variable,unused-argument
            pass

        with self.assertRaises(AssertionError):

            @ctx()
            def dup(c):  # noqa: F811  pylint: disable=function-redefined,unused-variable,unused-argument
                pass

        ctx2 = config_item_context(_schema)

        @ctx2(is_root=True)
        def root1(c):  # pylint: disable=unused-variable,unused-argument
            pass

        with self.assertRaises(AssertionError):

            @ctx2(is_root=True)
            def root2(c):  # pylint: disable=unused-variable,unused-argument
                pass


class RecipeApiConfigTest(unittest.TestCase):
    """The RecipeApi config plumbing, exercised on the `hello` module."""

    def _hello(self):
        return engine._Engine()._instantiate_module('hello', [])

    def test_set_config_and_greet_blob(self):
        hello = self._hello()
        hello.set_config('default_tool')
        self.assertEqual(hello.c.tool, 'echo')
        self.assertEqual(hello.c.TARGET, 'Bob')

    def test_config_vars_override_defaults(self):
        hello = self._hello()
        blob = hello.make_config('super_tool', TARGET='Charlie')
        self.assertEqual(blob.tool, 'unicorn.py')
        self.assertEqual(blob.TARGET, 'Charlie')

    def test_make_config_params_precedence(self):
        hello = self._hello()
        _, params = hello.make_config_params('default_tool', TARGET='Zed')
        self.assertEqual(params['TARGET'], 'Zed')

    def test_get_config_defaults_feeds_schema(self):
        hello = self._hello()
        hello.get_config_defaults = lambda: {'TARGET': 'Ada'}
        hello.set_config('default_tool')
        self.assertEqual(hello.c.TARGET, 'Ada')

    def test_unknown_config_name_raises(self):
        hello = self._hello()
        with self.assertRaises(KeyError):
            hello.set_config('does_not_exist')
        # optional=True swallows the unknown name and leaves c unset.
        hello.set_config('does_not_exist', optional=True)
        self.assertIsNone(hello.c)

    def test_apply_config_layers_onto_existing_blob(self):
        hello = self._hello()
        hello.set_config('default_tool')
        with self.assertRaises(BadConf):
            # default_tool already ran; super_tool is in the same group.
            hello.apply_config('super_tool')

    def test_module_without_config_ctx(self):
        step = engine._Engine()._instantiate_module('step', [])
        self.assertIsNone(step._config_ctx)
        # optional make on a context-less module yields no blob.
        self.assertIsNone(step.make_config(optional=True))
        with self.assertRaises(AssertionError):
            step.make_config_params(None)


class EngineConfigDiscoveryTest(unittest.TestCase):

    def test_loads_hello_context(self):
        ctx = engine._load_config_ctx('hello')
        self.assertIsInstance(ctx, ConfigContext)
        self.assertEqual(set(ctx.CONFIG_ITEMS),
                         {'BASE', 'super_tool', 'default_tool'})

    def test_no_config_py_returns_none(self):
        self.assertIsNone(engine._load_config_ctx('step'))


if __name__ == '__main__':
    unittest.main()
