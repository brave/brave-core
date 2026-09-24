# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `git_cache` module."""

from __future__ import annotations

import post_process

DEPS = ['env', 'git_cache', 'path', 'raw_io', 'step']

_URL = 'https://chromium.googlesource.com/chromium/src.git'


def RunSteps(api):
    # Everything here works against whatever cache `$GIT_CACHE_PATH` names.
    api.step('cache path', ['echo', api.git_cache.validate()])

    api.git_cache.populate(_URL)
    # An extra ref and a commit are fetched beyond the mirror's
    # `refs/heads/*`; asking for `refs/tags/*` is how tags reach a mirror at
    # all.
    api.git_cache.populate(_URL,
                           ref='refs/tags/*',
                           commit='c0ffee' * 6,
                           no_fetch_tags=False,
                           step_name='populate with tags')

    api.step('mirror', ['echo', api.git_cache.mirror_dir(_URL)])


def GenTests(api):
    _mirror = '/b/cache/chromium.googlesource.com-chromium-src'

    # The environment names the cache; nothing else configures it.
    yield api.test(
        'cache from environment',
        api.env.set('GIT_CACHE_PATH', '/b/cache'),
        api.path.dirs('/b/cache'),
        # The first populate() call's mirror already exists *before* it
        # runs (as it would on any second-or-later run in production) --
        # this is the case that matters: auto-gc must be disabled ahead of
        # that call's own `git fetch`, not after it, since that fetch is
        # exactly what can trigger the OOM-prone auto-maintenance.
        api.step_data('git cache populate exists (before)',
                      stdout=api.raw_io.output_text(f'{_mirror}\n')),
        # The second populate() call's mirror doesn't exist yet going in
        # (unseeded "before" check, so empty stdout) but does by the time
        # that call returns -- simulating a call that bootstraps it fresh.
        api.step_data('populate with tags exists (after)',
                      stdout=api.raw_io.output_text(f'{_mirror}\n')),
        api.step_data('git cache exists',
                      stdout=api.raw_io.output_text(f'{_mirror}\n')),
        api.post_process(post_process.StepCommandContains, 'cache path',
                         ['/b/cache']),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate',
                         ['--reset-fetch-config', '--no-fetch-tags']),
        api.post_process(post_process.StepCommandContains,
                         'populate with tags',
                         ['--ref', 'refs/tags/*', '--commit']),
        # `--no-fetch-tags` is only about tags git would follow on its own, so
        # asking for them explicitly means dropping it.
        api.post_process(post_process.StepCommandDoesNotContain,
                         'populate with tags', ['--no-fetch-tags']),
        api.post_process(post_process.StepCommandContains, 'mirror',
                         [_mirror]),
        # The already-existing mirror gets auto-gc disabled *before* its
        # fetch, not just after -- otherwise a fetch that hangs or gets
        # OOM-killed would mean the disable step downstream never runs. The
        # actual config-setting is the `git` module's `disable_auto_gc`
        # (see its own example), invoked here with `cwd=_mirror` rather than
        # `--git-dir` -- git's own repository discovery resolves a bare
        # mirror from `cwd` just as well as a working tree.
        api.post_process(post_process.StepCommandContains,
                         'git cache populate disable (before): gc.auto=0',
                         ['config', 'gc.auto', '0']),
        api.post_process(
            post_process.StepCommandContains,
            'git cache populate disable (before): '
            'maintenance.gc.enabled=false',
            ['config', 'maintenance.gc.enabled', 'false']),
        # Nothing to configure before the first fetch of a mirror that
        # doesn't exist yet...
        api.post_process(post_process.DoesNotRun,
                         'populate with tags disable (before): gc.auto=0'),
        # ...but it exists once that call bootstraps it, so the *next* use
        # of this mirror is still protected.
        api.post_process(post_process.StepCommandContains,
                         'populate with tags disable (after): gc.auto=0',
                         ['config', 'gc.auto', '0']),
        api.post_process(post_process.StatusSuccess),
    )
    # No cache at all is a hard error rather than an uncached run.
    yield api.test(
        'no cache configured',
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # A configured path that is not a directory is equally fatal.
    yield api.test(
        'cache path is not a directory',
        api.env.set('GIT_CACHE_PATH', '/b/nope'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
