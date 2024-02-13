# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

# A override file to add brave perf directories to src/tools/perf/

import os

import override_utils


def _GetSrcDir():
    return os.path.abspath(
        os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir,
                     os.pardir, os.pardir))


def _GetBravePerfDir(*dirs):
    return os.path.join('brave', 'tools', 'perf', *dirs)


def _GetDiscoverClassesExtras(rel_start_dir, rel_top_level):
    if rel_top_level == os.path.join('tools', 'perf', 'page_sets'):
        if rel_start_dir == 'system_health':
            return 'brave_system_health', _GetBravePerfDir('brave_page_sets')
    if rel_top_level == os.path.join('tools', 'perf'):
        if rel_start_dir == 'benchmarks':
            return 'brave_benchmarks', _GetBravePerfDir()
        if rel_start_dir == 'page_sets':
            return 'brave_page_sets', _GetBravePerfDir()

    return None


@override_utils.override_function(globals())
def DiscoverClasses(original_function, start_dir, top_level_dir, *args,
                    **kwargs):
    original = original_function(start_dir, top_level_dir, *args, **kwargs)
    rel_top_level_dir = os.path.relpath(top_level_dir, _GetSrcDir())
    rel_start_dir = os.path.relpath(start_dir, top_level_dir)

    brave_extras = _GetDiscoverClassesExtras(rel_start_dir, rel_top_level_dir)
    if brave_extras is None:
        return original
    brave_rel_start, brave_rel_top_level = brave_extras

    top_level_dir = os.path.join(_GetSrcDir(), brave_rel_top_level)
    rel_start_dir = os.path.join(top_level_dir, brave_rel_start)
    brave_classes = original_function(rel_start_dir, top_level_dir, *args,
                                      **kwargs)

    brave_classes.update(original)
    return brave_classes
