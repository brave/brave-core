# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Upload the `cr<N>` branch autolift ran on to Gerrit, as a relation chain.

Runs after `cr/autolift` and then `gerrit/mirror_brave_core`, in the workspace
the lift ran in, on its brave-core checkout, which is on `cr<N>`. Brockit finds
where the branch's own commits start, and `gerrit_upload.py` pushes GitHub's
`cr<N>` to `refs/for/master` with topic `cr<N>`, and abandons the topic's CLs
whose commits left the branch.
"""

from __future__ import annotations

from dataclasses import dataclass
import re

from google.protobuf import json_format
import post_process
from PB.recipe_engine import result as result_pb
from PB.recipes.brave.cr.autolift import Outcome, ReviewCommit
from PB.recipes.brave.cr.autolift_gerrit_upload import InputProperties, Review
from PB.recipes.brave.cr.commit import Commit
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    brave_gerrit,
    chromium_checkout,
    depot_tools,
    git,
    json,
    raw_io,
    step,
)
from recipe_test_api import RecipeTestApi

_CR_BRANCH = re.compile(r'cr\d+')
_BASE_COMMIT = re.compile(r'base commit: ([0-9a-f]{40})')
_GERRIT_CCS = re.compile(r'gerrit ccs: (\S+)')
# Under the 4000 bytes a summary may take, with room for the last line.
_SUMMARY_BYTES = 3800


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    brave_gerrit: brave_gerrit.API
    depot_tools: depot_tools.API
    git: git.API
    json: json.API
    raw_io: raw_io.API
    step: step.API


PROPERTIES = InputProperties


def _summary(review: Review) -> str:
    """The new CLs, then the other CLs to review, as links, as many as the
    summary takes."""
    created = {change.commit.sha for change in review.created}
    changed = [c for c in review.changes if c.commit.sha not in created]
    lines: list[str] = []
    entries = []
    for title, changes in (('New CLs', review.created), ('To review', changed)):
        if changes:
            entries.append((f'{title} ({len(changes)}):', None))
            entries += [
                (f'- [{change.commit.subject}]({change.url})', change)
                for change in changes
            ]
    if not entries:
        return 'Nothing to review.'
    left = sum(1 for _, change in entries if change)
    for line, change in entries:
        if len('\n'.join([*lines, line]).encode()) > _SUMMARY_BYTES:
            lines.append(
                f'- … and {left} more, in the `review` output property.'
            )
            break
        lines.append(line)
        left -= bool(change)
    return '\n'.join(lines)


def RunSteps(
    api: DEPS, properties: InputProperties
) -> result_pb.RawResult | None:
    with api.brave_core_checkout.brave_core_layout():
        branch = api.git(
            'rev-parse',
            '--abbrev-ref',
            'HEAD',
            name='find branch',
            stdout=api.raw_io.output_text(),
        ).stdout.strip()
        if not _CR_BRANCH.fullmatch(branch):
            raise RuntimeError(
                f'the checkout is on {branch}, not on a cr<N> branch'
            )
        # Where `brockit rebase` would start replaying the branch: before its
        # last major bump, as its base is always on the previous milestone.
        shown = api.step(
            'find branch start',
            ['brockit', 'show', '--from-ref-hash=@previous-major'],
            stdout=api.raw_io.output_text(),
        ).stdout
        match = _BASE_COMMIT.search(shown)
        if not match:
            raise RuntimeError(f'brockit found no start for {branch}')
        # The staff looking after the milestone, its pull request's assignees.
        shown = api.step(
            'find ccs',
            ['brockit', 'show', f'--gerrit-ccs={branch.removeprefix("cr")}'],
            stdout=api.raw_io.output_text(),
            step_test_data=lambda: api.raw_io.test_api.stream_output_text(
                'gerrit ccs: cdesouza,erogul,alexey\n'
            ),
        ).stdout
        ccs_match = _GERRIT_CCS.search(shown)
        ccs = ccs_match[1].split(',') if ccs_match else []
        cmd = [
            api.depot_tools.vpython3(),
            api.resource('gerrit_upload.py'),
            '--url',
            api.brave_gerrit.ssh_url(properties.gerrit_user),
            '--branch',
            branch,
            '--start',
            match[1],
            '--head',
            f'origin/{branch}',
        ]
        if properties.dry_run:
            cmd.append('--dry-run')
        for cc in ccs:
            cmd += ['--cc', cc]
        # The commits the run pushed to this branch, which is all the upload
        # takes; a channel bump's branches don't go to Gerrit.
        commits = [
            json_format.MessageToDict(
                review.commit, preserving_proto_field_name=True
            )
            for review in properties.outcome.review
            if review.branch == branch
        ]
        if commits:
            cmd += ['--review', api.json.input(commits)]
        result = api.step(
            'upload',
            [*cmd, '--json-output', api.json.output()],
            step_test_data=lambda: api.json.test_api.output(
                {'review': [], 'created': []}
            ),
        )
        review = Review()
        output = result.json.output or {}
        for link in output.get('review', []):
            json_format.ParseDict(link, review.changes.add())
        for link in output.get('created', []):
            json_format.ParseDict(link, review.created.add())
        result.presentation.properties['review'] = review
        return result_pb.RawResult(
            status=result_pb.SUCCESS, summary_markdown=_summary(review)
        )


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    chromium_checkout: chromium_checkout.TEST_API
    json: json.TEST_API
    raw_io: raw_io.TEST_API


def GenTests(api: TEST_DEPS):
    start = 'f' * 40

    def checkout(branch: str = 'cr160', shown: str = f'base commit: {start}\n'):
        return (
            api.properties(gerrit_user='autolift-bot')
            # The workspace the lift ran in.
            + api.brave_core_checkout.with_git_cache()
            + api.chromium_checkout.existing_checkout()
            + api.step_data(
                'find branch', stdout=api.raw_io.output_text(f'{branch}\n')
            )
            + api.step_data(
                'find branch start', stdout=api.raw_io.output_text(shown)
            )
        )

    yield api.test(
        'uploads',
        checkout(),
        api.post_process(
            post_process.StepCommandContains,
            'upload',
            [
                '--url',
                'ssh://autolift-bot@gerrit-ssh.brave.com:29418/'
                'brave-browser/brave-core',
                '--branch',
                'cr160',
                '--start',
                start,
                '--head',
                'origin/cr160',
            ],
        ),
        api.post_process(
            post_process.StepCommandDoesNotContain, 'upload', ['--dry-run']
        ),
        # The milestone's staff are CC'd.
        api.post_process(
            post_process.StepCommandContains,
            'find ccs',
            ['brockit', 'show', '--gerrit-ccs=160'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'upload',
            ['--cc', 'cdesouza', '--cc', 'erogul', '--cc', 'alexey'],
        ),
        api.post_process(post_process.StatusSuccess),
    )

    # An older Brockit that prints no CCs leaves the CLs without them.
    yield api.test(
        'no ccs',
        checkout(),
        api.step_data('find ccs', stdout=api.raw_io.output_text('')),
        api.post_process(
            post_process.StepCommandDoesNotContain, 'upload', ['--cc']
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    cl = 'https://gerrit.brave.com/c/brave-browser/brave-core/+/'

    def outcome(*branches: str) -> Outcome:
        return Outcome(
            complete=True,
            summary='Lifted.',
            review=[
                ReviewCommit(
                    branch=branch,
                    commit=Commit(
                        sha=f'{index}' * 40, subject=f'[{branch}] Fix {index}'
                    ),
                )
                for index, branch in enumerate(branches, 1)
            ],
        )

    def change(n: int, patch_set: int, subject: str = '') -> dict:
        return {
            'commit': {
                'sha': f'{n}' * 40,
                'subject': subject or f'[cr160] Fix {n}',
            },
            'change': 40 + n,
            'patch_set': patch_set,
            'url': f'{cl}{40 + n}'
            + (f'/{patch_set - 1}..{patch_set}' if patch_set > 1 else ''),
        }

    # The autolift run's commits to review on this branch get their CLs'
    # links, and so do the CLs the push created, a teammate's included: in
    # the summary, new CLs first, and the `review` output property.
    yield api.test(
        'review links',
        checkout(),
        api.properties(outcome=outcome('cr160', 'cr157.x', 'cr160')),
        api.step_data(
            'upload',
            api.json.output(
                {
                    'review': [change(1, 3), change(3, 1)],
                    'created': [
                        change(3, 1),
                        change(9, 1, '[cr160] A teammate fix'),
                    ],
                }
            ),
        ),
        api.post_process(
            post_process.StepCommandContains, 'upload', ['--review']
        ),
        api.post_process(
            post_process.PropertyMatchesCallable,
            'review',
            lambda check, value: check(
                [c['change'] for c in value['changes']] == [41, 43]
                and [c['change'] for c in value['created']] == [43, 49]
            ),
        ),
        api.post_process(
            post_process.SummaryMarkdown,
            f'New CLs (2):\n- [[cr160] Fix 3]({cl}43)\n'
            f'- [[cr160] A teammate fix]({cl}49)\n'
            f'To review (1):\n- [[cr160] Fix 1]({cl}41/2..3)',
        ),
        api.post_process(post_process.StatusSuccess),
    )

    # Nothing of this branch's for the run to review: its new CLs alone.
    yield api.test(
        'only new cls',
        checkout(),
        api.properties(outcome=outcome('cr157.x')),
        api.step_data(
            'upload', api.json.output({'review': [], 'created': [change(2, 1)]})
        ),
        api.post_process(
            post_process.StepCommandDoesNotContain, 'upload', ['--review']
        ),
        api.post_process(
            post_process.SummaryMarkdown,
            f'New CLs (1):\n- [[cr160] Fix 2]({cl}42)',
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    # A long list is cut short of the summary's limit.
    yield api.test(
        'many to review',
        checkout(),
        api.properties(outcome=outcome(*['cr160'] * 9)),
        api.step_data(
            'upload',
            api.json.output(
                {
                    'review': [
                        {
                            'commit': {
                                'sha': f'{n:040x}',
                                'subject': 'A long subject ' * 4,
                            },
                            'change': n,
                            'patch_set': 2,
                            'url': f'{cl}{n}/1..2',
                        }
                        for n in range(100)
                    ],
                    'created': [change(1, 1)],
                }
            ),
        ),
        api.post_process(
            post_process.SummaryMarkdownRE,
            r'(?s)^New CLs \(1\):\n.*\nTo review \(100\):\n.*\n- … and \d+ '
            r'more, in the `review` output property\.$',
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    # Nothing it found to review says so.
    yield api.test(
        'review finds no CLs',
        checkout(),
        api.properties(outcome=outcome('cr160')),
        api.post_process(post_process.SummaryMarkdown, 'Nothing to review.'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    yield api.test(
        'dry run',
        checkout(),
        api.properties(dry_run=True),
        api.post_process(
            post_process.StepCommandContains, 'upload', ['--dry-run']
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    yield api.test(
        'refused upload fails',
        checkout(),
        api.step_data('upload', retcode=1),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )

    yield api.test(
        'not on a cr branch',
        checkout(branch='master'),
        api.post_process(post_process.DoesNotRun, 'find branch start'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )

    yield api.test(
        'no branch start',
        checkout(shown='upstream version: 160.0.8158.0\n'),
        api.post_process(post_process.DoesNotRun, 'upload'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
