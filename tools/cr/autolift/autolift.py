#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Helpers for the autonomous Chromium lift in tools/cr/autolift/SKILL.md.

Subcommands:
    prepare N [--no-rebase] [--force]
        Resets the local cr<N> branch to origin/cr<N>, sets its upstream to the
        base (origin/cr<N-1>, or origin/master once master is on milestone N-1),
        picks the lift target, and starts `brockit rebase --squash-minor-bumps`,
        with `--discard-regen-changes` when there will be a lift, or when the
        base got a bump since the branch was last rebased (for `brockit regen`
        to remake them after the rebase). The target is
        `@latest-for-branch`, until origin/cr<N+1> exists; then it is the
        newest beta or stable release of M<N> newer than cr<N>, if there is
        one. For a new milestone with no origin/cr<N> yet, creates cr<N> from
        the base instead, and leaves the rebase out.
    lift [--continue]
        Runs `brockit lift --with-github` (in infra mode) to the target
        `prepare` picked for the current cr<N> branch, keeps the log under
        .autolift/cr<N>/, and prints and appends to the run's report the
        bump range and what needs attention. Exits non-zero if anything does,
        even when Brockit itself exits 0, and when there is no lift to do.
    publish N [--built]
        At the very end of a run that finished with no issues: pushes cr<N>,
        keeping the base as its upstream. It refreshes CI's git cache, when
        there is one, so that `origin` is as recent as GitHub. First, it
        cherry-picks what landed on cr<N> since the run started, fetches the
        latest base (once per run), and runs
        `brockit rebase --squash-minor-bumps` if the base moved or picked
        fixups need squashing. If that changed the
        branch, it exits 2: rebuild, then run `publish --built` to push. A new
        milestone is pushed, and gets its version issue and pull request
        against the base from `brockit update-version-issue`.
    build-summary LOG
        Summarises a `pnpm run build` log: the final status line, the steps
        that failed, and each distinct error with how often it occurred.
    culprits RANGE [--symbol S] [PATH ...]
        Lists the upstream commits in RANGE (e.g. 158.0.8117.1..158.0.8127.2)
        that touch PATH, or that add or remove the string S, with full hashes.
    next-fixes RANGE [PATH ...]
        For a maintenance branch's minor bump: lists the upstream cherry-picks
        in RANGE (touching PATH) whose origin a commit on a newer cr<M> branch
        already cites as its culprit.
    next-tracker RANGE
        For a maintenance branch's minor bump: lists the items of each newer
        branch's tools/cr/M<M>_codehealth.md that cite the origin of an
        upstream cherry-pick in RANGE, or name a file one of them changed,
        with each item's state and, for ticked ones, the newer branch's fix.
    pick-fix COMMIT [CULPRIT]
        Cherry-picks COMMIT, a fix next-fixes or next-tracker found on a newer
        branch, with its message as it is. With CULPRIT, a cherry-pick in this
        branch's range, it first checks that the fix cites its origin.
    verify-culprits BASE RANGE
        Checks that every culprit hash cited in the brave-core commits after
        BASE names a real Chromium commit inside RANGE, or the main commit of
        a cherry-pick inside it.
