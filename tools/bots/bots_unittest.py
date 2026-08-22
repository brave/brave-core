#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Unit tests for bots.py. Run: python3 brave/tools/bots/bots_unittest.py"""

import json
import os
import tempfile
import unittest

import bots


def _write(path, text):
    with open(path, 'w', encoding='utf-8') as f:
        f.write(text)


class FlattenTest(unittest.TestCase):

    def setUp(self):
        self.mixins = {
            'release': {
                'gn_args': {
                    'is_debug': False,
                    'is_official_build': True
                }
            },
            'x64': {
                'gn_args': {
                    'target_cpu': 'x64'
                }
            },
            'override_cpu': {
                'gn_args': {
                    'target_cpu': 'arm64'
                }
            },
            # 'parent' sets is_debug=True itself, then its nested 'release' mixin
            # flips is_debug back to False (nested overrides parent's own).
            'parent': {
                'gn_args': {
                    'is_debug': True
                },
                'mixins': ['release']
            },
            'cycle_a': {
                'mixins': ['cycle_b']
            },
            'cycle_b': {
                'mixins': ['cycle_a']
            },
        }
        self.configs = {
            'simple': ['release', 'x64'],
            'last_wins': ['x64', 'override_cpu'],
            'nested_override': ['parent'],
            'bad_mixin': ['does_not_exist'],
            'cyclic': ['cycle_a'],
        }

    def flatten(self, name):
        return bots.flatten_config(self.configs, self.mixins, name)

    def test_simple_compose(self):
        self.assertEqual(self.flatten('simple'), {
            'is_debug': False,
            'is_official_build': True,
            'target_cpu': 'x64',
        })

    def test_last_mixin_wins_on_dup_key(self):
        self.assertEqual(self.flatten('last_wins')['target_cpu'], 'arm64')

    def test_nested_mixin_overrides_parent_own_args(self):
        self.assertEqual(self.flatten('nested_override')['is_debug'], False)

    def test_unknown_mixin_raises(self):
        with self.assertRaises(bots.BotsError):
            self.flatten('bad_mixin')

    def test_unknown_config_raises(self):
        with self.assertRaises(bots.BotsError):
            self.flatten('missing')

    def test_cycle_raises(self):
        with self.assertRaises(bots.BotsError):
            self.flatten('cyclic')


class RenderTest(unittest.TestCase):

    def test_render_shape_sorted_and_trailing_newline(self):
        out = bots.render({'b': 1, 'a': True})
        self.assertTrue(out.endswith('\n'))
        self.assertEqual(json.loads(out), {'gn_args': {'a': True, 'b': 1}})
        # Keys sorted: "a" appears before "b".
        self.assertLess(out.index('"a"'), out.index('"b"'))


class BuilderMatrixTest(unittest.TestCase):

    def test_duplicate_bot_across_groups_raises(self):
        contents = {
            'mixins': {
                'm': {
                    'gn_args': {
                        'x': 1
                    }
                }
            },
            'configs': {
                'c': ['m']
            },
            'builder_groups': {
                'g1': {
                    'bot': 'c'
                },
                'g2': {
                    'bot': 'c'
                },
            },
        }
        with self.assertRaises(bots.BotsError):
            list(bots.iter_builders(contents))

    def test_generate_produces_expected_path_and_contents(self):
        contents = {
            'mixins': {
                'linux': {
                    'gn_args': {
                        'target_os': 'linux'
                    }
                }
            },
            'configs': {
                'c': ['linux']
            },
            'builder_groups': {
                'defaults.linux': {
                    'linux-x64-release': 'c'
                }
            },
        }
        files = bots.generate(contents)
        self.assertEqual(len(files), 1)
        (path, text), = files.items()
        self.assertTrue(
            path.replace(os.sep,
                         '/').endswith('brave/build/defaults/generated/'
                                       'linux-x64-release/gn-args.json'))
        self.assertEqual(json.loads(text), {'gn_args': {'target_os': 'linux'}})


class RealConfigTest(unittest.TestCase):

    def test_real_configs_pyl_validates(self):
        contents = bots.load_config(bots._DEFAULT_CONFIG)
        for _group, _bot, config_name in bots.iter_builders(contents):
            bots.flatten_config(contents['configs'], contents['mixins'],
                                config_name)

    def test_load_rejects_non_dict_top_level(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, 'bad.pyl')
            _write(path, '[1, 2, 3]')
            with self.assertRaises(bots.BotsError):
                bots.load_config(path)


if __name__ == '__main__':
    unittest.main()
