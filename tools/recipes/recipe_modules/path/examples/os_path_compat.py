# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `path` module's `os.path`-equivalent helpers
(`dirname`/`basename`/`split`/`splitext`/`join`/`normpath`/`abspath`/
`realpath`/`is_absolute`/`assert_absolute`/`expandvars`/`mkstemp`) and its
test-only `mock_add_file`/`mock_add_directory`/`mock_copy_paths`/
`mock_remove_paths` seams.
"""

from __future__ import annotations

import post_process

DEPS = ['context', 'path', 'platform', 'step']


def RunSteps(api):
    sample = api.path.chromium_src / 'chrome' / 'VERSION'

    # dirname/basename/split: a Path argument keeps dirname a Path; basename
    # is always a str regardless of the argument's type.
    assert api.path.dirname(sample) == api.path.chromium_src / 'chrome'
    assert api.path.basename(sample) == 'VERSION'
    assert api.path.split(sample) == (api.path.chromium_src / 'chrome',
                                      'VERSION')

    # Same, but for a plain string argument -- dirname then returns a str.
    sample_str = api.path.join(str(api.path.chromium_src), 'chrome', 'VERSION')
    assert api.path.dirname(sample_str) == api.path.join(
        str(api.path.chromium_src), 'chrome')
    assert api.path.basename(sample_str) == 'VERSION'

    # splitext strips only the last extension, on both a Path and a str.
    archive = api.path.out / 'archive.tar.gz'
    assert api.path.splitext(archive) == (api.path.out / 'archive.tar', '.gz')
    archive_str = api.path.join(str(api.path.out), 'archive.tar.gz')
    assert api.path.splitext(archive_str) == (api.path.join(
        str(api.path.out), 'archive.tar'), '.gz')

    # normpath collapses '..'/'.' lexically.
    messy = api.path.join(str(api.path.workspace), 'a', '..', 'b')
    assert api.path.normpath(messy) == api.path.join(str(api.path.workspace),
                                                     'b')

    # abspath/realpath expand '~' and fully resolve, exactly like abs().
    assert api.path.abspath('~/x') == str(api.path.home() / 'x')
    assert api.path.realpath('~/x') == str(api.path.home() / 'x')

    # is_absolute/assert_absolute.
    assert api.path.is_absolute(api.path.workspace)
    assert not api.path.is_absolute('relative/path')
    api.path.assert_absolute(api.path.workspace)
    raised = False
    try:
        api.path.assert_absolute('relative/path')
    except AssertionError as exc:
        raised = True
        assert 'not absolute' in str(exc), exc
    assert raised, 'expected assert_absolute to raise on a relative path'

    # expandvars substitutes ${VAR} from the context module's current env.
    with api.context(env={'BUILD_TAG': 'v1'}):
        assert api.path.expandvars('${BUILD_TAG}/out') == 'v1/out'

    # mkstemp: like mkdtemp, but for a file.
    scratch_file = api.path.mkstemp('scratch')
    assert scratch_file.parent == api.path.cleanup_dir
    api.step('mkstemp', ['echo', str(scratch_file)])

    # mock_add_file/mock_add_directory/mock_copy_paths/mock_remove_paths are
    # test-only seams for marking simulated filesystem state mid-run (no-ops
    # in production) -- checked via step output + post_process below, since
    # asserting the result directly wouldn't hold outside simulation.
    scratch = api.path.out / 'scratch.txt'
    api.path.mock_add_file(scratch)
    api.step('scratch is file', ['echo', str(api.path.is_file(scratch))])

    scratch_dir = api.path.out / 'scratch_dir'
    api.path.mock_add_directory(scratch_dir)
    api.step('scratch dir is dir', ['echo', str(api.path.is_dir(scratch_dir))])

    copy_dest = api.path.out / 'scratch_copy.txt'
    api.path.mock_copy_paths(scratch, copy_dest)
    api.step('copy is file', ['echo', str(api.path.is_file(copy_dest))])

    api.path.mock_remove_paths(scratch)
    api.step('scratch removed', ['echo', str(api.path.is_file(scratch))])
    api.step('copy survives removal',
             ['echo', str(api.path.is_file(copy_dest))])


def GenTests(api):
    # Run under both simulated platforms: every assertion above is built
    # entirely from other `api.path.*` calls (never a hardcoded '/' or '\\'),
    # so it should hold either way -- the same proof of separator-independence
    # as `examples/full.py`.
    for plat in ('linux', 'win'):
        sep = '\\' if plat == 'win' else '/'
        yield api.test(
            plat,
            api.platform.name(plat),
            api.post_process(post_process.StepCommandContains, 'mkstemp',
                             [f'[WORKSPACE]{sep}rc{sep}scratch_tmp_1']),
            api.post_process(post_process.StepCommandContains,
                             'scratch is file', ['True']),
            api.post_process(post_process.StepCommandContains,
                             'scratch dir is dir', ['True']),
            api.post_process(post_process.StepCommandContains, 'copy is file',
                             ['True']),
            api.post_process(post_process.StepCommandContains,
                             'scratch removed', ['False']),
            api.post_process(post_process.StepCommandContains,
                             'copy survives removal', ['True']),
            api.post_process(post_process.StatusSuccess),
        )