"""

from __future__ import annotations

import argparse
from collections.abc import Sequence
import os
import dataclasses
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
import re
import subprocess
import sys

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from exceptions import RebaseNeededException  # noqa: E402  -- after _boot
import rebase  # noqa: E402  -- after _boot
import repository  # noqa: E402  -- after _boot
from versioning import (  # noqa: E402  -- after _boot
    Version,
    read_chromium_version_file,
)

_BROCKIT = Path(__file__).resolve().parent.parent / 'brockit.py'

# `brockit rebase --dry-run`'s exit status when the rebase would change the
# branch.
_REBASE_NEEDED = RebaseNeededException.EXIT_CODE

# Logs and the run's report live in the workspace rather than in a temporary
# directory, and outside `out/`, so that a clean rebuild keeps them.
_RUN_ROOT = repository.brave.root / '.autolift'

# Brockit's log lines carry a `[hh:mm:ss]` prefix and a right-aligned
# `file.py:NNN` suffix.
_LOG_TIME = re.compile(r'^\[\d\d:\d\d:\d\d\]\s?')
_LOG_SOURCE = re.compile(r'\s{2,}[\w.]+\.py:\d+\s*$')
_LIFT_RANGE = re.compile(r'\+log/([0-9.]+\.\.[0-9.]+)')

_ANSI_ESCAPE = re.compile(r'\x1b\[[0-9;]*[A-Za-z]')

_STATUS_LINE = re.compile(
    r'Build (?:Failure|Succeeded):.*|The build has finished successfully\.'
)
_FAILED_STEP = re.compile(r'FAILED: \S+ (.*)')
# Leading progress dots, but not the dots of a relative path such as ../../a.
_PROGRESS_DOTS = re.compile(r'^\.+(?=[^./])')

# Each pattern captures the part of an error line worth deduplicating on. Paths
# and line numbers are kept for compile errors so that each failing location is
# reported once.
_ERROR_PATTERNS = (
    # clang: ../../foo/bar.cc:12:3: error: message
    re.compile(r'^(\S+:\d+:\d+: error: .*)$'),
    # javac / turbine: ../../Foo.java:12: error: message
    re.compile(r'^(\S+\.java:\d+: error: .*)$'),
    # gn gen / gn check
    re.compile(r'^(ERROR at .*)$'),
    # TypeScript: file.ts:6:41 - error TS2307: message
    re.compile(r'^(\S+\.ts:\d+:\d+ - error TS\d+: .*)$'),
    # eslint: "  25:65  error  message  rule-name"
    re.compile(r'^\s+\d+:\d+\s+error\s+(.*)$'),
    # linkers
    re.compile(r'^(\S+: error: undefined symbol: .*)$'),
    # d8 / r8
    re.compile(r'^(Type \S+ is defined multiple times: .*)$'),
    # siso scheduling
    re.compile(r'^(.* missing and no known rule to make it)$'),
    # Python tracebacks from build actions
    re.compile(r'^((?:ModuleNotFoundError|ImportError|AssertionError): .*)$'),
)

_CULPRIT_URL = re.compile(
    r'chromium\.googlesource\.com/chromium/src/\+/'
    r'([0-9a-f]{7,40})\b'
)

# The trailer upstream's release branches add to each merge from main.
_CHERRY_PICKED = re.compile(
    r'^\(cherry picked from commit ([0-9a-f]{40})\)$', re.MULTILINE
)


def strip_ansi(text: str) -> str:
    """Removes terminal colour escapes from build output."""
    return _ANSI_ESCAPE.sub('', text)


@dataclass(frozen=True)
class BuildSummary:
    status: str | None
    failed_steps: list[str]
    errors: list[tuple[int, str]]


def summarize_build(log: str) -> BuildSummary:
    """Extracts the outcome, failed steps and distinct errors from a log."""
    status = None
    failed_steps = []
    counts: dict[str, int] = {}
    for line in strip_ansi(log).splitlines():
        # siso prints progress dots before some messages on the same line.
        line = _PROGRESS_DOTS.sub('', line)
        if match := _STATUS_LINE.search(line):
            status = match.group(0)
        if match := _FAILED_STEP.search(line):
            if match.group(1) not in failed_steps:
                failed_steps.append(match.group(1))
            continue
        for pattern in _ERROR_PATTERNS:
            if match := pattern.match(line):
                error = match.group(1).strip()
                counts[error] = counts.get(error, 0) + 1
                break
    errors = sorted(
        ((n, e) for e, n in counts.items()),
        key=lambda item: (-item[0], item[1]),
    )
    return BuildSummary(status, failed_steps, errors)


def parse_culprit_hashes(message: str) -> list[str]:
    """Returns the hashes in the "Chromium changes:" block of a message.

    `git cr commit --culprit` writes that block, one URL per culprit, ending at
    the first blank line. Links elsewhere in the body are not culprits.
    """
    hashes = []
    in_block = False
    for line in message.splitlines():
        if line.strip() == 'Chromium changes:':
            in_block = True
            continue
        if not in_block:
            continue
        if not line.strip():
            break
        for found in _CULPRIT_URL.findall(line):
            if found not in hashes:
                hashes.append(found)
    return hashes


def cherry_pick_origins(message: str) -> list[str]:
    """The commits an upstream release-branch commit was cherry-picked from."""
    return _CHERRY_PICKED.findall(message)


def _same_commit(hashes: list[str], others: list[str]) -> bool:
    """Whether any hash names the same commit as any other (short or full)."""
    return any(
        a.startswith(b) or b.startswith(a) for a in hashes for b in others
    )


@dataclass(frozen=True)
class Commit:
    sha: str
    subject: str
    # Culprits for a Brave commit; cherry-pick origins for a Chromium one.
    # Either way, the hashes that identify the upstream change.
    upstream: list[str]
    # The files a Chromium commit changed, when asked for.
    files: tuple[str, ...] = ()


# A file a tracker item names, such as `chrome/browser/foo.cc:12` or a bare
# `foo_impl.mm`.
_TRACKER_PATH = re.compile(
    r'(?<![\w./-])((?:[\w-]+/)*[\w.-]+\.'
    r'(?:cc|h|mm|m|java|kt|ts|js|py|gn|gni|grd|grdp|mojom|proto|json|html|'
    r'css|xml|yaml|star))\b'
)

# How many candidate fixes `next-tracker` lists for an item ticked without a
# recorded subject.
_CANDIDATES = 5


@dataclass(frozen=True)
class TrackerMatch:
    """A newer branch's tracker item that concerns a bump's cherry-picks."""

    title: str
    # The picks whose origin the item cites.
    cited: list[Commit]
    # (file the item names, pick that changed it).
    named: list[tuple[str, Commit]]
    # Every file the item names, upstream's and Brave's.
    paths: tuple[str, ...] = ()
    # Its `- [x]` and `- [ ]` tasks.
    ticked: int = 0
    open: int = 0
    # The subjects its ticked tasks were fixed in (``fixed in `<subject>` ``).
    fixed_in: tuple[str, ...] = ()

    @property
    def state(self) -> str:
        if self.ticked and not self.open:
            return 'ticked'
        if self.ticked:
            return 'partly ticked'
        return 'open'

    def fixes(self, base: str, branch: str) -> list[str]:
        """`<hash> <subject>` of the commits on *branch*, and not on *base*,
        that fixed the item's ticked tasks.

        By the subjects the tasks record. An item ticked without one gets the
        commits that changed a file it names instead, as candidates.
        """
        log = ['log', '--format=%h %s', f'{base}..{branch}']
        if self.fixed_in:
            return [
                line
                for subject in self.fixed_in
                for line in repository.brave.run_git(
                    *log, '--fixed-strings', f'--grep={subject}'
                ).splitlines()
                if line.split(' ', 1)[1] == subject
            ]
        if not self.paths:
            return []
        return repository.brave.run_git(
            *log, '--', *(f':(glob)**/{path}*' for path in self.paths)
        ).splitlines()

    @classmethod
    def find(
        cls, cherry_picks: list[Commit], tracker: str
    ) -> list[TrackerMatch]:
        """The `###` items of *tracker* that cite a pick's origin, or name a
        file a pick changed.

        Tooling TODOs are requests for tools rather than regressions, so
        their section is left out.
        """
        tracker = re.split(r'^## Tooling TODOs', tracker, flags=re.M)[0]
        matches = []
        for item in re.split(r'^(?=### )', tracker, flags=re.M)[1:]:
            title, _, body = item.partition('\n')
            cited = _CULPRIT_URL.findall(body)
            named = set(_TRACKER_PATH.findall(body))
            match = cls(
                title.removeprefix('### ').strip(),
                [
                    pick
                    for pick in cherry_picks
                    if _same_commit([pick.sha, *pick.upstream], cited)
                ],
                [
                    (path, pick)
                    for path in sorted(named)
                    for pick in cherry_picks
                    if any(
                        f == path or f.endswith(f'/{path}') for f in pick.files
                    )
                ],
                tuple(sorted(named)),
                len(re.findall(r'^\s*- \[x\]', body, flags=re.M)),
                len(re.findall(r'^\s*- \[ \]', body, flags=re.M)),
                tuple(re.findall(r'fixed in `([^`]+)`', body)),
            )
            if match.cited or match.named:
                matches.append(match)
        return matches


@dataclass(frozen=True)
class NextFix:
    """A Brave fix on a newer branch for an upstream change this bump picked."""

    cherry_pick: Commit
    branch: str
    fix: Commit


def match_next_fixes(
    cherry_picks: list[Commit], branches: dict[str, list[Commit]]
) -> list[NextFix]:
    """Pairs each cherry-pick with the newer branches' fixes for its origin.

    A fix cites the change by the hash it has on its own branch, which differs
    from the cherry-pick's, so both sides are compared by every hash they are
    known by: the cherry-pick by its own and its origins, a fix by its culprits
    and theirs.
    """
    return [
        NextFix(pick, branch, fix)
        for pick in cherry_picks
        for branch, fixes in branches.items()
        for fix in fixes
        if _same_commit([pick.sha, *pick.upstream], fix.upstream)
    ]


def _split_range(revision_range: str) -> tuple[str, str]:
    old, sep, new = revision_range.partition('..')
    if not sep or not old or not new:
        raise ValueError(f'Expected OLD..NEW, got {revision_range!r}')
    return old, new


def _is_ancestor(ancestor: str, descendant: str) -> bool:
    try:
        repository.chromium.run_git(
            'merge-base', '--is-ancestor', ancestor, descendant
        )
    except subprocess.CalledProcessError:
        return False
    return True


def culprit_problem(
    culprit: str, revision_range: str, picked: Sequence[str] = ()
) -> str | None:
    """Explains why `culprit` isn't a valid culprit, or returns None.

    Short hashes are accepted when they resolve, since teammates sometimes pass
    them; a mistyped or invented hash fails to resolve or falls outside the
    range. A main commit in `picked`, the origins of the range's cherry-picks,
    is valid too: fixes picked from a newer branch cite it.
    """
    old, new = _split_range(revision_range)
    if _same_commit([culprit], picked):
        return None
    try:
        full = repository.chromium.run_git(
            'rev-parse', '--verify', '--quiet', f'{culprit}^{{commit}}'
        )
    except subprocess.CalledProcessError:
        return f'{culprit} is not a Chromium commit'
    if not _is_ancestor(full, new) or _is_ancestor(full, old):
        return f'{culprit} is not in {revision_range}'
    return None


def choose_base(milestone: int, master_major: int) -> str:
    """Returns the branch `cr<milestone>` should be rebased onto.

    The previous milestone lives on `cr<milestone - 1>` until it lands on
    master, at which point master's Chromium version moves to that milestone.
    Whether `origin/cr<milestone - 1>` still exists, or is an ancestor of
    master, says nothing: branches outlive their merges, and merges can be
    squashed.
    """
    if master_major >= milestone - 1:
        return 'origin/master'
    return f'origin/cr{milestone - 1}'


def _unpushed_commits(branch: str) -> int:
    """Commits on the local branch that origin's copy lacks (0 if no branch)."""
    brave = repository.brave
    try:
        brave.run_git(
            'rev-parse', '--verify', '--quiet', f'refs/heads/{branch}'
        )
    except subprocess.CalledProcessError:
        return 0
    return int(
        brave.run_git('rev-list', '--count', f'origin/{branch}..{branch}')
    )


