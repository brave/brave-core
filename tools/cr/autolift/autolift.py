#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Helpers for the autonomous Chromium lift in tools/cr/autolift/SKILL.md.

Subcommands:
    prepare N [--no-rebase] [--force] [--always]
        Resets the local cr<N> branch to origin/cr<N>, sets its upstream to the
        base (origin/cr<N-1>, or origin/master once master is on milestone N-1),
        picks the lift target, and starts `brockit rebase --squash-minor-bumps`,
        with `--discard-regen-changes` when there will be a lift, or when the
        base got a bump since the branch was last rebased (for `brockit regen`
        to remake them after the rebase). The target is
        `@latest-for-branch`, until origin/cr<N+1> exists; then it is the
        newest beta or stable release of M<N> newer than cr<N>, if there is
        one. For a new milestone with no origin/cr<N> yet, creates cr<N> from
        the base instead, and leaves the rebase out. With no lift, and 20 base
        commits or fewer to rebase over within 12 hours of the last rebase, it
        exits 3 before rebasing, as a lift run isn't worth it (the run turns to
        an autofix run instead), unless `--always` or the branch conflicts
        with the base.
    plan N
        Says, without changing anything, whether a run of the checked-out
        cr<N> would lift, only rebase, or isn't worth it (exit 3), as
        `prepare` would decide. The recipe syncs the tooling only for a run
        that lifts or rebases.
    lift [--continue]
        Runs `brockit lift --with-github` (in infra mode) to the target
        `prepare` picked for the current cr<N> branch, keeps the log under
        .autolift/cr<N>/, and prints and appends to the run's report the
        bump range and what needs attention. Exits non-zero if anything does,
        even when Brockit itself exits 0, and when there is no lift to do.
    publish N [--built] [--no-rebase [--allow-new]]
        At the very end of a run that finished with no issues: pushes cr<N>,
        keeping the base as its upstream. It refreshes CI's git cache, when
        there is one, so that `origin` is as recent as GitHub. First, it
        cherry-picks what landed on cr<N> since the run started, fetches the
        latest base (once per run), and runs
        `brockit rebase --squash-minor-bumps` if the base moved or picked
        fixups need squashing. If that changed the
        branch, it exits 2: rebuild, then run `publish --built` to push. A new
        milestone is pushed, and gets its version issue and pull request
        against the base from `brockit update-version-issue`. With
        `--no-rebase`, for an autofix run, it only picks what landed (and asks
        for a rebuild if that changed the branch), and refuses to push the
        run's commits unless each is a fixup, has a pinned subject, or is
        tooling `sync_tooling.py` picked (any commit with `--allow-new`). With
        no fixes of the run's own, it does nothing.
    autofix N
        Starts an autofix run: resets cr<N> to origin/cr<N>, with no rebase,
        keeping the tooling `sync_tooling.py` picked, sets the base as its
        upstream, and syncs the checkout if its Chromium version doesn't match
        the branch's.
    ci-failures N
        Reports what failed behind each failed check of cr<N>'s pull request,
        in .autolift/cr<N>/ci-failures.md, and appends a summary to the
        report. Exits 2 if any check failed, 3 if only some are pending.
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
        branch's tools/cr/codehealth/M<M>.md that cite the origin of an
        upstream cherry-pick in RANGE, or name a file one of them changed,
        with each item's state and, for ticked ones, the newer branch's fix.
    pick-fix COMMIT [CULPRIT]
        Cherry-picks COMMIT, a fix next-fixes or next-tracker found on a newer
        branch, with its message as it is. With CULPRIT, a cherry-pick in this
        branch's range, it first checks that the fix cites its origin.
    status [N]
        Where the run is: the branch against origin, operations in progress,
        uncommitted changes, running builds, the lift target, the steps the
        report records since the last `prepare`, and the step to carry on
        with.
    step N (--done OUTCOME | --skipped WHY)
        Records step N as done or skipped, as a `## Step N` report heading.
"""

from __future__ import annotations

import argparse
from collections.abc import Callable, Mapping, Sequence
import os
import dataclasses
from dataclasses import dataclass
from datetime import datetime, timedelta
import fnmatch
import json
from pathlib import Path
import re
import subprocess
import sys

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import failures as ci_failures  # noqa: E402  -- after _boot
from ci import github as ci_github  # noqa: E402  -- after _boot
from ci import report as ci_report  # noqa: E402  -- after _boot
from exceptions import RebaseNeededException  # noqa: E402  -- after _boot
import rebase as brockit_rebase  # noqa: E402  -- after _boot
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

# `brockit lift`'s continuation file, which `lift --continue` resumes from.
_LIFT_CONTINUATION = '.version_upgrade'

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
_GN_NO_EFFECT = re.compile(r'The variable "(\w+)" was set as a build argument')
# Where the build's own output starts: after siso's banner, or at `gn gen`'s
# when the build stopped before siso ran. What precedes it is `pnpm run build`
# setup.
_SISO_BANNER = 'in another terminal to see build status.'
_GN_GEN_START = 'generate ninja files...'
_OUTPUT_COLUMNS = 300
_OUTPUT_HEAD = 150
_OUTPUT_TAIL = 30
# Leading progress dots, but not the dots of a relative path such as ../../a.
_PROGRESS_DOTS = re.compile(r'^\.+(?=[^./])')

# Each pattern captures the part of an error line worth deduplicating on. Paths
# and line numbers are kept for compile errors so that each failing location is
# reported once.
_ERROR_PATTERNS = (
    # clang: ../../foo/bar.cc:12:3: error: message (or fatal error:)
    re.compile(r'^(\S+:\d+:\d+: (?:fatal )?error: .*)$'),
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
    # Build args GN warned have no effect, even on a successful build.
    no_effect_args: list[str]
    # The build's own output when it didn't succeed, trimmed (see
    # `_build_output`); empty otherwise.
    output: list[str]

    @property
    def succeeded(self) -> bool:
        return self.status is not None and (
            self.status.startswith('Build Succeeded')
            or self.status == 'The build has finished successfully.'
        )


def _build_output(lines: list[str]) -> list[str]:
    """The lines from where the build's own output starts, made readable.

    Lines over `_OUTPUT_COLUMNS` (echoed compile commands) are cut, repeated
    lines collapsed, and the middle of a long output elided.
    """
    start = 0
    for index, line in enumerate(lines):
        if _SISO_BANNER in line:
            start = index + 1
        elif _GN_GEN_START in line and not start:
            start = index
    output: list[str] = []
    for line in lines[start:]:
        line = _PROGRESS_DOTS.sub('', line).rstrip()
        if len(line) > _OUTPUT_COLUMNS:
            line = (
                f'{line[:_OUTPUT_COLUMNS]}… '
                f'({len(line) - _OUTPUT_COLUMNS} more columns)'
            )
        if output and line == output[-1]:
            continue
        output.append(line)
    if len(output) > _OUTPUT_HEAD + _OUTPUT_TAIL:
        omitted = len(output) - _OUTPUT_HEAD - _OUTPUT_TAIL
        output = (
            output[:_OUTPUT_HEAD]
            + [f'… {omitted} lines omitted; see the log …']
            + output[-_OUTPUT_TAIL:]
        )
    return output


def summarize_build(log: str) -> BuildSummary:
    """Extracts the outcome, failed steps and distinct errors from a log."""
    status = None
    failed_steps = []
    no_effect_args = []
    counts: dict[str, int] = {}
    lines = strip_ansi(log).splitlines()
    for line in lines:
        if match := _GN_NO_EFFECT.search(line):
            if match.group(1) not in no_effect_args:
                no_effect_args.append(match.group(1))
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
    summary = BuildSummary(status, failed_steps, errors, no_effect_args, [])
    if summary.succeeded:
        return summary
    return dataclasses.replace(summary, output=_build_output(lines))


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


# A ticked task's ``(fixed in `<subject>`)``. A subject holding backticks is
# written in a longer run (``fixed in `` [cr159] Pin `kFoo` ``), as in Markdown,
# and may wrap.
_FIXED_IN = re.compile(r'fixed\s+in\s+(`+)\s?((?:(?!\1).)+?)\s?\1(?!`)', re.S)


def fixed_in_subjects(text: str) -> tuple[str, ...]:
    """The subjects the ticked tasks of a tracker item record."""
    return tuple(' '.join(m.group(2).split()) for m in _FIXED_IN.finditer(text))


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
                fixed_in_subjects(body),
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


def chromium_pathspec(path: str, cwd: Path, chromium_root: Path) -> str:
    """`path` as a pathspec relative to the Chromium checkout.

    Absolute paths and paths starting with `./` or `../` are taken relative to
    `cwd`; anything else is already relative to `src/`.
    """
    if not (os.path.isabs(path) or path.startswith(('./', '../'))):
        return path
    absolute = os.path.normpath(os.path.join(cwd, path))
    relative = os.path.relpath(absolute, os.path.normpath(chromium_root))
    if relative == '..' or relative.startswith('..' + os.sep):
        raise ValueError(f'{path} is outside the Chromium checkout')
    return Path(relative).as_posix()


def _split_range(revision_range: str) -> tuple[str, str]:
    old, sep, new = revision_range.partition('..')
    if not sep or not old or not new:
        raise ValueError(f'Expected OLD..NEW, got {revision_range!r}')
    return old, new


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


# Without a lift, a rebase over this many base commits or fewer isn't worth a
# run, unless the branch was last rebased longer than `_STALE_REBASE` ago, or it
# conflicts with the base.
_MAX_IDLE_BASE_COMMITS = 20
_STALE_REBASE = timedelta(hours=12)
# `prepare`'s exit status when the run isn't worth it.
NOT_WORTH_A_RUN = 3


def not_worth_a_run(
    lift: bool,
    base_commits: int,
    since_rebase: timedelta,
    conflicts: Sequence[str] = (),
) -> str | None:
    """Why a run would be wasted, or None when it's worth it.

    `base_commits` is how many commits the base has that the branch doesn't,
    `since_rebase` how long ago the branch was last rebased, and `conflicts`
    the files the branch conflicts with the base in.
    """
    if lift:
        return None
    if base_commits == 0:
        return 'no lift, and nothing to rebase'
    if conflicts:
        return None
    if base_commits <= _MAX_IDLE_BASE_COMMITS and since_rebase <= _STALE_REBASE:
        hours = since_rebase.total_seconds() / 3600
        return (
            f'no lift, and only {base_commits} base commit(s) to rebase over, '
            f'{hours:.0f}h after the last rebase'
        )
    return None


def _since_rebase(base: str) -> timedelta:
    """How long ago HEAD was last rebased onto `base`: a rebase rewrites
    every commit, so the oldest one's committer date says when, which a
    commit pushed on top later doesn't change."""
    brave = repository.brave
    commits = brave.run_git('rev-list', '--reverse', f'{base}..HEAD').split()
    stamp = brave.run_git(
        'log', '-1', '--format=%ct', commits[0] if commits else 'HEAD'
    )
    return datetime.now() - datetime.fromtimestamp(int(stamp))


