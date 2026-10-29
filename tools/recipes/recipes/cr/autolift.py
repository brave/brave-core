# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Rebase and lift a `cr<N>` branch with Claude Code, following
`tools/cr/autolift/SKILL.md`."""

from __future__ import annotations

from dataclasses import dataclass
import re

import post_process
from PB.recipes.brave.cr.autolift import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    chromium_checkout,
    claude_cli,
    file,
    git,
    path,
    raw_io,
    step,
)
from recipe_test_api import RecipeTestApi

BRAVE_CORE_URL = 'https://github.com/brave/brave-core.git'

SKILL = 'tools/cr/autolift/SKILL.md'

# The run's report, in its `.autolift/cr<N>/` directory, as SKILL names it.
REPORT = 'report.md'

_CR_BRANCH_RE = re.compile(r'refs/heads/cr(\d+)')

_PROMPT = """\
Lift cr{milestone} by following {skill} from step 1 to the end. Read the whole \
file before starting. The milestone N is {milestone}.

This is an unattended CI run: there is no human to ask, so make the calls the \
document describes and record them in the report."""

_SYNC_PROMPT = """\
Bring cr{milestone}'s lift tooling up to date with the newer cr branches. \
{report} lists the commits `sync_tooling.py` cherry-picked, and those it left \
for judgement: follow its "What to do" section until the script exits 0.

This is an unattended CI run: there is no human to ask, so make the calls the \
section describes, and list each pending commit you dropped, and why, at the \
end of {report}."""

_SYNC_PENDING = 2

_RESUME_PROMPT = """\
Resume the lift of cr{milestone}, which an earlier run of {skill} stopped \
before it finished. Read the whole file, then follow its "Resuming a run" \
section to carry on where that run stopped. The milestone N is {milestone}.