def _remote_branch_exists(branch: str) -> bool:
    try:
        repository.brave.run_git(
            'ls-remote', '--exit-code', '--heads', 'origin', branch
        )
    except subprocess.CalledProcessError:
        return False
    return True


def _brockit_show(option: str, prefix: str) -> Version | None:
    """The version `brockit show <option>` prints after `prefix`, if any."""
    output = subprocess.run(
        [sys.executable, str(_BROCKIT), 'show', option],
        cwd=repository.brave.root,
        check=False,
        capture_output=True,
        text=True,
    ).stdout.strip()
    if not output.startswith(prefix):
        return None
    return Version.from_value(output.rsplit(' ', 1)[-1])


def _chromium_version(label: str) -> Version | None:
    """The version a `brockit lift --to` label resolves to, if it resolves."""
    return _brockit_show(f'--chromium-version-label={label}', 'version:')


def _rebased_on(base: Version, head: Version) -> Version | None:
    """The base's version where HEAD's own commits start.

    That is where `brockit rebase` starts replaying them from, found the same
    way: before the last bump, or the last major bump when the base is on
    another major.
    """
    label = '@previous' if base.major == head.major else '@previous-major'
    return _brockit_show(f'--from-ref-value={label}', 'base version:')


def maintenance_target(
    milestone: int, current: Version, releases: list[Version | None]
) -> Version | None:
    """The tag a branch with a newer cr<N+1> above it should lift to, if any.

    Such a branch no longer follows `@latest-for-branch`: it only moves to a
    beta or stable release of its own milestone newer than its current tag.
    """
    return max(
        (
            release
            for release in releases
            if release is not None
            and release.major == milestone
            and release > current
        ),
        default=None,
    )


def _lift_target_file(milestone: int) -> Path:
    """Where `prepare` records the `--to` value for `lift`, or `none`."""
    return run_dir(milestone) / 'lift-target'


def moves_to_master(milestone: int, base: str, fork_major: int | None) -> bool:
    """Whether this rebase is the one moving cr<milestone> onto master.

    That happens once, right after the previous milestone lands on master:
    master is then the base, but the branch still forked from master before
    master reached milestone - 1. `fork_major` is the Chromium major at the
    merge-base of origin/cr<milestone> and origin/master.
    """
    return (
        base == 'origin/master'
        and fork_major is not None
        and fork_major < milestone - 1
    )


def _fork_major(branch: str) -> int | None:
    fork = repository.brave.run_git(
        'merge-base', f'origin/{branch}', 'origin/master'
    )
    return Version.from_git(fork).major


def chromium_mismatch() -> str | None:
    """Says how the Chromium checkout is off the branch's version, if it is.

    `brockit rebase` regenerates conflicted patches from their plasters
    against the checkout, only when it is on the version rebased so far: the
    branch's own, for almost all of its commits. The checkout's version is
    `HEAD:chrome/VERSION`, as Brockit reads it: Brave's patches rewrite the
    working copy with Brave's own version.
    """
    checkout = read_chromium_version_file()
    branch = Version.from_git('HEAD')
    if checkout == branch:
        return None
    return f'Chromium is checked out at {checkout}, the branch is on {branch}'


def sync_chromium(milestone: int) -> int:
    """Runs `pnpm run sync`, logged in the run's directory."""
    log = run_dir(milestone) / 'sync.log'
    with log.open('wb') as file:
        returncode = subprocess.run(
            ['pnpm', 'run', 'sync'],
            cwd=repository.brave.root,
            stdout=file,
            stderr=subprocess.STDOUT,
            check=False,
        ).returncode
    if returncode:
        print(f'`pnpm run sync` failed; see {log}. Not rebasing.')
    return returncode