def merge_conflicts(base: str) -> list[str]:
    """The files merging HEAD into `base` conflicts in, as the branch's pull
    request shows them on GitHub."""
    try:
        # `--quiet` exits 0 even on conflicts; `--name-only` exits 1 then.
        repository.brave.run_git(
            'merge-tree',
            '--write-tree',
            '--name-only',
            '--no-messages',
            'HEAD',
            base,
        )
    except subprocess.CalledProcessError as e:
        if e.returncode != 1:
            raise
        # The merged tree's id, then one conflicted path per line.
        return e.stdout.strip().splitlines()[1:]
    return []


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


def _lift_stopped_file(milestone: int) -> Path:
    """Where `lift` marks a lift that stopped before Brockit finished it."""
    return run_dir(milestone) / 'lift-stopped'


def _lift_ranges_file(milestone: int) -> Path:
    """Where the run's first `lift` records its bump and branch ranges."""
    return run_dir(milestone) / 'lift-ranges.json'


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


def _synced_tooling(milestone: int, branch: str) -> bool:
    """Whether HEAD is where `sync_tooling.py` left cr<milestone>: on top of
    origin's, with the tooling it cherry-picked from the newer branches."""
    marker = run_dir(milestone) / 'synced-head'
    if not marker.exists():
        return False
    brave = repository.brave
    head = brave.run_git('rev-parse', 'HEAD')
    if marker.read_text(encoding='utf-8').strip() != head:
        return False
    try:
        brave.run_git('merge-base', '--is-ancestor', f'origin/{branch}', 'HEAD')
    except subprocess.CalledProcessError:
        return False
    return True


def _no_lift_plan(base: str, current: Version) -> tuple[bool, str] | None:
    """For a run that only rebases: whether it regenerates, and the summary
    lines saying so. None when the version the branch starts from on the
    base can't be found.

    A base bump brings its own regen commits, which conflict with the
    branch's: the rebase drops the branch's, and `brockit regen` remakes
    them. Without one, the rebase keeps them.
    """
    base_version = Version.from_git(base)
    rebased_on = _rebased_on(base_version, current)
    if rebased_on is None:
        return None
    summary = f'- Branch range: {base_version}..{current}\n'
    if rebased_on != base_version:
        return True, summary + (
            f'- No lift needed, but {base} moved from {rebased_on} to '
            f'{base_version}: rebase discarding the regen changes, then run '
            '`brockit regen`, build Linux only, and stop.'
        )
    return False, summary + (
        '- No lift needed: rebase only, keeping the regen changes. Then sync, '
        'build Linux only, and stop.'
    )


def _cmd_autofix(args: argparse.Namespace) -> int:
    """Starts an autofix run: cr<N> as GitHub has it, with no rebase."""
    brave = repository.brave
    branch = f'cr{args.milestone}'
    if brave.is_rebase_in_progress():
        print('A rebase is in progress; finish or abort it first.')
        return 1
    if brave.run_git('status', '--porcelain', '--untracked-files=no'):
        print(
            f'Tracked files have changes, and resetting {branch} would '
            'discard them. Commit or stash them first.'
        )
        return 1
    if not _remote_branch_exists(branch):
        print(f'{branch} is not on GitHub: there is no CI to fix.')
        return 1
    brave.run_git('fetch', 'origin', 'master', branch)
    # The tooling `sync_tooling.py` picked from the newer branches is the
    # run's own, and what it runs: kept, and pushed with the fixups.
    synced = _synced_tooling(args.milestone, branch)
    if not synced and (unpushed := _unpushed_commits(branch)):
        print(
            f'Local {branch} has {unpushed} commit(s) that origin/{branch} '
            'lacks, and resetting it would drop them. Push or drop them first.'
        )
        return 1
    base = choose_base(args.milestone, Version.from_git('origin/master').major)
    if base != 'origin/master':
        brave.run_git('fetch', 'origin', base.removeprefix('origin/'))
    start = brave.run_git('rev-parse', 'HEAD') if synced else f'origin/{branch}'
    brave.run_git('checkout', '-B', branch, start)
    brave.run_git('branch', f'--set-upstream-to={base}')
    head = brave.run_git('rev-parse', 'HEAD')
    Publish.record_start(
        args.milestone, brave.run_git('rev-parse', f'origin/{branch}')
    )
    _lift_target_file(args.milestone).write_text('none\n', encoding='utf-8')
    where = (
        f'kept on origin/{branch} with the tooling `sync_tooling.py` picked'
        if synced
        else f'reset to origin/{branch}'
    )
    summary = (
        f'- Autofix run: {branch} {where} ({head[:12]}), upstream {base}. No '
        'lift, no rebase; fixes go out with `publish --no-rebase`.'
    )
    mismatch = chromium_mismatch()
    if mismatch:
        summary += f'\n- {mismatch}: synced.'
    print(summary)
    append_report(args.milestone, f'autofix {args.milestone}', summary)
    if mismatch:
        return sync_chromium(args.milestone)
    return 0


