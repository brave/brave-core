# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Mirror brave-core from the git cache into Gerrit.

Mirrors `master`, the version branches and the tags on them, force-pushed so
Gerrit follows GitHub. Tags on no mirrored branch are left out. Refs Gerrit already has
at the same object are not sent again.
"""

from __future__ import annotations

import re
from dataclasses import dataclass

import post_process
from PB.recipes.brave.gerrit.mirror_brave_core import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_gerrit,
    depot_tools,
    env,
    git,
    git_cache,
    path,
    raw_io,
)
from recipe_test_api import RecipeTestApi

BRAVE_CORE_URL = 'https://github.com/brave/brave-core.git'

MASTER = 'refs/heads/master'

_MIRRORED_BRANCH = re.compile(r'refs/heads/(master|\d+\.\d+\.x)')

# Commits per push while Gerrit's `master` is first seeded: one push of all of
# brave-core's history is more than Gerrit's unpacker takes, as with
# chromium/src in `gerrit/refresh_mirrors`.
SEED_CHUNK = 5000

# Refs per `git push`, so thousands of tags don't run into Gerrit's receive
# timeout.
REFS_PER_PUSH = 200


@dataclass
class DEPS(RecipeScriptApi):
    brave_gerrit: brave_gerrit.API
    depot_tools: depot_tools.API
    git: git.API
    git_cache: git_cache.API
    raw_io: raw_io.API


PROPERTIES = InputProperties


def RunSteps(api: DEPS, properties: InputProperties) -> None:
    # `git cache` is a depot_tools command.
    api.depot_tools.ensure_on_path()
    mirror_dir = api.git_cache.populate(
        BRAVE_CORE_URL,
        ref='refs/tags/*',
        no_fetch_tags=False,
        step_name='populate brave-core',
    )
    gerrit_url = api.brave_gerrit.ssh_url(properties.gerrit_user)

    branches = {
        ref: sha
        for ref, sha in _parse_refs(
            api.git(
                'for-each-ref',
                '--format=%(objectname)%09%(refname)',
                'refs/heads/',
                name='list mirror branches',
                cwd=mirror_dir,
                stdout=api.raw_io.output_text(),
            ).stdout
        ).items()
        if _MIRRORED_BRANCH.fullmatch(ref)
    }
    tags = _parse_refs(
        api.git(
            'for-each-ref',
            '--format=%(objectname)%09%(refname)',
            *(f'--merged={ref}' for ref in sorted(branches)),
            'refs/tags/',
            name='list mirror tags',
            cwd=mirror_dir,
            stdout=api.raw_io.output_text(),
        ).stdout
    )
    local = {**branches, **tags}
    remote = _parse_refs(
        api.git(
            'ls-remote',
            gerrit_url,
            'refs/heads/*',
            'refs/tags/*',
            name='list gerrit refs',
            cwd=mirror_dir,
            stdout=api.raw_io.output_text(),
        ).stdout
    )

    def push(refspecs: list[str], name: str) -> None:
        api.git(
            'push',
            '-o',
            'skip-validation',
            gerrit_url,
            *refspecs,
            name=name,
            cwd=mirror_dir,
        )

    if MASTER not in remote:
        history = api.git(
            'rev-list',
            '--reverse',
            '--first-parent',
            MASTER,
            name='list master history',
            cwd=mirror_dir,
            stdout=api.raw_io.output_text(),
        ).stdout.split()
        checkpoints = _checkpoints(history, SEED_CHUNK)
        for index, sha in enumerate(checkpoints, start=1):
            push(
                [f'+{sha}:{MASTER}'],
                f'seed master {index}/{len(checkpoints)}',
            )
        remote[MASTER] = local[MASTER]

    # Branches sort before tags, so tags arrive with their commits on Gerrit.
    changed = sorted(
        ref for ref, sha in local.items() if remote.get(ref) != sha
    )
    for start in range(0, len(changed), REFS_PER_PUSH):
        batch = changed[start : start + REFS_PER_PUSH]
        push(
            [f'+{ref}:{ref}' for ref in batch],
            f'push refs {start + 1}-{start + len(batch)} of {len(changed)}',
        )


def _parse_refs(output: str) -> dict[str, str]:
    """`<sha>\\t<ref>` lines as {ref: sha}, skipping the peeled `^{}` lines
    `ls-remote` adds for annotated tags."""
    refs = {}
    for line in output.splitlines():
        sha, _, ref = line.partition('\t')
        if ref and not ref.endswith('^{}'):
            refs[ref] = sha
    return refs


def _checkpoints(history: list[str], chunk: int) -> list[str]:
    """Every *chunk*th commit of *history*, oldest first, ending at its tip."""
    checkpoints = history[chunk - 1 :: chunk]
    if history and (not checkpoints or checkpoints[-1] != history[-1]):
        checkpoints.append(history[-1])
    return checkpoints


@dataclass
class TEST_DEPS(RecipeTestApi):
    env: env.TEST_API
    path: path.TEST_API
    raw_io: raw_io.TEST_API


def GenTests(api: TEST_DEPS):
    mirror = '/b/cache/github.com-brave-brave-core'
    gerrit_url = (
        'ssh://refresh-repos-bot@gerrit-ssh.brave.com:29418/'
        'brave-browser/brave-core'
    )

    def refs(*pairs: tuple[str, str]) -> str:
        return ''.join(f'{sha}\t{ref}\n' for sha, ref in pairs)

    def setup(branches: str, tags: str, remote: str):
        return (
            api.properties(gerrit_user='refresh-repos-bot')
            + api.env.set('GIT_CACHE_PATH', '/b/cache')
            + api.path.dirs('/b/cache')
            + api.env.on_path('gclient', '/depot_tools/gclient')
            + api.step_data(
                'populate brave-core exists (after)',
                stdout=api.raw_io.output_text(mirror),
            )
            + api.step_data(
                'list mirror branches', stdout=api.raw_io.output_text(branches)
            )
            + api.step_data(
                'list mirror tags', stdout=api.raw_io.output_text(tags)
            )
            + api.step_data(
                'list gerrit refs', stdout=api.raw_io.output_text(remote)
            )
        )

    mirror_branches = refs(
        ('a' * 40, 'refs/heads/1.85.x'),
        ('b' * 40, 'refs/heads/cr160'),
        ('c' * 40, MASTER),
        ('d' * 40, 'refs/heads/some-feature'),
    )
    mirror_tags = refs(('e' * 40, 'refs/tags/v1.85.1'))

    yield api.test(
        'seeds empty gerrit',
        setup(mirror_branches, mirror_tags, ''),
        # Only tags on the mirrored branches are listed.
        api.post_process(
            post_process.StepCommandContains,
            'list mirror tags',
            [
                '--merged=refs/heads/1.85.x',
                f'--merged={MASTER}',
                'refs/tags/',
            ],
        ),
        api.post_process(
            post_process.StepCommandDoesNotContain,
            'list mirror tags',
            ['--merged=refs/heads/cr160'],
        ),
        api.step_data(
            'list master history',
            stdout=api.raw_io.output_text(
                ''.join(f'{i:040x}\n' for i in range(1, SEED_CHUNK + 3))
            ),
        ),
        api.post_process(
            post_process.StepCommandContains,
            'populate brave-core',
            ['--ref', 'refs/tags/*'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'seed master 1/2',
            [gerrit_url, f'+{SEED_CHUNK:040x}:{MASTER}'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'seed master 2/2',
            [f'+{SEED_CHUNK + 2:040x}:{MASTER}'],
        ),
        # `master` was seeded, and other branches are not mirrored.
        api.post_process(
            post_process.StepCommandContains,
            'push refs 1-2 of 2',
            [
                '+refs/heads/1.85.x:refs/heads/1.85.x',
                '+refs/tags/v1.85.1:refs/tags/v1.85.1',
            ],
        ),
        api.post_process(post_process.StatusSuccess),
    )

    yield api.test(
        'pushes changed refs only',
        setup(
            mirror_branches,
            mirror_tags,
            refs(
                ('a' * 40, 'refs/heads/1.85.x'),
                ('0' * 40, MASTER),
                ('f' * 40, 'refs/tags/v1.85.0'),
                ('9' * 40, 'refs/tags/v1.85.0^{}'),
            ),
        ),
        api.post_process(post_process.DoesNotRun, 'list master history'),
        api.post_process(
            post_process.StepCommandContains,
            'push refs 1-2 of 2',
            [
                f'+{MASTER}:{MASTER}',
                '+refs/tags/v1.85.1:refs/tags/v1.85.1',
            ],
        ),
        api.post_process(
            post_process.StepCommandDoesNotContain,
            'push refs 1-2 of 2',
            ['+refs/heads/1.85.x:refs/heads/1.85.x'],
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    yield api.test(
        'up to date',
        setup(
            mirror_branches,
            mirror_tags,
            refs(
                ('a' * 40, 'refs/heads/1.85.x'),
                ('c' * 40, MASTER),
                ('e' * 40, 'refs/tags/v1.85.1'),
            ),
        ),
        api.post_process(post_process.DoesNotRunRE, r'(seed|push) .*'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    yield api.test(
        'batches many refs',
        setup(
            refs(('c' * 40, MASTER)),
            refs(*(('e' * 40, f'refs/tags/v1.0.{i}') for i in range(250))),
            refs(('c' * 40, MASTER)),
        ),
        api.post_process(post_process.MustRun, 'push refs 1-200 of 250'),
        api.post_process(post_process.MustRun, 'push refs 201-250 of 250'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