def _cmd_prepare(args: argparse.Namespace) -> int:
    brave = repository.brave
    branch = f'cr{args.milestone}'
    if brave.is_rebase_in_progress():
        print('A rebase is in progress; finish or abort it first.')
        return 1
    if not args.force and brave.run_git(
        'status', '--porcelain', '--untracked-files=no'
    ):
        print(
            f'Tracked files have changes, and resetting {branch} would '
            'discard them. Commit or stash them, or pass --force.'
        )
        return 1

    # A new milestone has no branch yet; it starts from its base instead.
    exists = _remote_branch_exists(branch)
    if exists and not args.force:
        brave.run_git('fetch', 'origin', branch)
        if unpushed := _unpushed_commits(branch):
            print(
                f'Local {branch} has {unpushed} commit(s) that origin/{branch} '
                'lacks, and resetting it would drop them. Push them, or pass '
                '--force.'
            )
            return 1
    brave.run_git('fetch', 'origin', 'master', *([branch] if exists else []))
    master_version = Version.from_git('origin/master')
    base = choose_base(args.milestone, master_version.major)
    if base != 'origin/master':
        brave.run_git('fetch', 'origin', base.removeprefix('origin/'))

    transition = exists and moves_to_master(
        args.milestone, base, _fork_major(branch)
    )

    start = f'origin/{branch}' if exists else base
    brave.run_git('checkout', '-B', branch, start)
    # What `publish` takes as GitHub's tip when the run started.
    Publish.record_start(args.milestone, brave.run_git('rev-parse', 'HEAD'))
    brave.run_git('branch', f'--set-upstream-to={base}')
    upstream = brave.run_git('rev-parse', '--abbrev-ref', '@{upstream}')
    if upstream != base:
        print(f'Upstream is {upstream}, expected {base}.')
        return 1
    summary = (
        f'- master is on Chromium {master_version}.\n'
        f'- {branch} {"reset to" if exists else "created from"} {start}.\n'
        f'- Upstream (base): {base}.\n'
    )

    # Once cr<N+1> exists, cr<N> is no longer the head branch, and only
    # follows its milestone's beta and stable releases.
    maintenance = exists and _remote_branch_exists(f'cr{args.milestone + 1}')
    regen = False
    if maintenance:
        current = Version.from_git('HEAD')
        beta = _chromium_version('@latest-beta')
        stable = _chromium_version('@latest-stable')
        if beta is None or stable is None:
            print(
                'Could not resolve @latest-beta and @latest-stable, so whether '
                f'{branch} needs a lift is unknown.'
            )
            return 1
        target = maintenance_target(args.milestone, current, [beta, stable])
        lift_to = str(target) if target else None
        base_version = Version.from_git(base)
        summary += (
            f'- Maintenance run: origin/cr{args.milestone + 1} exists, so '
            f'{branch} lifts only to a beta or stable release of '
            f'M{args.milestone}.\n'
            f'- {branch} is on {current}; @latest-beta is {beta}, '
            f'@latest-stable is {stable}.\n'
        )
        if target:
            # The ranges the run will have once lifted, rather than now.
            summary += (
                f'- Lift needed: to {target}.\n'
                f'- Bump range: {current}..{target}\n'
                f'- Branch range, once lifted: {base_version}..{target}'
            )
        else:
            summary += f'- Branch range: {base_version}..{current}\n'
            rebased_on = _rebased_on(base_version, current)
            if rebased_on is None:
                print(f'Could not find the base version {branch} starts from.')
                return 1
            # A base bump brings its own regen commits, which conflict with
            # the branch's; drop the branch's, and `brockit regen` remakes them.
            regen = rebased_on != base_version
            if regen:
                summary += (
                    f'- No lift needed, but {base} moved from {rebased_on} '
                    f'to {base_version}: rebase discarding the regen changes, '
                    'then run `brockit regen`, build, and stop.'
                )
            else:
                summary += (
                    '- No lift needed: rebase only, keeping the regen '
                    'changes. Then sync, build, and stop.'
                )
    else:
        lift_to = '@latest-for-branch'
        tag = _chromium_version(f'@latest-m{args.milestone}')
        summary += (
            f'- Target tag (@latest-m{args.milestone}): {tag or "unknown"}.'
        )
    _lift_target_file(args.milestone).write_text(
        f'{lift_to or "none"}\n', encoding='utf-8'
    )
    if not exists:
        summary += (
            '\n- New milestone: no rebase; publish at the end once the '
            'builds pass.'
        )
    if transition:
        summary += (
            f'\n- Base moves to origin/master: cr{args.milestone - 1} has '
            f'landed, and {branch} still forks from master before it. Expect '
            "conflicts from master's own changes in this rebase, and retarget "
            f"{branch}'s pull request at master (the CI job's call)."
        )
    # A new branch has nothing of its own to rebase yet.
    do_rebase = exists and not args.no_rebase
    mismatch = chromium_mismatch() if do_rebase else None
    if mismatch:
        summary += f'\n- {mismatch}: synced before the rebase.'
    print(summary)
    append_report(args.milestone, f'prepare {args.milestone}', summary)

    if not do_rebase:
        return 0
    if mismatch and (returncode := sync_chromium(args.milestone)):
        return returncode
    cmd = [sys.executable, str(_BROCKIT), 'rebase', '--squash-minor-bumps']
    # Without a lift or `brockit regen` to remake them, they must stay.
    if lift_to or regen:
        cmd.append('--discard-regen-changes')
    return subprocess.run(cmd, cwd=brave.root, check=False).returncode


