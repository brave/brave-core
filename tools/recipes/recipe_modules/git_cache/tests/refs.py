# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for how `git_cache` classifies refs."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_modules.git_cache.api import RefKind
from recipe_api import RecipeScriptApi
from recipe_modules import (
    env,
    git_cache,
    path,
    step,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    git_cache: git_cache.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    env: env.TEST_API
    path: path.TEST_API


def RunSteps(api: DEPS):
    parse = api.git_cache.parse_ref
    commit = 'ef35003457e93c278f911a334b06e4a5f8967e06'
    refs = {
        'tag': (RefKind.TAG, parse('refs/tags/v1.80.100')),
        'commit': (RefKind.COMMIT, parse(commit)),
        'ref': (RefKind.OTHER, parse('refs/branch-heads/6834')),
        'head': (RefKind.HEAD, parse('refs/heads/1.80.x')),
    }
    for kind, ref in refs.values():
        assert ref.kind == kind, (kind, ref)
    refs = {name: ref for name, (_, ref) in refs.items()}
    # A tag and any other ref are mirrored by name; a branch is mirrored
    # anyway, and a commit has its own switch.
    assert refs['tag'].populate_ref == 'refs/tags/v1.80.100'
    assert refs['ref'].populate_ref == 'refs/branch-heads/6834'
    assert refs['head'].populate_ref is None
    assert refs['commit'].populate_ref is None
    assert refs['commit'].commit == commit
    assert refs['head'].short_name == '1.80.x'
    assert refs['tag'].short_name == 'v1.80.100'

    # A bare name, which nothing tells a branch from a tag, a name git would
    # read as an option, and a ref that is neither a branch nor a tag when a
    # name is wanted, are all refused.
    for bad in ('main', '--upload-pack=evil'):
        try:
            parse(bad)
        except ValueError:
            api.step(f'refused {bad}', ['echo', 'refused'])
    try:
        refs['ref'].short_name  # pylint: disable=pointless-statement
    except ValueError:
        api.step('no short name', ['echo', 'none'])


def GenTests(api: TEST_DEPS):
    yield api.test(
        'refs',
        api.env.set('GIT_CACHE_PATH', '/b/cache'),
        api.path.dirs('/b/cache'),
        api.post_process(post_process.MustRun, 'refused main'),
        api.post_process(post_process.MustRun, 'refused --upload-pack=evil'),
        api.post_process(post_process.MustRun, 'no short name'),
        api.post_process(post_process.StatusSuccess),
    )
