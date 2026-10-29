# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Rebase and lift a `cr<N>` branch, or bump Brave's release channels to a
Chromium tag, with Claude Code, following `tools/cr/autolift/SKILL.md`."""

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
    json,
    path,
    raw_io,
    step,
)
from recipe_test_api import RecipeTestApi

BRAVE_CORE_URL = 'https://github.com/brave/brave-core.git'

AUTOLIFT = 'tools/cr/autolift/autolift.py'
# Where a channel bump's `prepare` and `autofix` copy the tooling, which the
# channels' branches don't carry: what a resumed channel run runs.
STAGED_AUTOLIFT = '.autolift/tooling/cr/autolift/autolift.py'

# The run's report, in its `.autolift/cr<N>/` (or `.autolift/cr<N>.x/`, for
# a channel bump of M<N>) directory, as SKILL names it.
REPORT = 'report.md'

_CR_BRANCH_RE = re.compile(r'refs/heads/cr(\d+)')

# What a step's simulation reports when a test doesn't say.
_LIFT_ADVICE = {'action': 'lift', 'prompt': 'Lift.', 'reasons': []}
_NOTHING_PENDING = {'pending': 0, 'prompt': None, 'report': 'sync-tooling.md'}


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    claude_cli: claude_cli.API
    file: file.API
    git: git.API
    json: json.API
    path: path.API
    raw_io: raw_io.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    chromium_checkout: chromium_checkout.TEST_API
    claude_cli: claude_cli.TEST_API
    json: json.TEST_API
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


def _advise(
    api: DEPS, script: str, target: str, properties: InputProperties
) -> dict:
    """What the run does, and what its agent is told, as `autolift.py advise`
    decides it: no prompt for a run with nothing to do."""
    cmd = ['vpython3', script, 'advise', target]
    if properties.autofix:
        cmd.append('--autofix')
    if properties.resume:
        cmd.append('--resume')
    result = api.step(
        'advise',
        [*cmd, '--json-output', api.json.output()],
        step_test_data=lambda: api.json.test_api.output(_LIFT_ADVICE),
    )
    if not result.json.output:
        raise RuntimeError('autolift.py advise gave no advice')
    return result.json.output


def _sync_tooling(
    api: DEPS,
    milestone: int,
    run_dir,
    add_dirs,
    allowed_tools,
) -> None:
    """Cherry-picks the newer cr branches' tooling onto cr<milestone>, with an
    agent reconciling what the script leaves for judgement, before the lift
    reads its skill. Runs in the checkout, from the brave-core layout."""
    cmd = [
        'python3',
        api.resource('sync_tooling.py'),
        '--milestone',
        str(milestone),
        '--json-output',
        api.json.output(),
    ]
    result = api.step(
        'sync tooling',
        cmd,
        check=False,
        step_test_data=lambda: api.json.test_api.output(_NOTHING_PENDING),
    )
    if result.retcode:
        raise RuntimeError(f'sync_tooling.py failed ({result.retcode})')
    if prompt := result.json.output['prompt']:
        api.claude_cli.run(
            'sync tooling (judgement)',
            prompt,
            log_dir=run_dir,
            add_dirs=add_dirs,
            allowed_tools=allowed_tools,
        )


def RunSteps(api: DEPS, properties: InputProperties) -> None:
    tag = properties.chromium_version
    # A new run syncs its tooling with the newest branch, which a resumed run
    # has done already. A channel bump's tooling comes from the newest branch
    # as it is.
    newest = None
    if not properties.resume or not (properties.milestone or tag):
        newest = _latest_milestone(api)
    milestone = properties.milestone or newest
    # A channel bump's run is named after its milestone, as its branches.
    run_name = f'cr{tag.split(".")[0]}.x' if tag else f'cr{milestone}'
    # A resumed channel bump is on a channel's branch, without the tooling:
    # it runs the copy its `prepare` or `autofix` made.
    script = STAGED_AUTOLIFT if tag and properties.resume else AUTOLIFT
    if properties.resume:
        # As the stopped run left it: a checkout would reset the branch, and
        # clearing would lose the run's report and build.
        root = api.path.abs(api.path.brave_core)
    else:
        api.brave_core_checkout.set_brave_core_ref(f'refs/heads/cr{milestone}')
        # The `.env` for the builds, with RBE when the module's
        # `use_remoteexec` property asks for it.
        api.brave_core_checkout.set_config('brave')
        # A lift builds Android too: every sync checks its deps out.
        api.brave_core_checkout.apply_config('linux')
        api.brave_core_checkout.apply_config('android')
        root = api.brave_core_checkout.ensure_checkout()
        # The agent commits with `git cr commit`. A resumed run keeps the
        # checkout, hook included.
        api.brave_core_checkout.install_commit_hook()
    run_dir = root / '.autolift' / run_name
    # `b/`, which holds the whole checkout, upstream included.
    checkout = root.parent.parent
    # The agent edits anything in the checkout, upstream files included,
    # without auto mode reviewing each edit. `publish`, the run's one push, is
    # allowed too: auto mode otherwise judges it by what the run printed before
    # it, and a rebased branch "behind" its remote reads as overwriting it.
    # The agent runs it as `publish ... > <log> 2>&1; echo "exit $?"`.
    add_dirs = [checkout]
    allowed_tools = [
        f'Edit(/{checkout}/**)',
        f'Write(/{checkout}/**)',
        f'Bash(vpython3 {AUTOLIFT} publish *)',
        f'Bash(vpython3 {STAGED_AUTOLIFT} publish *)',
        'Bash(echo *)',
    ]
    with api.brave_core_checkout.brave_core_layout():
        advice = _advise(api, script, tag or str(milestone), properties)
        # The advice step says why there is nothing to do; an earlier run's
        # report and build are left as they are.
        if not advice['prompt']:
            return
        if not properties.resume:
            # The run's logs and report start empty, so that the agent only
            # reads its own run's. Those of earlier runs are kept with the job
            # that ran them.
            api.file.rmtree('clear earlier run', run_dir)
            # Each run builds from scratch, so that nothing an earlier run
            # built hides a failure of this one.
            api.file.rmtree('clear out dir', root.parent / 'out')
        try:
            # Only a new lift or rebase of an older branch reads the tooling
            # enough to be worth syncing it with the newer ones.
            if (
                not properties.resume
                and not tag
                and advice['action'] in ('lift', 'rebase')
                and newest > milestone
            ):
                _sync_tooling(
                    api,
                    milestone,
                    run_dir,
                    add_dirs,
                    allowed_tools,
                )
            api.claude_cli.run(
                'autolift',
                advice['prompt'],
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

    def new_run(*data):
        """A run on a checkout an earlier job left, of the newest branch
        unless the properties say otherwise."""
        return sum(
            data,
            api.brave_core_checkout.with_git_cache()
            + api.brave_core_checkout.git_cache_populated()
            + api.chromium_checkout.existing_checkout()
            + api.step_data(
                'find latest cr branch',
                stdout=api.raw_io.output_text(_ls_remote),
            ),
        )

    def advice(action: str, prompt: str | None):
        return api.step_data(
            'advise',
            api.json.output(
                {'action': action, 'prompt': prompt, 'reasons': ['- Why.']}
            ),
        )

    def advised(*args: str):
        return api.post_process(
            post_process.StepCommandContains, 'advise', list(args)
        )

    # The newest cr<N> branch is found, checked out, synced, advised on, and
    # lifted as the advice says.
    yield api.test(
        'latest',
        new_run(
            advice('lift', 'Lift cr159.'),
            api.post_process(post_process.MustRun, 'pnpm run sync'),
            api.post_process(
                post_process.StepCommandContains,
                'install commit-msg hook',
                ['git', 'cr', 'install-hook'],
            ),
            advised(AUTOLIFT, 'advise', '159', '--json-output'),
            api.post_process(post_process.MustRun, 'clear out dir'),
            api.claude_cli.prompt_matches('autolift', r'Lift cr159\.'),
            # The newest branch has nothing newer to sync with.
            api.post_process(post_process.DoesNotRun, 'sync tooling'),
            api.post_process(post_process.StatusSuccess),
        ),
    )

    # A run with nothing to do starts no agent, and leaves the earlier run's
    # report and build alone.
    yield api.test(
        'nothing to do',
        new_run(
            advice('none', None),
            api.post_process(post_process.DoesNotRun, 'clear earlier run'),
            api.post_process(post_process.DoesNotRun, 'clear out dir'),
            api.post_process(post_process.DoesNotRun, 'sync tooling'),
            api.post_process(post_process.DoesNotRun, 'autolift'),
            api.post_process(post_process.DoesNotRun, 'report'),
            api.post_process(post_process.StatusSuccess),
        ),
    )

    # Advice that can't be told fails the run before any agent starts.
    yield api.test(
        'advise fails',
        new_run(
            api.step_data('advise', retcode=1),
            api.post_process(post_process.DoesNotRun, 'autolift'),
            api.post_process(post_process.StatusFailure),
            api.post_process(post_process.DropExpectation),
        ),
        status='FAILURE',
    )

    # And so does advice that comes back empty.
    yield api.test(
        'no advice',
        new_run(
            api.step_data('advise', api.json.invalid('')),
            api.post_process(post_process.DoesNotRun, 'autolift'),
            api.post_process(post_process.StatusException),
            api.post_process(post_process.DropExpectation),
        ),
        status='EXCEPTION',
    )

    # An explicit milestone is checked out as it is.
    yield api.test(
        'explicit milestone',
        new_run(
            api.properties(milestone=158),
            api.post_process(
                post_process.StepCommandContains,
                'brave-core checkout branch',
                ['checkout', '--force', '-B', 'cr158'],
            ),
            advised('advise', '158'),
            api.post_process(post_process.StatusSuccess),
            api.post_process(post_process.DropExpectation),
        ),
    )

    # An older branch that will lift or rebase first syncs its tooling with
    # the newer ones.
    yield api.test(
        'sync tooling',
        new_run(
            api.properties(milestone=158),
            advice('rebase', 'Rebase cr158.'),
            api.post_process(
                post_process.StepCommandContains,
                'sync tooling',
                ['--milestone', '158', '--json-output'],
            ),
            api.post_process(
                post_process.DoesNotRun, 'sync tooling (judgement)'
            ),
            api.post_process(post_process.MustRun, 'autolift'),
            api.post_process(post_process.StatusSuccess),
        ),
    )

    # A run that only fixes the CI leaves the tooling as it is.
    yield api.test(
        'sync tooling not worth it',
        new_run(
            api.properties(milestone=158),
            advice('autofix', 'Fix the CI of cr158.'),
            api.post_process(post_process.DoesNotRun, 'sync tooling'),
            api.claude_cli.prompt_matches('autolift', r'Fix the CI of cr158\.'),
            api.post_process(post_process.StatusSuccess),
            api.post_process(post_process.DropExpectation),
        ),
    )

    # Commits the script left for judgement are reconciled by an agent first,
    # as the script tells it.
    yield api.test(
        'sync tooling pending',
        new_run(
            api.properties(milestone=158),
            api.step_data(
                'sync tooling',
                api.json.output(
                    {
                        'pending': 2,
                        'prompt': 'Reconcile cr158.',
                        'report': 'sync-tooling.md',
                    }
                ),
            ),
            api.claude_cli.prompt_matches(
                'sync tooling (judgement)', r'Reconcile cr158\.'
            ),
            api.post_process(post_process.MustRun, 'autolift'),
            api.post_process(post_process.StatusSuccess),
            api.post_process(post_process.DropExpectation),
        ),
    )

    # A script that fails stops the run before the lift.
    yield api.test(
        'sync tooling fails',
        new_run(
            api.properties(milestone=158),
            api.step_data('sync tooling', retcode=1),
            api.post_process(post_process.DoesNotRun, 'autolift'),
            api.post_process(post_process.StatusException),
            api.post_process(post_process.DropExpectation),
        ),
        status='EXCEPTION',
    )

    # Builds use RBE through the module's properties, as on brave_builder.
    yield api.test(
        'rbe',
        new_run(
            api.brave_core_checkout.rbe(siso_cache_dir='/b/siso'),
            api.properties(milestone=159),
            api.post_process(post_process.MustRun, 'write .env'),
            api.post_process(post_process.MustRun, 'autolift'),
            api.post_process(post_process.StatusSuccess),
        ),
    )

    # A stopped run is carried on as it was left: nothing is checked out,
    # synced or cleared.
    yield api.test(
        'resume',
        api.brave_core_checkout.with_git_cache(),
        api.properties(milestone=159, resume=True),
        advice('resume', 'Resume cr159.'),
        api.post_process(post_process.DoesNotRun, 'find latest cr branch'),
        api.post_process(post_process.DoesNotRunRE, r'brave-core .*'),
        api.post_process(post_process.DoesNotRunRE, r'git cache .*'),
        api.post_process(post_process.DoesNotRun, 'write .env'),
        api.post_process(post_process.DoesNotRun, 'pnpm run sync'),
        api.post_process(post_process.DoesNotRun, 'clear earlier run'),
        api.post_process(post_process.DoesNotRun, 'clear out dir'),
        api.post_process(post_process.DoesNotRun, 'install commit-msg hook'),
        advised(AUTOLIFT, 'advise', '159', '--resume'),
        api.claude_cli.prompt_matches('autolift', r'Resume cr159\.'),
        api.post_process(post_process.StatusSuccess),
    )

    # An autofix run is advised as one.
    yield api.test(
        'autofix',
        new_run(
            api.properties(milestone=158, autofix=True),
            advice('autofix', 'Fix the CI of cr158.'),
            advised('advise', '158', '--autofix'),
            api.post_process(post_process.DoesNotRun, 'sync tooling'),
            api.claude_cli.prompt_matches('autolift', r'Fix the CI of cr158\.'),
            api.post_process(post_process.StatusSuccess),
        ),
    )

    # A stopped autofix run is resumed as an autofix run.
    yield api.test(
        'resume autofix',
        api.brave_core_checkout.with_git_cache(),
        api.properties(milestone=159, resume=True, autofix=True),
        advised('advise', '159', '--autofix', '--resume'),
        api.post_process(post_process.DoesNotRun, 'clear earlier run'),
        api.post_process(post_process.DoesNotRun, 'pnpm run sync'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    # A channel bump runs from the newest branch's tooling, as it is, with
    # its own run directory.
    yield api.test(
        'channel bump',
        new_run(
            api.properties(chromium_version='157.0.8103.38'),
            api.post_process(
                post_process.StepCommandContains,
                'brave-core checkout branch',
                ['checkout', '--force', '-B', 'cr159'],
            ),
            advised(AUTOLIFT, 'advise', '157.0.8103.38'),
            api.post_process(post_process.DoesNotRun, 'sync tooling'),
            api.post_process(
                post_process.StepCommandRE,
                'report',
                ['cat', r'.*/b/src/brave/\.autolift/cr157\.x/report\.md'],
            ),
            api.post_process(post_process.StatusSuccess),
        ),
    )

    # An older branch's tooling is used as it is too.
    yield api.test(
        'channel bump from an older branch',
        new_run(
            api.properties(milestone=158, chromium_version='157.0.8103.38'),
            api.post_process(post_process.DoesNotRun, 'sync tooling'),
            api.post_process(post_process.StatusSuccess),
            api.post_process(post_process.DropExpectation),
        ),
    )

    # A channel bump's autofix run is advised as one.
    yield api.test(
        'channel bump autofix',
        new_run(
            api.properties(chromium_version='157.0.8103.38', autofix=True),
            advised('advise', '157.0.8103.38', '--autofix'),
            api.post_process(post_process.StatusSuccess),
            api.post_process(post_process.DropExpectation),
        ),
    )

    # A stopped channel bump is on a channel's branch, which lacks the
    # tooling: it runs the copy the run made.
    yield api.test(
        'resume channel bump',
        api.brave_core_checkout.with_git_cache(),
        api.properties(chromium_version='157.0.8103.38', resume=True),
        api.post_process(post_process.DoesNotRun, 'find latest cr branch'),
        api.post_process(post_process.DoesNotRun, 'clear earlier run'),
        advised(STAGED_AUTOLIFT, 'advise', '157.0.8103.38', '--resume'),
        api.post_process(
            post_process.StepCommandRE,
            'report',
            ['cat', r'.*/b/src/brave/\.autolift/cr157\.x/report\.md'],
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
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
        api.post_process(post_process.DoesNotRun, 'advise'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