class Publish:
    """`autolift.py publish`: brings cr<N> up to date with origin, and pushes.

    In order: cherry-picks what landed on origin's cr<N> since the run
    started; moves the upstream to the base's latest tip, once per run; runs
    `brockit rebase --squash-minor-bumps` if that would change the branch,
    with `--discard-regen-changes` and a `brockit regen` after it if the base
    had a lift of its own; and, once a branch it changed has been rebuilt,
    force-pushes it with a lease.

    Its state between rounds lives in the run's directory, and what it does
    goes to the run's report, however the round ends.
    """

    # Exit status when the branch changed and wants a rebuild, or GitHub
    # moved just before the push: run publish again.
    AGAIN = 2

    # Picks and rebases leave the checkout behind the branch's DEPS and
    # patches, so the rebuild always syncs first.
    REBUILD = (
        'The branch changed: run `pnpm run sync`, rebuild, then run '
        '`publish --built`.'
    )

    def __init__(self, milestone: int):
        self.milestone = milestone
        self.branch = f'cr{milestone}'
        self._brave = repository.brave
        self._notes: list[str] = []

    @classmethod
    def record_start(cls, milestone: int, commit: str) -> None:
        """Records the commit `prepare` reset the branch to, for a new run."""
        publish = cls(milestone)
        for name in ('synced', 'base-synced', 'rebuild-needed', 'regen-needed'):
            publish._state(name).unlink(missing_ok=True)
        publish._state('start').write_text(f'{commit}\n', encoding='utf-8')

    def run(self, *, built: bool) -> int:
        try:
            return self._run(built=built)
        finally:
            if self._notes:
                append_report(
                    self.milestone,
                    f'publish {self.milestone}',
                    '\n'.join(f'- {note}' for note in self._notes),
                )

    def _run(self, *, built: bool) -> int:
        brave = self._brave
        if brave.current_branch() != self.branch:
            print(f'Check out {self.branch} first.')
            return 1
        if stopped := self._in_progress():
            print(f'A {stopped} is in progress; finish it first.')
            return 1
        if brave.run_git('status', '--porcelain', '--untracked-files=no'):
            print('Tracked files have changes; commit them first.')
            return 1
        rebuild = self._state('rebuild-needed')
        if rebuild.exists():
            if not built:
                print(self.REBUILD)
                return self.AGAIN
            rebuild.unlink()

        self._refresh_origin()
        remote = self._origin_tip(self.branch)
        new = remote is None
        if new:
            expected = ''
        elif not (start := self._state('start')).exists():
            print(f'Run `autolift.py prepare {self.milestone}` first.')
            return 1
        else:
            synced = self._state('synced')
            expected = (
                (synced if synced.exists() else start)
                .read_text(encoding='utf-8')
                .strip()
            )

        # Set before changing anything, so that a pick or rebase that stops,
        # and is finished by hand, still asks for a sync and rebuild.
        head = brave.run_git('rev-parse', 'HEAD')
        rebuild.touch()
        if not new and remote != expected:
            # Recorded first: once the picks are done, `remote` is the lease.
            self._state('synced').write_text(f'{remote}\n', encoding='utf-8')
            if returncode := self._pick_landed(expected, remote):
                return returncode
            expected = remote
        self._sync_base()
        if returncode := self._rebase():
            return returncode
        if brave.run_git('rev-parse', 'HEAD') != head:
            self._note(self.REBUILD)
            return self.AGAIN
        rebuild.unlink()

        if new:
            return self._push_new()
        if head == expected:
            self._note(
                f'{self.branch} is unchanged since the run started; nothing '
                'pushed.'
            )
            return 0
        if not self._push(expected):
            return self.AGAIN
        self._note(f'Pushed {self.branch} over {expected[:12]}.')
        self._note_for_review(expected)
        return 0

    def _note(self, message: str) -> None:
        print(message)
        self._notes.append(message)

    def _state(self, name: str) -> Path:
        return run_dir(self.milestone) / name

    def _brockit(self, *args: str) -> int:
        return subprocess.run(
            [sys.executable, str(_BROCKIT), *args],
            cwd=self._brave.root,
            check=False,
        ).returncode

    def _in_progress(self) -> str | None:
        """A rebase or cherry-pick left stopped, if any."""
        if self._brave.is_rebase_in_progress():
            return 'rebase'
        try:
            self._brave.run_git(
                'rev-parse', '--verify', '--quiet', 'CHERRY_PICK_HEAD'
            )
        except subprocess.CalledProcessError:
            return None
        return 'cherry-pick'

    def _git_cache(self) -> tuple[str, str] | None:
        """(cache dir, mirrored URL) when `origin` fetches from a git cache.

        CI clones from a `git cache` mirror under `GIT_CACHE_PATH`: `origin`
        fetches from the mirror's directory, and pushes to GitHub.
        """
        cache_dir = os.environ.get('GIT_CACHE_PATH')
        fetch_url = self._brave.run_git('remote', 'get-url', 'origin')
        if not cache_dir or not Path(fetch_url).is_dir():
            return None
        return cache_dir, self._brave.run_git(
            'remote', 'get-url', '--push', 'origin'
        )

    def _refresh_origin(self) -> None:
        """Brings the git cache, if there is one, up to GitHub's branches.

        The mirror is only as recent as the job's start, which a long run
        outlives. Without a cache, `origin` is GitHub already.
        """
        if cache := self._git_cache():
            cache_dir, url = cache
            self._brave.run_git(
                'cache',
                'populate',
                '--cache-dir',
                cache_dir,
                url,
                '--reset-fetch-config',
                '--no-fetch-tags',
            )

    def _origin_tip(self, branch: str) -> str | None:
        """Fetches `origin/<branch>`, and returns its tip (None if it has none).

        As recent as `origin` is: `_refresh_origin()` first for GitHub's.
        """
        output = self._brave.run_git(
            'ls-remote', '--heads', 'origin', f'refs/heads/{branch}'
        )
        if not output:
            return None
        self._brave.run_git('fetch', 'origin', branch)
        return output.split()[0]

    def _pick_landed(self, expected: str, remote: str) -> int | None:
        """Cherry-picks what landed on origin's branch since `expected`.

        Returns an exit status to stop with, or None once picked.
        """
        brave = self._brave
        try:
            brave.run_git('merge-base', '--is-ancestor', expected, remote)
        except subprocess.CalledProcessError:
            self._note(
                f'{self.branch} on GitHub was rewritten: {remote[:12]} no '
                f'longer contains {expected[:12]}. Not pushing.'
            )
            return 1
        landed = brave.run_git(
            'log', '--reverse', '--format=%h %s', f'{expected}..{remote}'
        )
        self._note(f'Picked what landed on GitHub since {expected[:12]}:')
        self._notes += [f'  - `{line}`' for line in landed.splitlines()]
        try:
            brave.run_git(
                'cherry-pick', '--empty=drop', f'{expected}..{remote}'
            )
        except subprocess.CalledProcessError:
            self._note(
                'A cherry-pick conflicts. Resolve it, `git add` the files, '
                'run `git cherry-pick --continue`, and run publish again.'
            )
            return 1
        return None

    def _sync_base(self) -> None:
        """Moves the upstream to the base's latest tip, once per run.

        Only once: on a long run, master moves on between every two rounds,
        and following it each time would never get to the push. A base whose
        Chromium version moved had a lift of its own, which `_rebase` follows
        with a regen.
        """
        synced = self._state('base-synced')
        if synced.exists():
            return
        upstream = self._brave.run_git(
            'rev-parse', '--abbrev-ref', '@{upstream}'
        )
        base = upstream.removeprefix('origin/')
        before = self._brave.run_git('rev-parse', upstream)
        version = Version.from_git(upstream)
        after = self._origin_tip(base)
        if after is None:
            self._note(f'{base} is gone from GitHub; staying on {upstream}.')
            synced.write_text('gone\n', encoding='utf-8')
            return
        synced.write_text(f'{after}\n', encoding='utf-8')
        if before == after:
            return
        self._note(f'{upstream} moved: {before[:12]}..{after[:12]}.')
        if (lifted := Version.from_git(upstream)) != version:
            self._note(
                f'{upstream} had a lift of its own ({version}..{lifted}): '
                'rebasing without the regen changes, and regenerating them.'
            )
            self._state('regen-needed').touch()

    def _rebase(self) -> int:
        """Rebases if that would change the branch, and regenerates if asked.

        The rebase happens if the base moved, or anything is left to squash:
        picked fixups, or the minor bump the run's lift added. Returns an exit
        status to stop with, or 0.
        """
        regen = self._state('regen-needed')
        rebase_args = ['rebase', '--squash-minor-bumps']
        if regen.exists():
            rebase_args.append('--discard-regen-changes')
        needed = self._brockit(*rebase_args, '--dry-run')
        if needed not in (0, _REBASE_NEEDED):
            self._note('`brockit rebase --dry-run` failed; not pushing.')
            return 1
        if needed:
            if mismatch := chromium_mismatch():
                self._note(f'{mismatch}: ran `pnpm run sync` first.')
                if returncode := sync_chromium(self.milestone):
                    return returncode
            self._note(f'Ran `brockit {" ".join(rebase_args)}`.')
            if returncode := self._brockit(*rebase_args):
                self._note(
                    'The rebase stopped. Finish it with `brockit rebase '
                    '--continue`, and run publish again.'
                )
                return returncode
        if regen.exists():
            self._note('Ran `brockit regen`.')
            if returncode := self._brockit('regen', '--infra-mode'):
                self._note('`brockit regen` failed; not pushing.')
                return returncode
            regen.unlink()
        return 0

    def _push(self, expected: str) -> bool:
        """Force-pushes HEAD over `expected` ('' for none), if still there.

        Returns False, without pushing, if origin's branch moved on.
        """
        # A fetch right before the push makes sure nothing landed since the
        # last pick; the lease covers the moment between the two.
        self._refresh_origin()
        if (tip := self._origin_tip(self.branch) or '') != expected:
            self._note(
                f'{self.branch} moved on GitHub to '
                f'{tip[:12] or "a new branch"} just before the push. Not '
                'pushing; run publish again to pick it.'
            )
            return False
        # No --set-upstream: the upstream must stay the base, which is also
        # the base of the pull request.
        self._brave.run_git(
            'push',
            f'--force-with-lease=refs/heads/{self.branch}:{expected}',
            'origin',
            f'HEAD:refs/heads/{self.branch}',
        )
        return True

    def _push_new(self) -> int:
        """Pushes a new milestone, and opens its version issue and PR."""
        upstream = self._brave.run_git(
            'rev-parse', '--abbrev-ref', '@{upstream}'
        )
        ahead = self._brave.run_git('rev-list', '--count', f'{upstream}..HEAD')
        if not int(ahead):
            print(f'{self.branch} has no commits on top of {upstream}.')
            return 1
        if not self._push(''):
            return self.AGAIN
        self._note(f'Pushed the new {self.branch}.')
        self._note_for_review('')
        return self._brockit('update-version-issue')

    # Groups of `rebase.PINNED_GROUPS` whose commits Brockit writes itself.
    # Conflict-resolved patches aren't among them: they hold the agent's
    # resolutions.
    _BROCKIT_GROUPS = frozenset({'version', 'plaster_reruns', 'gnrt'})

    def _note_for_review(self, pushed_over: str) -> None:
        """Notes the pushed commits the run wrote or changed, as URLs.

        A commit counts when no commit GitHub had before the push
        (`pushed_over`, '' for a new branch) has its diff: rebases change
        hashes, but not diffs. So a squashed-in fixup counts, and a picked
        teammate's commit doesn't. Commits Brockit writes itself don't count
        either.
        """
        upstream = self._brave.run_git(
            'rev-parse', '--abbrev-ref', '@{upstream}'
        )
        before = (
            set(self._patch_ids(f'{upstream}..{pushed_over}').values())
            if pushed_over
            else set()
        )
        after = self._patch_ids(f'{upstream}..HEAD')
        url = self._commit_url_base()
        review = [
            f'  - [{subject}]({url}{sha})'
            for sha, subject in self._commits(f'{upstream}..HEAD')
            if after.get(sha) not in before
            and not self._written_by_brockit(subject)
        ]
        if review:
            self._notes.append(f'To review ({len(review)}):')
            self._notes.extend(review)
        else:
            self._notes.append('Nothing to review: no commit changed.')

    @classmethod
    def _written_by_brockit(cls, subject: str) -> bool:
        return rebase.get_pinned_group_for_subject(
            subject
        ) in cls._BROCKIT_GROUPS or rebase.is_recyclable(subject)

    def _commits(self, revision_range: str) -> list[tuple[str, str]]:
        output = self._brave.run_git(
            'log', '--reverse', '--format=%H%x00%s', revision_range
        )
        return [tuple(line.split('\x00', 1)) for line in output.splitlines()]

    def _patch_ids(self, revision_range: str) -> dict[str, str]:
        """`{commit: patch id}` for the commits of *revision_range*."""
        diffs = self._brave.run_git(
            'log', '-p', '--format=commit %H', revision_range, no_trim=True
        )
        output = subprocess.run(
            ['git', 'patch-id', '--stable'],
            cwd=self._brave.root,
            input=diffs,
            capture_output=True,
            text=True,
            check=True,
        ).stdout
        return {
            commit: patch_id
            for patch_id, commit in (
                line.split() for line in output.splitlines()
            )
        }

    def _commit_url_base(self) -> str:
        """`https://github.com/<owner>/<repo>/commit/`, from the push URL."""
        url = self._brave.run_git('remote', 'get-url', '--push', 'origin')
        match = re.search(r'github\.com[:/]([^/]+/[^/]+?)(?:\.git)?/?$', url)
        repo = match.group(1) if match else 'brave/brave-core'
        return f'https://github.com/{repo}/commit/'