class RunPlan:
    """Whether a run of cr<N> lifts, and whether it is worth running at all.

    Read from HEAD, the branch as checked out, and its base, changing
    nothing: `prepare` acts on it, and `plan` only reports it, so that the
    recipe can skip the tooling sync of a run that won't lift or rebase.
    """

    def __init__(self, milestone: int, base: str, exists: bool):
        self.milestone = milestone
        self.base = base
        self.exists = exists
        self.branch = f'cr{milestone}'
        # What `prepare` reports, added to as the plan is made.
        self.summary = ''
        # The `--to` of the lift, or None for none.
        self.lift_to: str | None = None
        # Whether a run without a lift regenerates after its rebase.
        self.regen = False

    def decide_lift(self) -> bool:
        """Sets `lift_to` (and `regen` without a lift). False, having said
        why, when that can't be told."""
        branch = self.branch
        # Once cr<N+1> exists, cr<N> is no longer the head branch, and only
        # follows its milestone's beta and stable releases.
        maintenance = self.exists and _remote_branch_exists(
            f'cr{self.milestone + 1}'
        )
        current = Version.from_git('HEAD')
        if maintenance:
            beta = _chromium_version('@latest-beta')
            stable = _chromium_version('@latest-stable')
            if beta is None or stable is None:
                print(
                    'Could not resolve @latest-beta and @latest-stable, so '
                    f'whether {branch} needs a lift is unknown.'
                )
                return False
            target = maintenance_target(self.milestone, current, [beta, stable])
            self.lift_to = str(target) if target else None
            base_version = Version.from_git(self.base)
            self.summary += (
                f'- Maintenance run: origin/cr{self.milestone + 1} exists, so '
                f'{branch} lifts only to a beta or stable release of '
                f'M{self.milestone}.\n'
                f'- {branch} is on {current}; @latest-beta is {beta}, '
                f'@latest-stable is {stable}.\n'
            )
            if target:
                # The ranges the run will have once lifted, rather than now.
                self.summary += (
                    f'- Lift needed: to {target}.\n'
                    f'- Bump range: {current}..{target}\n'
                    f'- Branch range, once lifted: {base_version}..{target}'
                )
        else:
            self.lift_to = '@latest-for-branch'
            tag = _chromium_version(f'@latest-m{self.milestone}')
            self.summary += (
                f'- Target tag (@latest-m{self.milestone}): '
                f'{tag or "unknown"}.\n'
            )
            # A head branch already on its milestone's latest tag lifts
            # nothing, nor does it lift to a minor bump of its tag (only
            # PATCH moves): not worth a run's hassle until the build number
            # moves too.
            if (
                self.exists
                and tag is not None
                and tag.parts[:3] == current.parts[:3]
            ):
                self.lift_to = None
                if tag != current:
                    self.summary += (
                        f'- {tag} is only a minor bump of {current}: no lift.\n'
                    )
        if self.exists and not self.lift_to:
            plan = _no_lift_plan(self.base, current)
            if plan is None:
                print(f'Could not find the base version {branch} starts from.')
                return False
            self.regen, plan_summary = plan
            self.summary += plan_summary
        return True

    def not_worth(self) -> str | None:
        """Why the run isn't worth it, after `decide_lift`, if it isn't."""
        lifts = bool(self.lift_to)
        base_commits = int(
            repository.brave.run_git(
                'rev-list', '--count', f'HEAD..{self.base}'
            )
        )
        conflicts = (
            merge_conflicts(self.base) if base_commits and not lifts else []
        )
        if conflicts:
            self.summary += (
                f'\n- {self.branch} conflicts with {self.base} in '
                f'{len(conflicts)} file(s), so it is rebased however little it '
                f'is behind: {", ".join(conflicts)}.'
            )
        return not_worth_a_run(
            lifts, base_commits, _since_rebase(self.base), conflicts
        )


