# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `path` module's filesystem seams."""

from __future__ import annotations

import post_process

DEPS = ['path', 'platform', 'step']


def RunSteps(api):
    if api.platform.is_win:
        assert api.path.sep == '\\'
        assert api.path.pathsep == ';'
    else:
        assert api.path.sep == '/'
        assert api.path.pathsep == ':'

    # Probe a seeded file, then create and re-probe a directory: the created
    # directory must "exist" for the rest of the run.
    if api.path.exists(api.path.chromium_src / 'chrome/VERSION'):
        api.step('found version', ['echo', 'present'])
    api.path.mkdir(api.path.out)
    if api.path.is_dir(api.path.out):
        api.step('out ready', ['echo', str(api.path.out)])
    # Exercise the home() seam and `~`/`~/...`-expansion via abs().
    api.step('home', ['echo', str(api.path.home())])
    api.step('home again', ['echo', str(api.path.abs('~'))])
    api.step('cache', ['echo', str(api.path.abs('~/cache'))])

    # Temporary directories live under the job's scratch space, and exist for
    # the rest of the run. Names are numbered per prefix, so two directories
    # with the same prefix don't collide.
    first = api.path.mkdtemp()
    second = api.path.mkdtemp()
    unpacked = api.path.mkdtemp('unpacked')
    assert api.path.is_dir(first) and api.path.is_dir(unpacked)
    assert first.parent == api.path.cleanup_dir, first
    api.step('temp dirs', ['echo', str(first), str(second), str(unpacked)])


def GenTests(api):
    # `config_types.Path._OS_SEP` is driven by the simulated platform, not the
    # real host running the test suite: run the same case under both `linux` and
    # `win`, and check the separator in the recorded commands actually flips,
    # independent of whatever machine runs this test.
    for plat in ('linux', 'win'):
        sep = '\\' if plat == 'win' else '/'
        yield api.test(
            f'seeded_{plat}',
            api.platform.name(plat),
            api.path.files('b/src/chrome/VERSION'),
            api.post_process(post_process.MustRun, 'found version'),
            api.post_process(post_process.MustRun, 'out ready'),
            api.post_process(post_process.StepCommandContains, 'out ready',
                             [f'[WORKSPACE]{sep}out']),
            api.post_process(post_process.StepCommandContains, 'home',
                             ['[HOME]']),
            api.post_process(post_process.StepCommandContains, 'home again',
                             ['[HOME]']),
            api.post_process(post_process.StepCommandContains, 'cache',
                             [f'[HOME]{sep}cache']),
            api.post_process(post_process.StepCommandContains, 'temp dirs', [
                f'[WORKSPACE]{sep}rc{sep}tmp_tmp_1',
                f'[WORKSPACE]{sep}rc{sep}tmp_tmp_2',
                f'[WORKSPACE]{sep}rc{sep}unpacked_tmp_1'
            ]),
            api.post_process(post_process.StatusSuccess),
        )
    yield api.test(
        'absent',
        api.post_process(post_process.DoesNotRun, 'found version'),
        api.post_process(post_process.MustRun, 'out ready'),
        api.post_process(post_process.StatusSuccess),
    )