def _cmd_publish(args: argparse.Namespace) -> int:
    return Publish(args.milestone).run(built=args.built)


def run_dir(milestone: int) -> Path:
    """Returns the directory holding the run's logs and report."""
    directory = _RUN_ROOT / f'cr{milestone}'
    directory.mkdir(parents=True, exist_ok=True)
    return directory


def append_report(milestone: int, title: str, body: str) -> Path:
    """Appends a section to the run's `report.md`, and returns its path."""
    report = run_dir(milestone) / 'report.md'
    stamp = datetime.now().strftime('%Y-%m-%d %H:%M')
    with report.open('a', encoding='utf-8', newline='') as file:
        file.write(f'## {title} ({stamp})\n\n{body.rstrip()}\n\n')
    return report


def _branch_milestone() -> int | None:
    match = re.fullmatch(r'cr(\d+)', repository.brave.current_branch())
    return int(match.group(1)) if match else None


@dataclass(frozen=True)
class LiftSummary:
    """What a `brockit lift` log says needs doing."""

    bump_range: str | None
    branch_range: str | None
    # (section title, items) for each "(action needed)" section.
    sections: list[tuple[str, list[str]]]
    # ❌ checks and 🔥 errors.
    flags: list[str]
    done: bool

    @property
    def needs_attention(self) -> bool:
        return bool(self.sections or self.flags or not self.done)


def summarize_lift(log: str) -> LiftSummary:
    """Parses a `brockit lift` log into what needs attention."""
    bump_range = branch_range = None
    sections: list[tuple[str, list[str]]] = []
    flags: list[str] = []
    items: list[str] | None = None
    for raw in strip_ansi(log).splitlines():
        line = _LOG_SOURCE.sub('', _LOG_TIME.sub('', raw)).strip()
        if 'Changes for this bump' in line and (m := _LIFT_RANGE.search(line)):
            bump_range = m.group(1)
        if 'Changes since base version' in line and (
            m := _LIFT_RANGE.search(line)
        ):
            branch_range = m.group(1)
        if line.startswith('👋'):
            # "Address all sections with (action needed) above, ..."
            items = None
            continue
        if '(action needed)' in line:
            title = line.split('(action needed)')[0].lstrip('* ').strip()
            items = []
            sections.append((title, items))
            continue
        if '❌' in line:
            flags.append(line.split('❌', 1)[1].strip())
        if '🔥🔥' in line:
            flags.append(line.split('🔥🔥', 1)[1].strip())
        if items is None:
            continue
        if line.startswith('✘'):
            items.append(line.lstrip('✘ ').strip())
        elif line.startswith('(renamed to') and items:
            items[-1] += f' {line}'
        elif line.startswith('* '):
            items = None
    return LiftSummary(
        bump_range, branch_range, sections, flags, '💥 Done!' in log
    )


def _package_range() -> str | None:
    """`<upstream tag>..<HEAD tag>` from `package.json`, if they differ."""
    old = Version.from_git('@{upstream}')
    new = Version.from_git('HEAD')
    return None if old == new else f'{old}..{new}'


def with_package_range(
    summary: LiftSummary, package_range: str | None
) -> LiftSummary:
    """Fills the bump range when Brockit didn't print it (a new major)."""
    return dataclasses.replace(
        summary, bump_range=summary.bump_range or package_range
    )


def format_lift_summary(
    summary: LiftSummary, returncode: int, log: Path
) -> str:
    lines = [
        f'- Bump range: {summary.bump_range or "not printed"}',
        f'- Branch range: {summary.branch_range or "not printed"}',
        f'- Brockit exit status: {returncode}',
        f'- Log: `{log}`',
    ]
    for title, items in summary.sections:
        lines.append(f'- {title} (action needed):')
        lines += [f'  - `{item}`' for item in items]
    lines += [f'- ❌ {flag}' for flag in summary.flags]
    if summary.done and not summary.needs_attention:
        lines.append('- Finished with nothing left to do.')
    elif not summary.done and not summary.sections:
        lines.append('- Stopped before finishing; see the end of the log.')
    return '\n'.join(lines)