def _cmd_plan(args: argparse.Namespace) -> int:
    brave = repository.brave
    branch = f'cr{args.milestone}'
    if not _remote_branch_exists(branch):
        print(f'{branch} is new: its first lift is worth a run.')
        return 0
    brave.run_git('fetch', 'origin', 'master', branch)
    base = choose_base(args.milestone, Version.from_git('origin/master').major)
    if base != 'origin/master':
        brave.run_git('fetch', 'origin', base.removeprefix('origin/'))
    plan = RunPlan(args.milestone, base, exists=True)
    if not plan.decide_lift():
        return 1
    if plan.lift_to:
        print(f'Lift, to {plan.lift_to}.')
        return 0
    if reason := plan.not_worth():
        print(f'Not worth a lift run: {reason}.')
        return NOT_WORTH_A_RUN
    print('Rebase, without a lift.')
    return 0


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
    synced = False
    if exists and not args.force:
        brave.run_git('fetch', 'origin', branch)
        # Commits `sync_tooling.py` just made are the run's own.
        synced = _synced_tooling(args.milestone, branch)
        if not synced and (unpushed := _unpushed_commits(branch)):
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
    if synced:
        start = brave.run_git('rev-parse', 'HEAD')
    brave.run_git('checkout', '-B', branch, start)
    # A stopped run's lift is not this run's to continue.
    (brave.root / _LIFT_CONTINUATION).unlink(missing_ok=True)
    _lift_stopped_file(args.milestone).unlink(missing_ok=True)
    # What `publish` takes as GitHub's tip when the run started.
    Publish.record_start(
        args.milestone,
        brave.run_git('rev-parse', f'origin/{branch}' if synced else 'HEAD'),
    )
    brave.run_git('branch', f'--set-upstream-to={base}')
    upstream = brave.run_git('rev-parse', '--abbrev-ref', '@{upstream}')
    if upstream != base:
        print(f'Upstream is {upstream}, expected {base}.')
        return 1
    if synced:
        picked = brave.run_git('rev-list', '--count', f'origin/{branch}..HEAD')
        where = (
            f'kept on origin/{branch} with the {picked} tooling commit(s) '
            '`sync_tooling.py` picked from the newer branches'
        )
    else:
        where = f'{"reset to" if exists else "created from"} {start}'
    summary = (
        f'- master is on Chromium {master_version}.\n'
        f'- {branch} {where}.\n'
        f'- Upstream (base): {base}.\n'
    )

    plan = RunPlan(args.milestone, base, exists)
    plan.summary = summary
    if not plan.decide_lift():
        return 1
    lift_to, regen = plan.lift_to, plan.regen
    summary = plan.summary.rstrip('\n')
    _lift_target_file(args.milestone).write_text(
        f'{lift_to or "none"}\n', encoding='utf-8'
    )
    if exists and not args.always:
        plan.summary = summary
        reason = plan.not_worth()
        summary = plan.summary
        if reason:
            summary += (
                f'\n- Not worth a lift run: {reason}. No rebase: carry on '
                'with an autofix run of the CI; `--always` runs the lift '
                'anyway.'
            )
            print(summary)
            append_report(args.milestone, f'prepare {args.milestone}', summary)
            return NOT_WORTH_A_RUN
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
    had a lift of its own; and, once a branch it changed has been rebuilt and
    passes the presubmit against its upstream, force-pushes it with a lease.

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
        for name in (
            'synced',
            'base-synced',
            'rebuild-needed',
            'regen-needed',
            'presubmit-passed',
        ):
            publish._state(name).unlink(missing_ok=True)
        publish._state('start').write_text(f'{commit}\n', encoding='utf-8')

    def run(
        self, *, built: bool, rebase: bool = True, allow_new: bool = False
    ) -> int:
        """Publishes the branch. Without `rebase`, for an autofix run, the
        base isn't caught up with and nothing is rebased or regenerated:
        only what landed on GitHub is picked, and only fixups, pinned
        subjects and the tooling `sync_tooling.py` picked are pushed (any
        commit with `allow_new`), for the next `brockit rebase` to squash.
        """
        try:
            return self._run(built=built, rebase=rebase, allow_new=allow_new)
        finally:
            if self._notes:
                append_report(
                    self.milestone,
                    f'publish {self.milestone}',
                    '\n'.join(f'- {note}' for note in self._notes),
                )

    def _own_fixes(self, remote: str, start: str) -> list[tuple[str, str]]:
        """`(sha, subject)` of the run's own commits, those GitHub's `remote`
        has no copy of, other than the tooling `sync_tooling.py` picked."""
        own = self._brave.run_git(
            'log',
            '--reverse',
            '--right-only',
            '--cherry-pick',
            '--format=%H%x00%s',
            f'{remote}...HEAD',
        )
        picked = self._picked_tooling(start)
        commits = (line.split('\x00', 1) for line in own.splitlines())
        return [(sha, subject) for sha, subject in commits if sha not in picked]

    def _refused(
        self, fixes: list[tuple[str, str]], remote: str, allow_new: bool
    ) -> bool:
        """Whether `fixes` include any that a push without a rebase can't
        carry."""
        brave = self._brave
        refused = [
            subject
            for _, subject in fixes
            if not allow_new and not squashes_into_branch(subject)
        ]
        merges = brave.run_git('rev-list', '--merges', f'{remote}..HEAD')
        if not refused and not merges:
            return False
        self._note(
            'Not pushing: without a rebase, only fixups, pinned-subject '
            "commits and the tooling `sync_tooling.py` picked go "
            '(`--allow-new` for a fix with no commit to fix up), and no '
            'merges. Refused:'
        )
        self._notes += [f'  - `{subject}`' for subject in refused]
        self._notes += [f'  - merge {sha[:12]}' for sha in merges.split()]
        return True

    def _not_ready(self) -> str | None:
        """Why the checkout can't be published from, if it can't."""
        if self._brave.current_branch() != self.branch:
            return f'Check out {self.branch} first.'
        if stopped := self._in_progress():
            return f'A {stopped} is in progress; finish it first.'
        if self._brave.run_git('status', '--porcelain', '--untracked-files=no'):
            return 'Tracked files have changes; commit them first.'
        return None

    def _picked_tooling(self, tip: str) -> set[str]:
        """The commits `sync_tooling.py` picked onto `tip` this run, from the
        newer branches: they go with the fixups."""
        marker = self._state('synced-head')
        if not marker.exists():
            return set()
        synced = marker.read_text(encoding='utf-8').strip()
        try:
            self._brave.run_git('merge-base', '--is-ancestor', tip, synced)
            self._brave.run_git('merge-base', '--is-ancestor', synced, 'HEAD')
        except subprocess.CalledProcessError:
            return set()
        return set(self._brave.run_git('rev-list', f'{tip}..{synced}').split())

    def _run(self, *, built: bool, rebase: bool, allow_new: bool) -> int:
        brave = self._brave
        if reason := self._not_ready():
            print(reason)
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

        if not rebase:
            if new:
                print(
                    f'{self.branch} is not on GitHub: publish it with a rebase.'
                )
                return 1
            start_commit = self._state('start').read_text(encoding='utf-8')
            fixes = self._own_fixes(remote, start_commit.strip())
            if not fixes:
                # Nothing of its own to push: no pick, no rebuild, no push.
                self._note(
                    'The autofix run committed no fixes: nothing to publish.'
                )
                return 0
            if self._refused(fixes, remote, allow_new):
                return 1

        # Set before changing anything, so that a pick or rebase that stops,
        # and is finished by hand, still asks for a sync and rebuild.
        head = brave.run_git('rev-parse', 'HEAD')
        rebuild.touch()
        if not new and remote != expected:
            if returncode := self._pick_landed(expected, remote):
                return returncode
            expected = remote
        if rebase:
            self._sync_base()
            if returncode := self._rebase():
                return returncode
        if brave.run_git('rev-parse', 'HEAD') != head:
            self._note(self.REBUILD)
            return self.AGAIN
        rebuild.unlink()

        if not new and head == expected:
            self._note(
                f'{self.branch} is unchanged since the run started; nothing '
                'pushed.'
            )
            return 0
        if not self._presubmit(head):
            return 1
        if new:
            return self._push_new()
        if not self._push(expected):
            return self.AGAIN
        # A later publish in the same run builds on this push.
        self._state('synced').write_text(f'{head}\n', encoding='utf-8')
        self._note(f'Pushed {self.branch} over {expected[:12]}.')
        self._note_for_review(expected)
        return 0

    def _presubmit(self, head: str) -> bool:
        """Whether the presubmit passes against the upstream: over every
        commit the push carries, the picked ones included. Runs once per
        `head`."""
        passed = self._state('presubmit-passed')
        if (
            passed.exists()
            and passed.read_text(encoding='utf-8').strip() == head
        ):
            return True
        upstream = self._brave.run_git(
            'rev-parse', '--abbrev-ref', '@{upstream}'
        )
        log = self._state('presubmit.log')
        with log.open('w', encoding='utf-8') as output:
            returncode = subprocess.run(
                ['pnpm', 'run', 'presubmit', '--base', upstream],
                cwd=self._brave.root,
                stdout=output,
                stderr=subprocess.STDOUT,
                check=False,
            ).returncode
        if returncode:
            self._note(
                f'Not pushing: the presubmit against {upstream} has errors '
                f'(see {log}). Fix them as in step 7, then run `publish` '
                'again.'
            )
            return False
        passed.write_text(f'{head}\n', encoding='utf-8')
        return True

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
        # Recorded before picking: once the picks are done, `remote` is the
        # lease. Not before the check, or a rerun would push over a rewrite.
        self._state('synced').write_text(f'{remote}\n', encoding='utf-8')
        landed = brave.run_git(
            'log', '--reverse', '--format=%h %s', f'{expected}..{remote}'
        )
        self._note(f'Picked what landed on GitHub since {expected[:12]}:')
        self._notes += [f'  - `{line}`' for line in landed.splitlines()]
        try:
            # Brockit's `reassign!`/`drop!` markers are empty to begin with.
            brave.run_git(
                'cherry-pick',
                '--allow-empty',
                '--empty=drop',
                f'{expected}..{remote}',
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
        self._state('synced').write_text(
            f'{self._brave.run_git("rev-parse", "HEAD")}\n', encoding='utf-8'
        )
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
        return brockit_rebase.get_pinned_group_for_subject(
            subject
        ) in cls._BROCKIT_GROUPS or brockit_rebase.is_recyclable(subject)

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
    return Publish(args.milestone).run(
        built=args.built, rebase=args.rebase, allow_new=args.allow_new
    )


_AUTOSQUASH = re.compile(r'(?:fixup|amend|squash)! ')


def squashes_into_branch(subject: str) -> bool:
    """Whether the next `brockit rebase` folds a commit with `subject` into
    one already on the branch: a fixup, or a pinned subject such as
    `IWYU fixes.`."""
    return bool(
        _AUTOSQUASH.match(subject)
        or brockit_rebase.get_pinned_group_for_subject(subject)
    )


# `ci-failures`' exit statuses, besides 0 when every check passed.
CI_FAILED = 2
CI_PENDING = 3

# A failed test's name as CI reports it: TeamCity prefixes the binary
# (`unit_tests: Suite.Test`), and parameterised tests carry an instantiation
# prefix and a parameter suffix (`All/Suite.Test/0`).
_FAILED_TEST_NAME = re.compile(
    r'(?:(?P<binary>\w+): )?(?:\w+/)?(?P<suite>\w+)\.(?P<test>\w+)(?:/\w+)?'
)


@dataclass(frozen=True)
class TestLeads:
    """Where to look before fixing or filtering a test that failed on CI."""

    # As CI reports it.
    name: str
    binary: str | None
    suite: str
    # `<line>: <text>` of the tracker's headings and tasks naming the suite.
    tracker: tuple[str, ...]
    # `<file>:<line>: <entry>` of filter entries naming the suite, or whose
    # pattern matches the test.
    filters: tuple[str, ...]
    # `<file>:<line>: <entry> (base `<class>`)` of filter entries naming a
    # base class of the suite's fixture.
    inherited: tuple[str, ...] = ()
    # `<file>:<line>: <entry> (`<old>` in <hash>)` of filter entries naming a
    # fixture the commit adding the suite removed.
    renamed: tuple[str, ...] = ()
    # `<hash> <subject>` of the commits that changed filters naming the suite.
    history: tuple[str, ...] = ()

    @property
    def any(self) -> bool:
        return bool(
            self.tracker
            or self.filters
            or self.inherited
            or self.renamed
            or self.history
        )


# A gtest pattern quoted in the tracker, e.g. `ActorTaskListBubble*` or
# `-Suite.Test`: its suite part.
_QUOTED_SUITE_PATTERN = re.compile(r'`-?([\w*]*\*[\w*]*)(?:\.[\w*]+)?`')


class _SuiteNamed:
    """Whether a tracker line names a suite: by its name, or by a quoted
    gtest pattern matching it (`ActorTaskListBubble*`)."""

    def __init__(self, suite: str) -> None:
        self._suite = suite
        self._word = re.compile(rf'\b{re.escape(suite)}\b')

    def search(self, line: str) -> bool:
        return bool(self._word.search(line)) or any(
            fnmatch.fnmatchcase(self._suite, pattern)
            for pattern in _QUOTED_SUITE_PATTERN.findall(line)
        )


def _tracker_mentions(tracker: str, suite: str) -> tuple[str, ...]:
    """The tracker's `###` headings, with the tasks under them, that name
    `suite`, as `<line>: <text>`."""
    word = _SuiteNamed(suite)
    mentions = []
    heading: tuple[int, str] | None = None
    heading_named = False
    for number, line in enumerate(tracker.splitlines(), 1):
        if line.startswith('#'):
            heading = (number, line) if line.startswith('### ') else None
            heading_named = bool(heading and word.search(line))
            if heading_named:
                mentions.append(f'{number}: {line}')
            continue
        if not line.lstrip().startswith('- ['):
            continue
        if heading_named or word.search(line):
            if (
                heading
                and not heading_named
                and not any(m.startswith(f'{heading[0]}:') for m in mentions)
            ):
                mentions.append(f'{heading[0]}: {heading[1]}')
            mentions.append(f'{number}: {line.strip()}')
    return tuple(mentions)


def _filter_mentions(
    filters: dict[str, str], suite: str, full_name: str
) -> tuple[str, ...]:
    """The filter entries naming `suite`, or whose gtest pattern matches
    `full_name`, as `<file>:<line>: <entry>`."""
    mentions = []
    for path, text in sorted(filters.items()):
        for number, line in enumerate(text.splitlines(), 1):
            entry = line.strip()
            if not entry.startswith('-'):
                continue
            pattern = entry[1:]
            if suite in pattern or fnmatch.fnmatchcase(full_name, pattern):
                mentions.append(f'{path}:{number}: {entry}')
    return tuple(mentions)


def _filter_entries_on_fixture(
    filters: dict[str, str], fixture: str
) -> list[str]:
    """The filter entries whose suite part matches `fixture`, as
    `<file>:<line>: <entry>`."""
    entries = []
    for path, text in sorted(filters.items()):
        for number, line in enumerate(text.splitlines(), 1):
            entry = line.strip()
            if not entry.startswith('-') or '.' not in entry:
                continue
            suite = entry[1:].split('.', 1)[0].rsplit('/', 1)[-1]
            if suite.strip('*') and fnmatch.fnmatchcase(fixture, suite):
                entries.append(f'{path}:{number}: {entry}')
    return entries


def test_leads(
    names: Sequence[str],
    tracker: str,
    filters: dict[str, str],
    ancestors: Mapping[str, Sequence[str]] | None = None,
    renames: Mapping[str, Sequence[tuple[str, str]]] | None = None,
) -> list[TestLeads]:
    """The tracker items and filter entries concerning each of `names`, the
    failed tests CI reported. `filters` maps each filter file's path to its
    text, `ancestors` each suite to its fixture's base classes, and
    `renames` each suite new in the range to the `(fixture, commit)` pairs of
    the fixtures the commits adding it removed."""
    ancestors = ancestors or {}
    renames = renames or {}
    leads = []
    for name in dict.fromkeys(names):
        match = _FAILED_TEST_NAME.fullmatch(name.strip())
        if not match:
            continue
        suite = match['suite']
        direct = _filter_mentions(filters, suite, f'{suite}.{match["test"]}')
        inherited = [
            f'{entry} (base `{base}`)'
            for base in ancestors.get(suite, ())
            for entry in _filter_entries_on_fixture(filters, base)
            if entry not in direct
        ]
        renamed = [
            f'{entry} (`{old}` in {commit[:12]})'
            for old, commit in renames.get(suite, ())
            for entry in _filter_entries_on_fixture(filters, old)
            if entry not in direct
        ]
        leads.append(
            TestLeads(
                name=name,
                binary=match['binary'],
                suite=suite,
                tracker=_tracker_mentions(tracker, suite),
                filters=direct,
                inherited=tuple(dict.fromkeys(inherited)),
                renamed=tuple(dict.fromkeys(renamed)),
            )
        )
    return leads


_TEST_SOURCES = ('*.cc', '*.h', '*.mm')
_NESTED_TEMPLATE_ARGS = re.compile(r'<[^<>]*>')
_TRAILING_NAME = re.compile(r'(\w+)\s*$')


def fixture_bases(fixture: str, sources: Sequence[str]) -> tuple[str, ...]:
    """The base classes `fixture` derives from in `sources`, unqualified and
    without template arguments: `class X : public ns::Y<T>, public Z {`
    gives `('Y', 'Z')`."""
    head = re.compile(
        rf'\b(?:class|struct)\s+{re.escape(fixture)}\s*(?:final\s*)?:'
        r'([^{;]*)\{'
    )
    bases = []
    for text in sources:
        for match in head.finditer(text):
            listed = match[1]
            while (stripped := _NESTED_TEMPLATE_ARGS.sub('', listed)) != listed:
                listed = stripped
            for part in listed.split(','):
                if name := _TRAILING_NAME.search(part):
                    bases.append(name[1])
    return tuple(dict.fromkeys(bases))


def _has_tag(tag: str) -> bool:
    """Whether the Chromium checkout has `tag`: a maintenance branch's base
    tag may never have been fetched."""
    try:
        repository.chromium.run_git(
            'rev-parse', '--verify', '--quiet', f'refs/tags/{tag}^{{commit}}'
        )
    except subprocess.CalledProcessError:
        return False
    return True


def _defining_sources(ref: str, fixtures: Sequence[str]) -> list[str]:
    """The text, at `ref`, of the Chromium sources defining any of
    `fixtures`."""
    pattern = (
        r'(class|struct)[[:space:]]+('
        + '|'.join(fixtures)
        + r')([^[:alnum:]_]|$)'
    )
    try:
        listed = repository.chromium.run_git(
            'grep', '-l', '-E', pattern, ref, '--', *_TEST_SOURCES
        )
    except subprocess.CalledProcessError:
        return []
    return [
        repository.chromium.run_git('show', path, no_trim=True)
        for path in listed.splitlines()
    ]


def _fixture_ancestors(
    ref: str, suites: Sequence[str], depth: int = 3
) -> dict[str, tuple[str, ...]]:
    """Each suite's fixture's base classes at `ref`, up to `depth` levels."""
    bases: dict[str, tuple[str, ...]] = {}
    pending = list(dict.fromkeys(suites))
    for _ in range(depth):
        if not pending:
            break
        sources = _defining_sources(ref, pending)
        for fixture in pending:
            bases[fixture] = fixture_bases(fixture, sources)
        pending = list(
            dict.fromkeys(
                base
                for fixture in pending
                for base in bases[fixture]
                if base not in bases
            )
        )
    ancestors = {}
    for suite in suites:
        found: dict[str, None] = {}
        stack = list(bases.get(suite, ()))
        while stack:
            base = stack.pop(0)
            if base not in found and base != suite:
                found[base] = None
                stack += bases.get(base, ())
        ancestors[suite] = tuple(found)
    return ancestors


# A name where a fixture goes: `class X`, `TEST_F(X,`,
# `IN_PROC_BROWSER_TEST_P(X,` or `INSTANTIATE_TEST_SUITE_P(Prefix, X,`.
_FIXTURE_NAME = re.compile(
    r'\b(?:class|struct)\s+(\w+)'
    r'|\bINSTANTIATE_\w+\(\s*\w*\s*,\s*(\w+)'
    r'|\b\w*TEST\w*\(\s*(\w+)\s*,'
)


def _fixture_names(line: str) -> set[str]:
    return {
        name for match in _FIXTURE_NAME.findall(line) for name in match if name
    }


def fixture_renames(
    suites: Sequence[str], diffs: Mapping[str, str]
) -> dict[str, tuple[tuple[str, str], ...]]:
    """For each of `suites`, the fixtures that a commit in `diffs` (commit to
    its `-U0` diff) adding the suite also removed."""
    renames: dict[str, list[tuple[str, str]]] = {}
    for commit, diff in diffs.items():
        added: set[str] = set()
        removed: set[str] = set()
        for line in diff.splitlines():
            if line.startswith(('+++', '---')):
                continue
            if line.startswith('+'):
                added.update(_fixture_names(line))
            elif line.startswith('-'):
                removed.update(_fixture_names(line))
        for suite in suites:
            if suite in added:
                renames.setdefault(suite, []).extend(
                    (old, commit) for old in sorted(removed - added)
                )
    return {suite: tuple(pairs) for suite, pairs in renames.items()}


def _fixture_renames(
    old: str, new: str, suites: Sequence[str]
) -> dict[str, tuple[tuple[str, str], ...]]:
    """`fixture_renames` for the suites that don't exist at `old`, over the
    commits in `old..new` adding them."""
    alternation = '(' + '|'.join(suites) + ')'
    try:
        present = set(
            repository.chromium.run_git(
                'grep',
                '-h',
                '-o',
                '-w',
                '-E',
                alternation,
                old,
                '--',
                *_TEST_SOURCES,
            ).splitlines()
        )
    except subprocess.CalledProcessError:
        present = set()
    new_suites = [suite for suite in suites if suite not in present]
    if not new_suites:
        return {}
    commits = repository.chromium.run_git(
        'log',
        '--format=%H',
        '-E',
        f'-G({"|".join(new_suites)})',
        f'{old}..{new}',
        '--',
        *_TEST_SOURCES,
    ).splitlines()
    diffs = {
        commit: repository.chromium.run_git(
            'show', '--format=', '-U0', commit, '--', *_TEST_SOURCES
        )
        for commit in commits
    }
    return fixture_renames(new_suites, diffs)


def _filters_history(suite: str) -> tuple[str, ...]:
    return tuple(
        repository.brave.run_git(
            'log', '-3', '--format=%h %s', f'-S{suite}', '--', 'test/filters'
        ).splitlines()
    )


def _leads_markdown(leads: list[TestLeads], tracker_name: str) -> str:
    lines = ['## Leads for the failed tests', '']
    for lead in leads:
        if not lead.any:
            continue
        lines += [f'### `{lead.name}`', '']
        lines += [f'- {tracker_name}:{entry}' for entry in lead.tracker]
        lines += [f'- Filter `{entry}`' for entry in lead.filters]
        lines += [
            f'- Filter on a base class: {entry}' for entry in lead.inherited
        ]
        lines += [
            f'- Filter on a fixture renamed into it: {entry}'
            for entry in lead.renamed
        ]
        lines += [f'- Filter history: `{entry}`' for entry in lead.history]
        lines.append('')
    if len(lines) == 2:
        lines.append(
            f'No failed test is named in {tracker_name} or the test filters.'
        )
    return '\n'.join(lines) + '\n'


def _collect_leads(milestone: int, found: list) -> list[TestLeads]:
    """`test_leads` for every test that failed in `found`."""
    brave = repository.brave.root
    tracker_path = brave / 'tools' / 'cr' / 'codehealth' / f'M{milestone}.md'
    tracker = (
        tracker_path.read_bytes().decode('utf-8')
        if tracker_path.exists()
        else ''
    )
    filters = {
        path.relative_to(brave).as_posix(): path.read_bytes().decode('utf-8')
        for path in sorted((brave / 'test' / 'filters').glob('*.filter'))
    }
    names = [
        test.name
        for check in found
        for build in check.builds
        for test in build.tests
    ]
    suites = list(
        dict.fromkeys(
            match['suite']
            for name in names
            if (match := _FAILED_TEST_NAME.fullmatch(name.strip()))
        )
    )
    if not suites:
        return []
    old, new = (
        str(Version.from_git('@{upstream}')),
        str(Version.from_git('HEAD')),
    )
    ancestors = _fixture_ancestors(new, suites) if _has_tag(new) else {}
    renames = (
        _fixture_renames(old, new, suites)
        if old != new and _has_tag(old) and _has_tag(new)
        else {}
    )
    return [
        dataclasses.replace(lead, history=_filters_history(lead.suite))
        for lead in test_leads(names, tracker, filters, ancestors, renames)
    ]


def _cmd_ci_failures(args: argparse.Namespace) -> int:
    branch = f'cr{args.milestone}'
    checks = ci_github.pr_checks(branch)
    found = ci_report.collect(checks)
    pending = checks.in_state(ci_github.State.PENDING)
    leads = _collect_leads(args.milestone, found)
    output = run_dir(args.milestone) / 'ci-failures.md'
    text = (
        f'# CI failures of {branch} (PR {checks.number}) at {checks.head}\n\n'
        + ci_failures.to_markdown(found)
    )
    if leads:
        text += '\n' + _leads_markdown(leads, f'M{args.milestone}.md')
    output.write_text(text, encoding='utf-8', newline='')
    lines = [
        f'- PR {checks.number} at {checks.head[:12]}: {len(found)} failed, '
        f'{len(pending)} pending of {len(checks.checks)} checks. Details in '
        f'{output}.'
    ]
    head = repository.brave.run_git('rev-parse', 'HEAD')
    if head != checks.head:
        lines.append(
            f'- The checks ran on {checks.head[:12]}, not HEAD ({head[:12]}).'
        )
    for check in found:
        kinds = ', '.join(f'{b.name}: {b.kind}' for b in check.builds)
        lines.append(
            f'- {check.check}: {kinds or check.error or "no failed builds"}'
        )
    if leads:
        named = [lead.name for lead in leads if lead.any]
        lines.append(
            f'- {len(leads)} failed test(s), {len(named)} named in the tracker '
            'or the test filters (see "Leads" in the details)'
            + (f': {", ".join(named)}' if named else '')
            + '.'
        )
    lines += [f'- Pending: {check.name}' for check in pending]
    summary = '\n'.join(lines)
    print(summary)
    append_report(args.milestone, f'ci-failures {args.milestone}', summary)
    if found:
        return CI_FAILED
    return CI_PENDING if pending else 0


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


# The steps of SKILL.md's Procedure.
STEPS = {
    1: 'Reset and rebase',
    2: 'Lift',
    3: 'Linux build',
    4: 'Linux tests',
    5: 'Lit mangler snapshot',
    6: 'Android',
    7: 'Presubmit',
    8: 'Code health review',
    9: 'Update this document',
    10: 'Wrap up',
    11: 'Publish',
}
# A maintenance run without a lift (SKILL.md's "Maintenance run without a
# lift"): step 2 is skipped, and so is everything only a lift needs.
_NO_LIFT_STEPS = (1, 2, 3, 4, 10, 11)

_RUN_START = re.compile(r'^## prepare \d+ \(', re.M)
_STEP_HEADING = re.compile(r'^## Step (\d+) (done|skipped)\b[^\n]*', re.M)


def _current_run(report: str) -> str:
    """The report from the last `prepare` section on."""
    starts = list(_RUN_START.finditer(report))
    return report[starts[-1].start() :] if starts else report


def run_steps(report: str) -> dict[int, str]:
    """The steps the report's current run recorded, as `done` or `skipped`:
    the `## Step <n>` headings after the last `prepare` section."""
    return {
        int(match.group(1)): match.group(2)
        for match in _STEP_HEADING.finditer(_current_run(report))
    }


def run_not_worth_it(report: str) -> str | None:
    """Why the current run's `prepare` found it not worth a lift run, if it
    did."""
    match = re.search(
        r'Not worth a lift run: ([^\n]*?)\. No rebase', _current_run(report)
    )
    return match.group(1) if match else None


def next_step(steps: dict[int, str], lift: bool) -> int | None:
    """The first step the run still has to take, or None once all are."""
    sequence = STEPS if lift else _NO_LIFT_STEPS
    return next((n for n in sequence if n not in steps), None)


def _cmd_step(args: argparse.Namespace) -> int:
    milestone = _branch_milestone()
    if milestone is None:
        print('Check out a cr<N> branch first.')
        return 1
    if args.number not in STEPS:
        print(
            f'There is no step {args.number}; the steps are 1 to {len(STEPS)}.'
        )
        return 1
    status = 'skipped' if args.skipped is not None else 'done'
    note = args.skipped if args.skipped is not None else args.done
    title = f'Step {args.number} {status}: {STEPS[args.number]}'
    report = append_report(milestone, title, note)
    print(f'Recorded "{title}" in {report}.')
    return 0


@dataclass(frozen=True)
class RunStatus:
    """Where a run is: what `status` gathers for a starting or resumed run."""

    milestone: int
    branch: str
    head: str
    # `(behind, ahead)` of origin/cr<N>, or None when it doesn't exist yet.
    origin: tuple[int, int] | None
    # Operations left in progress: a rebase, a cherry-pick, a `brockit lift`.
    in_progress: list[str]
    uncommitted: int
    builds: list[str]
    # Whether the report has a `prepare` section.
    prepared: bool
    # Why `prepare` found the run not worth a lift, making it an autofix run.
    not_worth_it: str | None
    # `prepare`'s `--to` for `lift`, `none`, or None when not recorded.
    lift_target: str | None
    steps: dict[int, str]
    logs: list[str]


def format_status(status: RunStatus) -> str:
    """The status, ending with the step to carry on with."""
    lines = [
        f'Run of cr{status.milestone}: on {status.branch} at {status.head}'
    ]
    if status.branch != f'cr{status.milestone}':
        lines.append(f'- Not on cr{status.milestone}.')
    if status.origin is None:
        lines.append(f'- origin/cr{status.milestone} doesn\'t exist yet.')
    else:
        behind, ahead = status.origin
        lines.append(
            f'- {ahead} commit(s) ahead of origin/cr{status.milestone}, '
            f'{behind} behind.'
        )
    lines += [f'- In progress: {op}' for op in status.in_progress]
    if status.uncommitted:
        lines.append(f'- {status.uncommitted} uncommitted change(s).')
    lines += [f'- Build running: {build}' for build in status.builds]
    if not status.prepared:
        lines.append('- `prepare` hasn\'t run: start at step 1.')
        return '\n'.join(lines)
    if status.not_worth_it:
        lines.append(
            'Next: the "Autofix run" section, carrying on from where the '
            'report leaves it; `prepare` found nothing worth a lift run '
            f'({status.not_worth_it}).'
        )
        return '\n'.join(lines)
    lift = status.lift_target != 'none'
    if status.lift_target is None:
        lines.append('- Lift target: not recorded (an older `prepare`).')
    elif lift:
        lines.append(f'- Lift target: {status.lift_target}')
    else:
        lines.append('- No lift needed: a maintenance run without a lift.')
    for number, state in sorted(status.steps.items()):
        lines.append(f'- Step {number} {state}: {STEPS[number]}')
    lines += [f'- Log: {log}' for log in status.logs]
    upcoming = next_step(status.steps, lift)
    if status.in_progress:
        lines.append(
            'Next: finish or redo what is in progress, then '
            + (f'step {upcoming}.' if upcoming else 'wrap up.')
        )
    elif upcoming is None:
        lines.append('Next: nothing; every step is recorded.')
    else:
        lines.append(f'Next: step {upcoming}, {STEPS[upcoming]}.')
    return '\n'.join(lines)


def _in_progress(milestone: int) -> list[str]:
    brave = repository.brave
    git_dir = Path(brave.run_git('rev-parse', '--absolute-git-dir'))
    found = []
    if brave.is_rebase_in_progress():
        found.append('a rebase (`brockit.py rebase --continue`)')
    if (git_dir / 'CHERRY_PICK_HEAD').exists():
        found.append('a cherry-pick (`git cherry-pick --continue`)')
    if (git_dir / 'REVERT_HEAD').exists():
        found.append('a revert')
    if _lift_stopped_file(milestone).exists():
        found.append('a `brockit lift` (`autolift.py lift --continue`)')
    return found


def _running_builds() -> list[str]:
    """Builds running in this checkout; another checkout's don't matter."""
    result = subprocess.run(
        ['pgrep', '-af', 'siso|autoninja'],
        capture_output=True,
        text=True,
        check=False,
    )
    src = str(repository.chromium.root.resolve())
    return [line for line in result.stdout.splitlines() if src in line]


def _cmd_status(args: argparse.Namespace) -> int:
    brave = repository.brave
    milestone = args.milestone or _branch_milestone()
    if milestone is None:
        print('Not on a cr<N> branch: pass the milestone.')
        return 1
    branch = brave.current_branch()
    origin = None
    if _remote_branch_exists(f'cr{milestone}'):
        behind, ahead = brave.run_git(
            'rev-list',
            '--left-right',
            '--count',
            f'origin/cr{milestone}...HEAD',
        ).split()
        origin = (int(behind), int(ahead))
    target_file = _lift_target_file(milestone)
    report = run_dir(milestone) / 'report.md'
    report_text = report.read_text(encoding='utf-8') if report.exists() else ''
    logs = sorted(
        run_dir(milestone).glob('*.log'),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    status = RunStatus(
        milestone=milestone,
        branch=branch,
        head=brave.run_git('log', '-1', '--format=%h %s'),
        origin=origin,
        in_progress=_in_progress(milestone),
        uncommitted=len(brave.run_git('status', '--porcelain').splitlines()),
        builds=_running_builds(),
        prepared=bool(_RUN_START.search(report_text)),
        not_worth_it=run_not_worth_it(report_text),
        lift_target=(
            target_file.read_text(encoding='utf-8').strip()
            if target_file.exists()
            else None
        ),
        steps=run_steps(report_text),
        logs=[
            f'{path.name} ({datetime.fromtimestamp(path.stat().st_mtime):%H:%M})'
            for path in logs[:3]
        ],
    )
    print(format_status(status))
    return 0


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


def with_recorded_ranges(
    summary: LiftSummary, recorded: dict[str, str | None]
) -> LiftSummary:
    """Fills the ranges Brockit doesn't print on `--continue`."""
    return dataclasses.replace(
        summary,
        bump_range=summary.bump_range or recorded.get('bump_range'),
        branch_range=summary.branch_range or recorded.get('branch_range'),
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
    cmd = [sys.executable, str(_BROCKIT), 'lift', '--infra-mode']
    # Brockit continues to the tag the lift started with.
    if args.continue_:
        cmd.append('--continue')
    else:
        cmd += ['--with-github', f'--to={lift_to}']
    with log.open('wb') as file:
        returncode = subprocess.run(
            cmd,
            cwd=repository.brave.root,
            stdout=file,
            stderr=subprocess.STDOUT,
            check=False,
        ).returncode
    summary = summarize_lift(log.read_bytes().decode('utf-8', errors='replace'))
    stopped = _lift_stopped_file(milestone)
    if summary.done:
        stopped.unlink(missing_ok=True)
    else:
        stopped.touch()
    ranges_file = _lift_ranges_file(milestone)
    if args.continue_ and ranges_file.exists():
        summary = with_recorded_ranges(
            summary, json.loads(ranges_file.read_text(encoding='utf-8'))
        )
    elif not args.continue_:
        summary = with_package_range(summary, _package_range())
        ranges_file.write_text(
            json.dumps(
                {
                    'bump_range': summary.bump_range,
                    'branch_range': summary.branch_range,
                }
            ),
            encoding='utf-8',
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
    if summary.no_effect_args:
        print(
            'GN build args with no effect (upstream may have removed them): '
            + ', '.join(summary.no_effect_args)
        )
    if summary.output:
        print(f'Build output ({len(summary.output)} lines, setup omitted):')
        for line in summary.output:
            print(f'  {line}')
    return 0


def _cmd_culprits(args: argparse.Namespace) -> int:
    _split_range(args.range)
    cmd = ['log', '--format=%H %s', args.range]
    if args.symbol:
        cmd.append(f'-S{args.symbol}')
    if args.paths:
        try:
            pathspecs = [
                chromium_pathspec(
                    path, Path.cwd(), repository.chromium.root.resolve()
                )
                for path in args.paths
            ]
        except ValueError as error:
            print(f'culprits: {error}', file=sys.stderr)
            return 1
        cmd += ['--', *pathspecs]
    try:
        output = repository.chromium.run_git(*cmd)
    except subprocess.CalledProcessError as error:
        lines = (error.stderr or '').strip().splitlines()
        print(
            f'culprits: {lines[0] if lines else error}',
            file=sys.stderr,
        )
        return 1
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
        major = branch.removeprefix('cr')
        # A branch from before the trackers moved keeps its at the old path.
        for path in (
            f'tools/cr/codehealth/M{major}.md',
            f'tools/cr/M{major}_codehealth.md',
        ):
            try:
                tracker = repository.brave.run_git(
                    'show', f'origin/{branch}:{path}', no_trim=True
                )
                break
            except subprocess.CalledProcessError:
                continue
        else:
            print(f'origin/{branch} has no M{major} tracker.')
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
        '--always',
        action='store_true',
        help='Run even with no lift and only a few base commits to rebase '
        'over, which otherwise stops the run.',
    )
    prepare.add_argument(
        '--force',
        action='store_true',
        help='Reset even if tracked files have changes, or the local branch '
        'has commits origin lacks (discards them).',
    )
    prepare.set_defaults(func=_cmd_prepare)

    plan = subparsers.add_parser(
        'plan',
        help='Say whether a run of cr<N> would lift, rebase, or neither.',
    )
    plan.add_argument('milestone', type=int, help='e.g. 158')
    plan.set_defaults(func=_cmd_plan)

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
    publish.add_argument(
        '--no-rebase',
        dest='rebase',
        action='store_false',
        help='For an autofix run: only pick what landed, without catching up '
        'with the base or rebasing, and push only fixups.',
    )
    publish.add_argument(
        '--allow-new',
        action='store_true',
        help='With --no-rebase, also push commits that are not fixups or '
        'pinned subjects.',
    )
    publish.set_defaults(func=_cmd_publish)

    autofix = subparsers.add_parser(
        'autofix',
        help='Start an autofix run: cr<N> as on GitHub, no lift or rebase.',
    )
    autofix.add_argument('milestone', type=int, help='e.g. 159')
    autofix.set_defaults(func=_cmd_autofix)

    ci_failures_parser = subparsers.add_parser(
        'ci-failures',
        help="Report what failed in cr<N>'s pull request checks.",
    )
    ci_failures_parser.add_argument('milestone', type=int, help='e.g. 159')
    ci_failures_parser.set_defaults(func=_cmd_ci_failures)

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

    status = subparsers.add_parser(
        'status',
        help='Where the run is: git state, builds, steps done, and the next.',
    )
    status.add_argument(
        'milestone', type=int, nargs='?', help='By default, the branch\'s.'
    )
    status.set_defaults(func=_cmd_status)

    step = subparsers.add_parser(
        'step', help='Record a finished or skipped step in the report.'
    )
    step.add_argument('number', type=int, help='The step, 1 to 11.')
    outcome = step.add_mutually_exclusive_group(required=True)
    outcome.add_argument('--done', metavar='OUTCOME', help='What it found.')
    outcome.add_argument('--skipped', metavar='WHY', help='Why it was skipped.')
    step.set_defaults(func=_cmd_step)

    args = parser.parse_args()
    return args.func(args)


if __name__ == '__main__':
    sys.exit(main())
