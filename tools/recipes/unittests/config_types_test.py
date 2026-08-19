#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the platform-independent `Path` in config_types.py."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
import config_types
from config_types import (Path, RelativeToDifferentBases, RelativeToNotParent,
                          ResolvedBasePath, reset_global_variable_assignments)


def _base(token='[CACHE]'):
    return ResolvedBasePath(token)


class PathConstructionTest(unittest.TestCase):

    def setUp(self):
        config_types.Path._OS_SEP = '/'

    def tearDown(self):
        reset_global_variable_assignments()

    def test_splits_on_slash_and_drops_empty_and_dot_pieces(self):
        p = Path(_base(), 'a/b', '', '.', 'c')
        self.assertEqual(p.pieces, ('a', 'b', 'c'))

    def test_dotdot_collapses_against_preceding_piece(self):
        p = Path(_base(), 'a', 'b', '..')
        self.assertEqual(p.pieces, ('a', ))

    def test_dotdot_above_base_raises(self):
        with self.assertRaises(ValueError):
            Path(_base(), '..', 'something')

    def test_non_str_piece_raises(self):
        with self.assertRaises(ValueError):
            Path(_base(), 123)

    def test_non_resolved_base_path_raises(self):
        with self.assertRaises(ValueError):
            Path('[CACHE]', 'a')

    def test_backslash_piece_before_os_sep_known_raises(self):
        config_types.Path._OS_SEP = None
        with self.assertRaises(ValueError):
            Path(_base(), 'a\\b')

    def test_backslash_piece_split_only_when_os_sep_is_backslash(self):
        config_types.Path._OS_SEP = '\\'
        p = Path(_base(), 'a\\b\\c')
        self.assertEqual(p.pieces, ('a', 'b', 'c'))

        config_types.Path._OS_SEP = '/'
        p = Path(_base(), 'a\\b')
        self.assertEqual(p.pieces, ('a\\b', ))


class PathJoinTest(unittest.TestCase):

    def setUp(self):
        config_types.Path._OS_SEP = '/'

    def tearDown(self):
        reset_global_variable_assignments()

    def test_truediv_appends_a_piece(self):
        p = Path(_base()) / 'foo' / 'bar'
        self.assertEqual(p.pieces, ('foo', 'bar'))

    def test_joinpath_appends_multiple_pieces(self):
        p = Path(_base()).joinpath('foo', 'bar')
        self.assertEqual(p.pieces, ('foo', 'bar'))

    def test_joinpath_no_pieces_returns_self(self):
        p = Path(_base(), 'foo')
        self.assertIs(p.joinpath(), p)

    def test_joinpath_with_a_path_piece_reroots(self):
        other = Path(_base('[HOME]'), 'x')
        p = Path(_base(), 'foo').joinpath('ignored', other, 'y')
        self.assertEqual(p.base, _base('[HOME]'))
        self.assertEqual(p.pieces, ('x', 'y'))


class PathPartsTest(unittest.TestCase):

    def setUp(self):
        config_types.Path._OS_SEP = '/'

    def tearDown(self):
        reset_global_variable_assignments()

    def test_parent_and_parents(self):
        p = Path(_base(), 'foo', 'bar', 'baz')
        self.assertEqual(p.parent.pieces, ('foo', 'bar'))
        self.assertEqual([x.pieces for x in p.parents], [('foo', 'bar'),
                                                         ('foo', ), ()])

    def test_name(self):
        self.assertEqual(Path(_base(), 'foo', 'bar').name, 'bar')

    def test_stem_and_suffix(self):
        p = Path(_base(), 'dir', 'foo.tar.gz')
        self.assertEqual(p.stem, 'foo.tar')
        self.assertEqual(p.suffix, '.gz')
        self.assertEqual(p.suffixes, ['.tar', '.gz'])

    def test_suffix_empty_when_no_dot(self):
        self.assertEqual(Path(_base(), 'foo').suffix, '')
        self.assertEqual(Path(_base(), 'foo').suffixes, [])


class PathStrTest(unittest.TestCase):

    def tearDown(self):
        reset_global_variable_assignments()

    def test_raises_before_os_sep_is_set(self):
        config_types.Path._OS_SEP = None
        with self.assertRaises(ValueError):
            str(Path(_base(), 'foo'))

    def test_renders_with_slash(self):
        config_types.Path._OS_SEP = '/'
        self.assertEqual(str(Path(_base(), 'foo', 'bar')), '[CACHE]/foo/bar')

    def test_renders_with_backslash_when_simulating_windows(self):
        config_types.Path._OS_SEP = '\\'
        self.assertEqual(str(Path(_base(), 'foo', 'bar')), '[CACHE]\\foo\\bar')

    def test_as_posix_ignores_os_sep(self):
        config_types.Path._OS_SEP = '\\'
        self.assertEqual(
            Path(_base(), 'foo', 'bar').as_posix(), '[CACHE]/foo/bar')


class PathRelativeToTest(unittest.TestCase):

    def setUp(self):
        config_types.Path._OS_SEP = '/'

    def tearDown(self):
        reset_global_variable_assignments()

    def test_simple_child(self):
        self.assertEqual(
            Path(_base(), 'foo', 'bar').relative_to(Path(_base(), 'foo')),
            'bar')

    def test_not_a_parent_raises_without_walk_up(self):
        with self.assertRaises(RelativeToNotParent):
            Path(_base(), 'foo').relative_to(Path(_base(), 'bar'))

    def test_not_a_parent_walks_up_when_allowed(self):
        self.assertEqual(
            Path(_base(), 'foo').relative_to(Path(_base(), 'bar'),
                                             walk_up=True), '../foo')

    def test_different_bases_raises(self):
        with self.assertRaises(RelativeToDifferentBases):
            Path(_base(), 'foo').relative_to(Path(_base('[HOME]'), 'bar'))


class PathEqualityAndHashTest(unittest.TestCase):

    def setUp(self):
        config_types.Path._OS_SEP = '/'

    def tearDown(self):
        reset_global_variable_assignments()

    def test_equal_paths_are_equal(self):
        self.assertEqual(Path(_base(), 'foo'), Path(_base(), 'foo'))

    def test_compares_equal_to_its_own_str(self):
        p = Path(_base(), 'foo')
        self.assertEqual(p, str(p))

    def test_lt_compares_to_str(self):
        self.assertLess(Path(_base(), 'a'), '[CACHE]/b')

    def test_hashable_and_usable_as_a_dict_key(self):
        p1 = Path(_base(), 'foo')
        p2 = Path(_base(), 'foo')
        self.assertEqual(hash(p1), hash(p2))
        self.assertEqual({p1: 'x'}[p2], 'x')


if __name__ == '__main__':
    unittest.main()