def _cmd_lift(args: argparse.Namespace) -> int:
    milestone = _branch_milestone()
    if milestone is None:
        print('Check out a cr<N> branch first.')
        return 1
    target_file = _lift_target_file(milestone)
    if not target_file.exists():
        print(f'Run `autolift.py prepare {milestone}` first.')
        return 1
    lift_to = target_file.read_text(encoding='utf-8').strip()
    if lift_to == 'none':
        print(
            f'No lift for this run: cr{milestone + 1} exists, and no beta or '
            f'stable release of M{milestone} is newer than cr{milestone}.'
        )
        return 1
    name = 'lift-continue' if args.continue_ else 'lift'
    stamp = datetime.now().strftime('%Y%m%d-%H%M%S')
    log = run_dir(milestone) / f'{name}-{stamp}.log'
    cmd = [
        sys.executable,
        str(_BROCKIT),
        'lift',
        '--infra-mode',
        '--with-github',
        f'--to={lift_to}',
    ]
    if args.continue_:
        cmd.append('--continue')
    with log.open('wb') as file:
        returncode = subprocess.run(
            cmd,
            cwd=repository.brave.root,
            stdout=file,
            stderr=subprocess.STDOUT,
            check=False,
        ).returncode
    summary = with_package_range(
        summarize_lift(log.read_bytes().decode('utf-8', errors='replace')),
        _package_range(),
    )
    text = format_lift_summary(summary, returncode, log)
    report = append_report(milestone, f'brockit {" ".join(cmd[2:])}', text)
    print(text)
    print(f'Appended to {report}.')
    # Brockit can exit 0 with work left (e.g. a failed plaster check), so the
    # log decides too.
    if returncode:
        return returncode
    return 2 if summary.needs_attention else 0


def _cmd_build_summary(args: argparse.Namespace) -> int:
    log = args.log.read_bytes().decode('utf-8', errors='replace')
    summary = summarize_build(log)
    print(f'Status: {summary.status or "unknown (no final status line)"}')
    print(f'Failed steps ({len(summary.failed_steps)}):')
    for step in summary.failed_steps:
        print(f'  {step}')
    print(f'Distinct errors ({len(summary.errors)}):')
    for count, error in summary.errors[: args.limit]:
        print(f'  {count:4d}x {error}')
    if len(summary.errors) > args.limit:
        print(f'  ... {len(summary.errors) - args.limit} more')
    return 0


def _cmd_culprits(args: argparse.Namespace) -> int:
    _split_range(args.range)
    cmd = ['log', '--format=%H %s', args.range]
    if args.symbol:
        cmd.append(f'-S{args.symbol}')
    if args.paths:
        cmd += ['--', *args.paths]
    output = repository.chromium.run_git(*cmd)
    if output:
        print(output)
    return 0


def _log(repo: repository.Repository, *cmd: str) -> list[tuple[str, str, str]]:
    """(hash, subject, message) for each commit `git log` lists."""
    output = repo.run_git(
        'log', '--format=%H%x00%s%x00%B%x01', *cmd, no_trim=True
    )
    return [
        tuple(entry.strip('\n').split('\x00', 2))
        for entry in output.split('\x01')
        if entry.strip()
    ]


def _chromium_origins(commit: str) -> list[str]:
    """Cherry-pick origins of a Chromium commit, if the checkout has it."""
    try:
        message = repository.chromium.run_git(
            'log', '-1', '--format=%B', commit
        )
    except subprocess.CalledProcessError:
        return []
    return cherry_pick_origins(message)


def _newer_branches(milestone: int) -> list[str]:
    """The `cr<M>` branches on origin with M > milestone, fetched."""
    output = repository.brave.run_git(
        'ls-remote', '--heads', 'origin', 'refs/heads/cr*'
    )
    majors = sorted(
        int(major) for major in re.findall(r'refs/heads/cr(\d+)$', output, re.M)
    )
    branches = [f'cr{major}' for major in majors if major > milestone]
    if branches:
        repository.brave.run_git('fetch', 'origin', *branches)
    return branches


def _fixes_on(branch: str, milestone: int) -> list[Commit]:
    """Commits on origin/<branch> that cr<milestone> lacks, with culprits."""
    fixes = []
    for sha, subject, body in _log(
        repository.brave, f'origin/cr{milestone}..origin/{branch}'
    ):
        culprits = parse_culprit_hashes(body)
        if not culprits:
            continue
        upstream = list(culprits)
        for culprit in culprits:
            upstream += _chromium_origins(culprit)
        fixes.append(Commit(sha, subject, upstream))
    return fixes


def _cherry_picks(
    revision_range: str, paths: list[str], *, with_files: bool = False
) -> list[Commit]:
    """The upstream cherry-picks in a release branch range, with origins, and
    with the files each changed if *with_files*."""
    _split_range(revision_range)
    files: dict[str, tuple[str, ...]] = {}
    if with_files:
        output = repository.chromium.run_git(
            'log', '--format=%x01%H', '--name-only', revision_range
        )
        for entry in output.split('\x01'):
            if entry.strip():
                sha, *changed = entry.split()
                files[sha] = tuple(changed)
    return [
        Commit(sha, subject, origins, files.get(sha, ()))
        for sha, subject, body in _log(
            repository.chromium,
            revision_range,
            *(['--', *paths] if paths else []),
        )
        if (origins := cherry_pick_origins(body))
    ]


def _find_next_fixes(
    milestone: int, revision_range: str, paths: list[str]
) -> list[NextFix]:
    cherry_picks = _cherry_picks(revision_range, paths)
    branches = {
        branch: _fixes_on(branch, milestone)
        for branch in _newer_branches(milestone)
    }
    return match_next_fixes(cherry_picks, branches)


def _cmd_next_tracker(args: argparse.Namespace) -> int:
    milestone = _branch_milestone()
    if milestone is None:
        print('Check out a cr<N> branch first.')
        return 1
    cherry_picks = _cherry_picks(args.range, [], with_files=True)
    for branch in _newer_branches(milestone):
        path = f'tools/cr/M{branch.removeprefix("cr")}_codehealth.md'
        try:
            tracker = repository.brave.run_git(
                'show', f'origin/{branch}:{path}', no_trim=True
            )
        except subprocess.CalledProcessError:
            print(f'origin/{branch} has no {path}.')
            continue
        matches = TrackerMatch.find(cherry_picks, tracker)
        print(
            f'origin/{branch}:{path}: {len(matches)} item(s) concern this bump.'
        )
        for match in matches:
            print(f'  {match.title} ({match.state})')
            for pick in match.cited:
                print(f'    cites {pick.sha} {pick.subject}')
            for file, pick in match.named:
                print(f'    names {file}: changed by {pick.sha} {pick.subject}')
            if match.ticked:
                fixes = match.fixes(f'origin/cr{milestone}', f'origin/{branch}')
                if match.fixed_in:
                    for fix in fixes:
                        print(f'    fix on origin/{branch}: {fix}')
                else:
                    # No recorded subject: the commits that changed a file the
                    # item names, most recent first, to be checked by hand.
                    for fix in fixes[:_CANDIDATES]:
                        print(f'    candidate on origin/{branch}: {fix}')
                    if len(fixes) > _CANDIDATES:
                        print(f'    ... {len(fixes) - _CANDIDATES} more')
                if not fixes:
                    print(f'    no fix found on origin/{branch}')
    return 0


