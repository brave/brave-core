#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for config.py: freeze/thaw/FrozenDict, GnArgsRegistry and
BuildersRegistry."""

# Some tests read `_resolved` directly to check memoization, which is
# otherwise unobservable from the public API.

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# _UNSET is private (module-internal sentinel); tests reach for it directly
# rather than duplicating it.
from config import (
    AnonymousGnConfig,
    BuildersRegistry,
    builder_config,
    check_builder_spec,
    ConfigError,
    Defaults,
    FrozenDict,
    GnArgsRegistry,
    _UNSET,
    freeze,
    thaw,
)


class FreezeThawTest(unittest.TestCase):
    def test_freeze_dict_becomes_frozen_dict(self):
        frozen = freeze({'a': 1, 'b': 2})
        self.assertIsInstance(frozen, FrozenDict)
        self.assertEqual(dict(frozen), {'a': 1, 'b': 2})
        hash(frozen)  # Must not raise.

    def test_freeze_list_and_tuple_become_tuple(self):
        self.assertEqual(freeze([1, 2, 3]), (1, 2, 3))
        self.assertEqual(freeze((1, 2, 3)), (1, 2, 3))

    def test_freeze_set_becomes_frozenset(self):
        self.assertEqual(freeze({1, 2, 3}), frozenset({1, 2, 3}))

    def test_freeze_is_recursive(self):
        frozen = freeze({'a': [1, {2, 3}], 'b': {'c': 4}})
        self.assertEqual(frozen['a'], (1, frozenset({2, 3})))
        self.assertIsInstance(frozen['b'], FrozenDict)

    def test_freeze_unhashable_leaf_raises(self):

        class Unhashable:
            __hash__ = None

        with self.assertRaises(TypeError):
            freeze({'a': Unhashable()})

    def test_freeze_result_is_independent_of_original(self):
        original = {'a': [1, 2]}
        frozen = freeze(original)
        original['a'].append(3)
        original['b'] = 4
        self.assertEqual(frozen['a'], (1, 2))
        self.assertNotIn('b', frozen)

    def test_thaw_reverses_freeze(self):
        original = {'a': [1, {'b': 2}], 'c': {3, 4}}
        self.assertEqual(thaw(freeze(original)), original)

    def test_frozen_dict_equality_with_plain_dict(self):
        self.assertEqual(FrozenDict(a=1, b=2), {'a': 1, 'b': 2})

    def test_frozen_dict_repr(self):
        self.assertEqual(repr(FrozenDict(a=1)), "FrozenDict([('a', 1)])")


class GnArgsConfigTest(unittest.TestCase):
    def setUp(self):
        self.registry = GnArgsRegistry()

    def test_named_config_returns_none(self):
        self.assertIsNone(
            self.registry.config(name='linux', args={'target_os': 'linux'})
        )

    def test_duplicate_name_raises(self):
        self.registry.config(name='linux', args={'target_os': 'linux'})
        with self.assertRaises(ConfigError):
            self.registry.config(name='linux', args={'target_os': 'linux'})

    def test_anonymous_config_returns_struct(self):
        anon = self.registry.config(
            args={'is_asan': True},
            configs=['linux'],
            args_file='//foo.gni',
            secrets={'k': 'ENV_K'},
        )
        self.assertIsInstance(anon, AnonymousGnConfig)
        self.assertEqual(dict(anon.gn_args), {'is_asan': True})
        self.assertEqual(anon.configs, ('linux',))
        self.assertEqual(anon.args_file, '//foo.gni')
        self.assertEqual(dict(anon.secrets), {'k': 'ENV_K'})
        repr(anon)  # Must not raise.

    def test_config_args_are_frozen_against_later_mutation(self):
        args = {'target_os': 'linux', 'target_cpu': 'x64'}
        self.registry.config(name='linux', args=args)
        args['target_os'] = 'mac'  # Must not affect the registered config.
        self.assertEqual(
            self.registry.resolve('linux')['gn_args'],
            {
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
        )


class GnArgsResolveTest(unittest.TestCase):
    def setUp(self):
        self.registry = GnArgsRegistry()

    def test_undefined_config_raises(self):
        with self.assertRaises(ConfigError):
            self.registry.resolve('nope')

    def test_undefined_included_config_raises(self):
        self.registry.config(name='asan', configs=['nope'])
        with self.assertRaises(ConfigError):
            self.registry.resolve('asan')

    def test_missing_target_os_raises(self):
        self.registry.config(name='x64', args={'target_cpu': 'x64'})
        with self.assertRaises(ConfigError):
            self.registry.resolve('x64')

    def test_missing_target_cpu_raises(self):
        self.registry.config(name='linux', args={'target_os': 'linux'})
        with self.assertRaises(ConfigError):
            self.registry.resolve('linux')

    def test_simple_resolution(self):
        self.registry.config(name='linux', args={'target_os': 'linux'})
        self.registry.config(name='x64', args={'target_cpu': 'x64'})
        self.registry.config(name='builder', configs=['linux', 'x64'])
        self.assertEqual(
            self.registry.resolve('builder'),
            {
                'gn_args': {
                    'target_os': 'linux',
                    'target_cpu': 'x64',
                },
            },
        )

    def test_own_args_beat_included_configs(self):
        self.registry.config(
            name='base',
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
                'is_debug': True,
            },
        )
        self.registry.config(
            name='derived', configs=['base'], args={'is_debug': False}
        )
        self.assertFalse(
            self.registry.resolve('derived')['gn_args']['is_debug']
        )

    def test_later_config_in_list_beats_earlier(self):
        self.registry.config(
            name='base',
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
        )
        self.registry.config(name='a', configs=['base'], args={'v': 1})
        self.registry.config(name='b', configs=['base'], args={'v': 2})
        self.registry.config(name='ab', configs=['a', 'b'])
        self.registry.config(name='ba', configs=['b', 'a'])
        self.assertEqual(self.registry.resolve('ab')['gn_args']['v'], 2)
        self.assertEqual(self.registry.resolve('ba')['gn_args']['v'], 1)

    def test_single_args_file_survives(self):
        self.registry.config(
            name='base',
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
            args_file='//base.gni',
        )
        self.assertEqual(
            self.registry.resolve('base')['args_file'], '//base.gni'
        )

    def test_two_args_files_in_one_resolution_raises(self):
        self.registry.config(name='a', args_file='//a.gni')
        self.registry.config(name='b', args_file='//b.gni')
        self.registry.config(
            name='ab',
            configs=['a', 'b'],
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
        )
        with self.assertRaises(ConfigError):
            self.registry.resolve('ab')

    def test_diamond_include_of_the_same_args_file_still_raises(self):
        # Faithful to upstream: the merge only ever sees each child's already-
        # resolved args_file string, not node identity, so two siblings that
        # each transitively carry the *same* args_file still conflict when a
        # common parent includes both - merging is not deduplicated by value.
        self.registry.config(name='base', args_file='//base.gni')
        self.registry.config(name='a', configs=['base'])
        self.registry.config(name='b', configs=['base'])
        self.registry.config(
            name='ab',
            configs=['a', 'b'],
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
        )
        with self.assertRaises(ConfigError):
            self.registry.resolve('ab')

    def test_cycle_raises(self):
        self.registry.config(name='a', configs=['b'])
        self.registry.config(name='b', configs=['a'])
        with self.assertRaises(ConfigError):
            self.registry.resolve('a')

    def test_secrets_merge_like_gn_args(self):
        self.registry.config(
            name='base',
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
            secrets={'shared_key': 'BASE_ENV'},
        )
        self.registry.config(
            name='derived',
            configs=['base'],
            secrets={'derived_key': 'DERIVED_ENV'},
        )
        self.assertEqual(
            self.registry.resolve('derived')['secrets'],
            {
                'shared_key': 'BASE_ENV',
                'derived_key': 'DERIVED_ENV',
            },
        )

    def test_no_secrets_key_when_empty(self):
        self.registry.config(
            name='linux',
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
        )
        self.assertNotIn('secrets', self.registry.resolve('linux'))

    def test_no_args_file_key_when_empty(self):
        self.registry.config(
            name='linux',
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
        )
        self.assertNotIn('args_file', self.registry.resolve('linux'))

    def test_resolution_is_memoized(self):
        self.registry.config(name='linux', args={'target_os': 'linux'})
        self.registry.config(name='x64', args={'target_cpu': 'x64'})
        self.registry.config(name='builder', configs=['linux', 'x64'])
        self.assertNotIn('linux', self.registry._resolved)
        self.registry.resolve('builder')
        self.assertIn('linux', self.registry._resolved)
        self.assertIn('x64', self.registry._resolved)
        self.assertIn('builder', self.registry._resolved)

    def test_resolve_result_is_a_copy(self):
        self.registry.config(
            name='linux',
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
        )
        result = self.registry.resolve('linux')
        result['gn_args']['target_os'] = 'mac'
        self.assertEqual(
            self.registry.resolve('linux')['gn_args']['target_os'], 'linux'
        )

    def test_shared_subgraph_resolves_consistently_across_roots(self):
        self.registry.config(
            name='base',
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
        )
        self.registry.config(name='a', configs=['base'], args={'v': 1})
        self.registry.config(name='b', configs=['base'], args={'v': 2})
        self.registry.resolve('a')
        self.registry.resolve('b')
        # `base` was resolved once, memoized, and reused for both.
        self.assertEqual(
            self.registry.resolve('base')['gn_args'],
            {
                'target_os': 'linux',
                'target_cpu': 'x64',
            },
        )


class DefaultsTest(unittest.TestCase):
    def setUp(self):
        self.defaults = Defaults(channel=None, notifies=())

    def test_get_returns_value_when_given(self):
        self.assertEqual(self.defaults.get('channel', 'beta'), 'beta')

    def test_get_falls_back_to_initial_default(self):
        self.assertIsNone(self.defaults.get('channel', _UNSET))

    def test_set_changes_the_fallback(self):
        self.defaults.set(channel='nightly')
        self.assertEqual(self.defaults.get('channel', _UNSET), 'nightly')

    def test_set_unknown_field_raises(self):
        with self.assertRaises(ConfigError):
            self.defaults.set(nope='x')

    def test_explicit_falsy_value_is_not_treated_as_unset(self):
        self.defaults.set(notifies=['a'])
        self.assertEqual(self.defaults.get('notifies', []), [])


class BuildersRegistryTest(unittest.TestCase):
    def setUp(self):
        self.gn_args = GnArgsRegistry()
        self.builders = BuildersRegistry(self.gn_args)

    def _targets(self):
        return self.builders.targets(
            compile=['brave:all'], tests=['brave_all_unit_tests']
        )

    def _spec(self):
        return builder_config.builder_spec(
            gclient_config=builder_config.gclient_config(),
            chromium_config=builder_config.chromium_config(
                target_platform=builder_config.target_platform.LINUX
            ),
        )

    def test_builder_requires_a_builder_spec(self):
        with self.assertRaisesRegex(ConfigError, 'builder_spec must be'):
            self.builders.builder(
                name='b',
                builder_spec=builder_config.gclient_config(),
                gn_args=self.gn_args.config(
                    args={'target_os': 'linux', 'target_cpu': 'x64'}
                ),
                targets=self._targets(),
            )

    def test_targets_fields_become_tuples(self):
        targets = self.builders.targets(compile=['brave:all'], tests=['a', 'b'])
        self.assertEqual(targets.compile, ('brave:all',))
        self.assertEqual(targets.tests, ('a', 'b'))

    def test_builder_with_named_gn_args_config(self):
        self.gn_args.config(
            name='asan',
            args={
                'target_os': 'linux',
                'target_cpu': 'x64',
                'is_asan': True,
            },
        )
        builder = self.builders.builder(
            name='linux-x64-asan-brave',
            builder_spec=self._spec(),
            gn_args='asan',
            targets=self._targets(),
        )
        self.assertEqual(builder.name, 'linux-x64-asan-brave')
        self.assertTrue(
            self.gn_args.resolve('linux-x64-asan-brave')['gn_args']['is_asan']
        )

    def test_builder_with_anonymous_gn_args_config(self):
        self.gn_args.config(name='linux', args={'target_os': 'linux'})
        self.gn_args.config(name='x64', args={'target_cpu': 'x64'})
        anon = self.gn_args.config(
            configs=['linux', 'x64'],
            args={'is_asan': True},
            secrets={'k': 'ENV_K'},
        )
        self.builders.builder(
            name='b',
            builder_spec=self._spec(),
            gn_args=anon,
            targets=self._targets(),
        )
        resolved = self.gn_args.resolve('b')
        self.assertEqual(
            resolved['gn_args'],
            {
                'target_os': 'linux',
                'target_cpu': 'x64',
                'is_asan': True,
            },
        )
        self.assertEqual(resolved['secrets'], {'k': 'ENV_K'})

    def test_builder_with_invalid_gn_args_raises(self):
        with self.assertRaises(ConfigError):
            self.builders.builder(
                name='b',
                builder_spec=self._spec(),
                gn_args=123,
                targets=self._targets(),
            )

    def test_duplicate_builder_name_raises(self):
        self.builders.builder(
            name='b',
            builder_spec=self._spec(),
            gn_args=self.gn_args.config(
                args={
                    'target_os': 'linux',
                    'target_cpu': 'x64',
                }
            ),
            targets=self._targets(),
        )
        with self.assertRaises(ConfigError):
            self.builders.builder(
                name='b',
                builder_spec=self._spec(),
                gn_args=self.gn_args.config(
                    args={
                        'target_os': 'linux',
                        'target_cpu': 'x64',
                    }
                ),
                targets=self._targets(),
            )

    def test_defaults_are_applied_and_overridable(self):
        self.builders.defaults.set(
            builder_group='brave.sanitizers',
            execution_timeout_mins=270,
            channel='nightly',
            notifies=['browser-sanitizers-bot'],
        )
        inherited = self.builders.builder(
            name='inherited',
            builder_spec=self._spec(),
            gn_args=self.gn_args.config(
                args={
                    'target_os': 'linux',
                    'target_cpu': 'x64',
                }
            ),
            targets=self._targets(),
        )
        self.assertEqual(inherited.builder_group, 'brave.sanitizers')
        self.assertEqual(inherited.execution_timeout_mins, 270)
        self.assertEqual(inherited.channel, 'nightly')
        self.assertEqual(inherited.notifies, ('browser-sanitizers-bot',))

        overridden = self.builders.builder(
            name='overridden',
            builder_spec=self._spec(),
            gn_args=self.gn_args.config(
                args={
                    'target_os': 'linux',
                    'target_cpu': 'x64',
                }
            ),
            targets=self._targets(),
            channel='beta',
            notifies=['browser-bot'],
        )
        self.assertEqual(overridden.builder_group, 'brave.sanitizers')
        self.assertEqual(overridden.channel, 'beta')
        self.assertEqual(overridden.notifies, ('browser-bot',))

    def test_get_and_all(self):
        b1 = self.builders.builder(
            name='b1',
            builder_spec=self._spec(),
            gn_args=self.gn_args.config(
                args={
                    'target_os': 'linux',
                    'target_cpu': 'x64',
                }
            ),
            targets=self._targets(),
        )
        self.assertIs(self.builders.get('b1'), b1)
        self.assertEqual(self.builders.all(), (b1,))
        with self.assertRaises(ConfigError):
            self.builders.get('nope')

    def test_registries_are_isolated_per_instance(self):
        other_gn_args = GnArgsRegistry()
        other_builders = BuildersRegistry(other_gn_args)
        other_builders.builder(
            name='b',
            builder_spec=self._spec(),
            gn_args=other_gn_args.config(
                args={
                    'target_os': 'linux',
                    'target_cpu': 'x64',
                }
            ),
            targets=self._targets(),
        )
        # A same-named builder in a separate registry pair does not collide.
        self.builders.builder(
            name='b',
            builder_spec=self._spec(),
            gn_args=self.gn_args.config(
                args={
                    'target_os': 'linux',
                    'target_cpu': 'x64',
                }
            ),
            targets=self._targets(),
        )
        # Both succeeded without an "already defined" clash: each pair's
        # `gn_args` node for 'b' lives in its own registry.
        self.assertIn('b', self.gn_args._nodes)
        self.assertIn('b', other_gn_args._nodes)
        self.assertIsNot(self.gn_args._nodes['b'], other_gn_args._nodes['b'])


class BuilderConfigTest(unittest.TestCase):
    def test_enums_hold_upstream_values(self):
        self.assertEqual(builder_config.target_platform.ANDROID, 'android')
        self.assertEqual(builder_config.host_platform.MAC, 'mac')
        self.assertEqual(builder_config.target_arch.INTEL, 'intel')
        self.assertEqual(builder_config.build_config.RELEASE, 'Release')
        self.assertIn('android', builder_config.target_platform.values)

    def test_enums_leave_out_platforms_brave_does_not_build(self):
        self.assertNotIn('chromeos', builder_config.target_platform.values)
        self.assertNotIn('fuchsia', builder_config.target_platform.values)
        self.assertNotIn('mips', builder_config.target_arch.values)
        with self.assertRaisesRegex(ConfigError, "unknown target_cpu: 'mips'"):
            builder_config.gclient_config(target_cpu=['mips'])

    def test_enums_cannot_be_reassigned(self):
        with self.assertRaises(TypeError):
            builder_config.target_platform.LINUX = 'mac'

    def test_gclient_config_lists_are_sorted_and_deduplicated(self):
        gclient_config = builder_config.gclient_config(
            target_os=[
                builder_config.target_platform.LINUX,
                builder_config.target_platform.ANDROID,
                builder_config.target_platform.LINUX,
            ],
            target_cpu=['x64'],
        )
        self.assertEqual(gclient_config.target_os, ('android', 'linux'))
        self.assertEqual(gclient_config.target_cpu, ('x64',))

    def test_gclient_config_defaults_to_the_host_only(self):
        gclient_config = builder_config.gclient_config()
        self.assertEqual(gclient_config.target_os, ())
        self.assertEqual(gclient_config.target_cpu, ())

    def test_gclient_config_rejects_a_bare_string(self):
        with self.assertRaisesRegex(ConfigError, 'target_os must be a list'):
            builder_config.gclient_config(target_os='linux')

    def test_gclient_config_rejects_unknown_values(self):
        with self.assertRaisesRegex(
            ConfigError, "unknown target_os: 'andriod'"
        ):
            builder_config.gclient_config(target_os=['andriod'])
        with self.assertRaisesRegex(ConfigError, "unknown target_cpu: 'amd64'"):
            builder_config.gclient_config(target_cpu=['amd64'])

    def test_chromium_config_rejects_unknown_values(self):
        for kwargs, message in (
            ({'target_platform': 'linux2'}, 'unknown target_platform'),
            (
                {'target_platform': 'linux', 'host_platform': 'ios'},
                'unknown host_platform',
            ),
            (
                {'target_platform': 'linux', 'build_config': 'release'},
                'unknown build_config',
            ),
            (
                {'target_platform': 'linux', 'target_arch': 'x64'},
                'unknown target_arch',
            ),
            (
                {'target_platform': 'linux', 'target_bits': 16},
                'unknown target_bits',
            ),
        ):
            with self.subTest(kwargs=kwargs):
                with self.assertRaisesRegex(ConfigError, message):
                    builder_config.chromium_config(**kwargs)

    def test_chromium_config_gn_target_cpu(self):
        for arch, bits, cpu in (
            ('intel', 64, 'x64'),
            ('intel', 32, 'x86'),
            ('arm', 64, 'arm64'),
            ('arm', 32, 'arm'),
        ):
            with self.subTest(arch=arch, bits=bits):
                self.assertEqual(
                    builder_config.chromium_config(
                        target_platform='linux',
                        target_arch=arch,
                        target_bits=bits,
                    ).gn_target_cpu(),
                    cpu,
                )
        self.assertIsNone(
            builder_config.chromium_config(
                target_platform='linux', target_bits=64
            ).gn_target_cpu()
        )

    def test_builder_spec_rejects_the_wrong_types(self):
        with self.assertRaisesRegex(ConfigError, 'gclient_config must be'):
            builder_config.builder_spec(
                gclient_config={},
                chromium_config=builder_config.chromium_config(
                    target_platform='linux'
                ),
            )
        with self.assertRaisesRegex(ConfigError, 'chromium_config must be'):
            builder_config.builder_spec(
                gclient_config=builder_config.gclient_config(),
                chromium_config='linux',
            )


class CheckBuilderSpecTest(unittest.TestCase):
    def _builder(self, *, target_os=(), **chromium_config):
        gn_args = GnArgsRegistry()
        builders = BuildersRegistry(gn_args)
        return builders.builder(
            name='b',
            builder_spec=builder_config.builder_spec(
                gclient_config=builder_config.gclient_config(
                    target_os=target_os
                ),
                chromium_config=builder_config.chromium_config(
                    **chromium_config
                ),
            ),
            gn_args=gn_args.config(
                args={'target_os': 'unused', 'target_cpu': 'unused'}
            ),
            targets=builders.targets(compile=[], tests=[]),
        )

    def test_matching_spec_passes(self):
        check_builder_spec(
            self._builder(
                target_platform='linux',
                host_platform='linux',
                target_arch='intel',
                target_bits=64,
            ),
            {'target_os': 'linux', 'target_cpu': 'x64'},
        )

    def test_target_platform_must_match_gn_target_os(self):
        with self.assertRaisesRegex(
            ConfigError, "target_platform 'android' does not match"
        ):
            check_builder_spec(
                self._builder(target_platform='android'),
                {'target_os': 'linux', 'target_cpu': 'x64'},
            )

    def test_target_arch_and_bits_must_match_gn_target_cpu(self):
        with self.assertRaisesRegex(
            ConfigError, "make GN target_cpu 'arm64', not 'x64'"
        ):
            check_builder_spec(
                self._builder(
                    target_platform='linux', target_arch='arm', target_bits=64
                ),
                {'target_os': 'linux', 'target_cpu': 'x64'},
            )

    def test_cross_platform_target_must_be_synced(self):
        with self.assertRaisesRegex(
            ConfigError, "builds for 'android' on 'linux'"
        ):
            check_builder_spec(
                self._builder(target_platform='android', host_platform='linux'),
                {'target_os': 'android', 'target_cpu': 'arm64'},
            )
        check_builder_spec(
            self._builder(
                target_os=['android'],
                target_platform='android',
                host_platform='linux',
            ),
            {'target_os': 'android', 'target_cpu': 'arm64'},
        )


if __name__ == '__main__':
    unittest.main()