This is an unattended CI run: there is no human to ask, so make the calls the \
document describes and record them in the report."""


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    claude_cli: claude_cli.API
    file: file.API
    git: git.API
    path: path.API
    raw_io: raw_io.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    chromium_checkout: chromium_checkout.TEST_API
    claude_cli: claude_cli.TEST_API
    raw_io: raw_io.TEST_API


PROPERTIES = InputProperties


def _latest_milestone(api: DEPS) -> int:
    """The milestone of the newest `cr<N>` branch on brave-core."""
    result = api.git(
        'ls-remote',
        '--heads',
        BRAVE_CORE_URL,
        'refs/heads/cr*',
        name='find latest cr branch',
        stdout=api.raw_io.output_text(),
    )
    milestones = []
    for line in result.stdout.splitlines():
        _, _, ref = line.partition('\t')
        if match := _CR_BRANCH_RE.fullmatch(ref):
            milestones.append(int(match[1]))
    if not milestones:
        raise RuntimeError(f'no cr<N> branch on {BRAVE_CORE_URL}')
    return max(milestones)


def _sync_tooling(
    api: DEPS,
    milestone: int,
    root,
    run_dir,
    add_dirs,
    allowed_tools,
) -> None:
    """Cherry-picks the newer cr branches' tooling onto cr<milestone>, with an
    agent reconciling what the script leaves for judgement, before the lift
    reads its skill."""
    cmd = [
        'python3',
        api.resource('sync_tooling.py'),
        '--repo',
        root,
        '--milestone',
        str(milestone),
    ]
    result = api.step('sync tooling', cmd, check=False)
    report = run_dir / 'sync-tooling.md'
    if result.retcode == _SYNC_PENDING:
        api.claude_cli.run(
            'sync tooling (judgement)',
            _SYNC_PROMPT.format(milestone=milestone, report=report),
            log_dir=run_dir,
            add_dirs=add_dirs,
            allowed_tools=allowed_tools,
        )
    elif result.retcode:
        raise RuntimeError(f'sync_tooling.py failed ({result.retcode})')


def RunSteps(api: DEPS, properties: InputProperties) -> None:
    # A new run syncs its tooling with the newest branch, which a resumed run
    # has done already.
    newest = None
    if not properties.resume or not properties.milestone:
        newest = _latest_milestone(api)
    milestone = properties.milestone or newest
    if properties.resume:
        # As the stopped run left it: a checkout would reset the branch, and
        # clearing would lose the run's report and build.
        root = api.path.abs(api.path.brave_core)
        run_dir = root / '.autolift' / f'cr{milestone}'
        prompt = _RESUME_PROMPT
    else:
        api.brave_core_checkout.set_brave_core_ref(f'refs/heads/cr{milestone}')
        # The `.env` for the builds, with RBE when the module's
        # `use_remoteexec` property asks for it.
        api.brave_core_checkout.set_config('brave')
        root = api.brave_core_checkout.ensure_checkout()
        # The agent commits with `git cr commit`. A resumed run keeps the
        # checkout, hook included.
        api.brave_core_checkout.install_commit_hook()
        # The run's logs and report start empty, so that the agent only reads
        # its own run's. Those of earlier runs are kept with the job that ran
        # them.
        run_dir = root / '.autolift' / f'cr{milestone}'
        api.file.rmtree('clear earlier run', run_dir)
        # Each run builds from scratch, so that nothing an earlier run built
        # hides a failure of this one.
        api.file.rmtree('clear out dir', root.parent / 'out')
        prompt = _PROMPT
    # `b/`, which holds the whole checkout, upstream included.
    checkout = root.parent.parent
    # The agent edits anything in the checkout, upstream files included,
    # without auto mode reviewing each edit.
    add_dirs = [checkout]
    allowed_tools = [f'Edit(/{checkout}/**)', f'Write(/{checkout}/**)']
    with api.brave_core_checkout.brave_core_layout():
        try:
            if not properties.resume and newest > milestone:
                _sync_tooling(
                    api,
                    milestone,
                    root,
                    run_dir,
                    add_dirs,
                    allowed_tools,
                )
            api.claude_cli.run(
                'autolift',
                prompt.format(milestone=milestone, skill=SKILL),
                log_dir=run_dir,
                add_dirs=add_dirs,
                allowed_tools=allowed_tools,
            )
        finally:
            # The report in full, however the run ended. A run that stopped
            # before writing one has none.
            api.step('report', ['cat', run_dir / REPORT], check=False)


def GenTests(api: TEST_DEPS):
    _ls_remote = '\n'.join(
        [
            'aaaa\trefs/heads/cr158',
            'bbbb\trefs/heads/cr159',
            'cccc\trefs/heads/cr159_fix',
            'dddd\trefs/heads/cr99',
        ]
    )

    # The newest cr<N> branch is found, checked out, and lifted.
    yield api.test(
        'latest',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.step_data(
            'find latest cr branch',
            stdout=api.raw_io.output_text(_ls_remote),
        ),
        api.post_process(
            post_process.StepCommandContains,
            'install commit-msg hook',
            ['git', 'cr', 'install-hook'],
        ),
        api.claude_cli.prompt_matches('autolift', rf'Lift cr159\b.*{SKILL}.*'),
        # The newest branch has nothing newer to sync with.
        api.post_process(post_process.DoesNotRun, 'sync tooling'),
        api.post_process(post_process.StatusSuccess),
    )

    # An explicit milestone is checked out as it is.
    yield api.test(
        'explicit milestone',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.properties(milestone=158),
        api.step_data(
            'find latest cr branch',
            stdout=api.raw_io.output_text(_ls_remote),
        ),
        api.post_process(
            post_process.StepCommandContains,
            'brave-core checkout branch',
            ['checkout', '--force', '-B', 'cr158'],
        ),
        api.claude_cli.prompt_matches('autolift', r'Lift cr158\b.*'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    # An older branch first syncs its tooling with the newer ones.
    yield api.test(
        'sync tooling',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.properties(milestone=158),
        api.step_data(
            'find latest cr branch',
            stdout=api.raw_io.output_text(_ls_remote),
        ),
        api.post_process(
            post_process.StepCommandContains,
            'sync tooling',
            ['--milestone', '158'],
        ),
        api.post_process(post_process.DoesNotRun, 'sync tooling (judgement)'),
        api.post_process(post_process.MustRun, 'autolift'),
        api.post_process(post_process.StatusSuccess),
    )

    # Commits the script left for judgement are reconciled by an agent first.
    yield api.test(
        'sync tooling pending',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.properties(milestone=158),
        api.step_data(
            'find latest cr branch',
            stdout=api.raw_io.output_text(_ls_remote),
        ),
        api.step_data('sync tooling', retcode=_SYNC_PENDING),
        api.claude_cli.prompt_matches(
            'sync tooling (judgement)', r'.*cr158.*sync-tooling\.md.*'
        ),
        api.post_process(post_process.MustRun, 'autolift'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    # A script that fails stops the run before the lift.
    yield api.test(
        'sync tooling fails',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.properties(milestone=158),
        api.step_data(
            'find latest cr branch',
            stdout=api.raw_io.output_text(_ls_remote),
        ),
        api.step_data('sync tooling', retcode=1),
        api.post_process(post_process.DoesNotRun, 'autolift'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )

    # Builds use RBE through the module's properties, as on brave_builder.
    yield api.test(
        'rbe',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.brave_core_checkout.rbe(siso_cache_dir='/b/siso'),
        api.properties(milestone=159),
        api.step_data(
            'find latest cr branch',
            stdout=api.raw_io.output_text(_ls_remote),
        ),
        api.post_process(post_process.MustRun, 'write .env'),
        api.post_process(post_process.MustRun, 'autolift'),
        api.post_process(post_process.StatusSuccess),
    )

    # A stopped run is carried on as it was left: nothing is checked out or
    # cleared.
    yield api.test(
        'resume',
        api.brave_core_checkout.with_git_cache(),
        api.properties(milestone=159, resume=True),
        api.post_process(post_process.DoesNotRun, 'find latest cr branch'),
        api.post_process(post_process.DoesNotRunRE, r'brave-core .*'),
        api.post_process(post_process.DoesNotRunRE, r'git cache .*'),
        api.post_process(post_process.DoesNotRun, 'write .env'),
        api.post_process(post_process.DoesNotRun, 'clear earlier run'),
        api.post_process(post_process.DoesNotRun, 'clear out dir'),
        api.post_process(post_process.DoesNotRun, 'install commit-msg hook'),
        api.claude_cli.prompt_matches(
            'autolift', rf'Resume the lift of cr159\b.*{SKILL}.*'
        ),
        api.post_process(post_process.StatusSuccess),
    )

    # A run that Claude didn't finish, e.g. with background work it stopped,
    # fails the build.
    yield api.test(
        'unfinished run fails',
        api.brave_core_checkout.with_git_cache(),
        api.properties(milestone=159, resume=True),
        api.claude_cli.failed('autolift'),
        api.post_process(post_process.StepFailure, 'autolift'),
        # The report is printed for a failed run too.
        api.post_process(
            post_process.StepCommandRE,
            'report',
            ['cat', r'.*/b/src/brave/\.autolift/cr159/report\.md'],
        ),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )

    # No cr<N> branch to lift.
    yield api.test(
        'no cr branch',
        api.brave_core_checkout.with_git_cache(),
        api.step_data(
            'find latest cr branch',
            stdout=api.raw_io.output_text('eeee\trefs/heads/master'),
        ),
        api.post_process(post_process.DoesNotRun, 'autolift'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