def _cmd_next_fixes(args: argparse.Namespace) -> int:
    milestone = _branch_milestone()
    if milestone is None:
        print('Check out a cr<N> branch first.')
        return 1
    picked = {
        culprit
        for _, _, body in _log(repository.brave, '@{upstream}..HEAD')
        for culprit in parse_culprit_hashes(body)
    }
    matches = _find_next_fixes(milestone, args.range, args.paths)
    for match in matches:
        pick = match.cherry_pick
        done = _same_commit([pick.sha, *pick.upstream], list(picked))
        print(f'{pick.sha} {pick.subject}')
        print(f'  cherry-picked from {", ".join(pick.upstream)}')
        print(
            f'  origin/{match.branch} {match.fix.sha[:12]} {match.fix.subject}'
            + (f' (already on cr{milestone})' if done else '')
        )
    print(f'{len(matches)} fix(es) found on newer branches.')
    return 0


def _cmd_pick_fix(args: argparse.Namespace) -> int:
    brave = repository.brave
    if _branch_milestone() is None:
        print('Check out a cr<N> branch first.')
        return 1
    # Without a culprit, the commit is a tracker item's fix, as next-tracker
    # lists it, and is picked as it is.
    if args.culprit:
        culprit = repository.chromium.run_git('rev-parse', args.culprit)
        _, _, message = _log(brave, '-1', args.commit)[0]
        fix_upstream = list(parse_culprit_hashes(message))
        for hash_ in list(fix_upstream):
            fix_upstream += _chromium_origins(hash_)
        origins = [culprit, *_chromium_origins(culprit)]
        if not _same_commit(origins, fix_upstream):
            print(
                f'{args.commit} cites none of the commits {culprit} was '
                'cherry-picked from; it is not the fix for this culprit.'
            )
            return 1
    # The message stays as it is, `[cr<M>]` prefix and culprits included.
    try:
        brave.run_git('cherry-pick', args.commit)
    except subprocess.CalledProcessError:
        print(
            'The cherry-pick conflicts. Resolve it, `git add` the files, '
            'and run `git cherry-pick --continue`.'
        )
        return 1
    return 0


def _cmd_verify_culprits(args: argparse.Namespace) -> int:
    picked = [
        origin
        for pick in _cherry_picks(args.range, [])
        for origin in pick.upstream
    ]
    problems = 0
    for commit, subject, body in _log(repository.brave, f'{args.base}..HEAD'):
        for culprit in parse_culprit_hashes(body):
            if problem := culprit_problem(culprit, args.range, picked):
                problems += 1
                print(f'{commit[:12]} {subject}: {problem}')
    print(f'{problems} problem(s) found.')
    return 1 if problems else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    subparsers = parser.add_subparsers(dest='command', required=True)

    prepare = subparsers.add_parser(
        'prepare',
        help='Reset cr<N>, set its base as upstream, and start the rebase.',
    )
    prepare.add_argument('milestone', type=int, help='e.g. 158')
    prepare.add_argument(
        '--no-rebase',
        action='store_true',
        help='Stop before running brockit rebase.',
    )
    prepare.add_argument(
        '--force',
        action='store_true',
        help='Reset even if tracked files have changes, or the local branch '
        'has commits origin lacks (discards them).',
    )
    prepare.set_defaults(func=_cmd_prepare)

    lift = subparsers.add_parser(
        'lift',
        help='Run brockit lift for the current cr<N> branch, and summarise it.',
    )
    lift.add_argument(
        '--continue',
        dest='continue_',
        action='store_true',
        help='Pass --continue, after handling action items.',
    )
    lift.set_defaults(func=_cmd_lift)

    publish = subparsers.add_parser(
        'publish',
        help='Push cr<N>, picking what landed on GitHub meanwhile first.',
    )
    publish.add_argument('milestone', type=int, help='e.g. 159')
    publish.add_argument(
        '--built',
        action='store_true',
        help='The branch was rebuilt after publish last changed it.',
    )
    publish.set_defaults(func=_cmd_publish)

    summary = subparsers.add_parser(
        'build-summary', help='Summarise a build log.'
    )
    summary.add_argument('log', type=Path)
    summary.add_argument(
        '--limit',
        type=int,
        default=60,
        help='Maximum number of distinct errors to print.',
    )
    summary.set_defaults(func=_cmd_build_summary)

    culprits = subparsers.add_parser(
        'culprits', help='List upstream commits in a range touching paths.'
    )
    culprits.add_argument('range', help='e.g. 158.0.8117.1..158.0.8127.2')
    culprits.add_argument(
        '--symbol', help='Only commits adding or removing this string.'
    )
    culprits.add_argument(
        'paths', nargs='*', help='Chromium-relative paths, e.g. net/http/x.cc'
    )
    culprits.set_defaults(func=_cmd_culprits)

    next_fixes = subparsers.add_parser(
        'next-fixes',
        help='List fixes on newer cr branches for changes this bump picked.',
    )
    next_fixes.add_argument('range', help='e.g. 158.0.8127.2..158.0.8127.4')
    next_fixes.add_argument(
        'paths', nargs='*', help='Chromium-relative paths, e.g. net/http/x.cc'
    )
    next_fixes.set_defaults(func=_cmd_next_fixes)

    next_tracker = subparsers.add_parser(
        'next-tracker',
        help="List newer branches' tracker items citing this bump's changes.",
    )
    next_tracker.add_argument('range', help='e.g. 158.0.8127.2..158.0.8127.4')
    next_tracker.set_defaults(func=_cmd_next_tracker)

    pick_fix = subparsers.add_parser(
        'pick-fix',
        help="Cherry-pick a newer branch's fix, citing this branch's culprit.",
    )
    pick_fix.add_argument(
        'commit', help='The fix, as next-fixes or next-tracker lists it.'
    )
    pick_fix.add_argument(
        'culprit',
        nargs='?',
        help="The cherry-pick in this branch's bump range the fix is for. "
        "Leave it out for a ticked tracker item's fix.",
    )
    pick_fix.set_defaults(func=_cmd_pick_fix)

    verify = subparsers.add_parser(
        'verify-culprits',
        help='Check the culprit hashes cited by the lift commits.',
    )
    verify.add_argument('base', help='e.g. origin/cr157')
    verify.add_argument('range', help='e.g. 158.0.8117.1..158.0.8127.2')
    verify.set_defaults(func=_cmd_verify_culprits)

    args = parser.parse_args()
    return args.func(args)


if __name__ == '__main__':
    sys.exit(main())
