#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Helpers for the autonomous Chromium lift described in tools/cr/autolift/SKILL.md.

Subcommands:
    prepare N [--no-rebase] [--force]
        Resets the local cr<N> branch to origin/cr<N>, sets its upstream to the
        base (origin/cr<N-1>, or origin/master once master is on milestone N-1)
        and starts `brockit rebase --squash-minor-bumps --discard-regen-changes`.
        For a new milestone with no origin/cr<N> yet, creates cr<N> from the
        base instead, and leaves the rebase out.
    lift [--continue]
        Runs `brockit lift --with-github --to=@latest-for-branch` (in infra
        mode) for the current cr<N> branch, keeps the log under
        .autolift/cr<N>/, and prints and appends to the run's report the
        bump range and what needs attention. Exits non-zero if anything does,
        even when Brockit itself exits 0.
    publish N
        For a new milestone only, at the very end of a run whose builds passed:
        pushes the lifted cr<N> to origin, keeping the base as its upstream,
        and runs `brockit update-version-issue`, which creates the version
        issue and a pull request against the base.
    build-summary LOG
        Summarises a `pnpm run build` log: the final status line, the steps
        that failed, and each distinct error with how often it occurred.
    culprits RANGE [--symbol S] [PATH ...]
        Lists the upstream commits in RANGE (e.g. 158.0.8117.1..158.0.8127.2)
        that touch PATH, or that add or remove the string S, with full hashes.
    verify-culprits BASE RANGE
        Checks that every culprit hash cited in the brave-core commits after
        BASE names a real Chromium commit inside RANGE.
"""

from __future__ import annotations

import argparse
import dataclasses
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
import re
import subprocess
import sys

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
import repository  # noqa: E402  -- after _boot
from versioning import Version  # noqa: E402  -- after _boot

_BROCKIT = Path(__file__).resolve().parent.parent / 'brockit.py'

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


def culprit_problem(culprit: str, revision_range: str) -> str | None:
    """Explains why `culprit` isn't a valid culprit, or returns None.

    Short hashes are accepted when they resolve, since teammates sometimes pass
    them; a mistyped or invented hash fails to resolve or falls outside the
    range.
    """
    old, new = _split_range(revision_range)
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


def _target_tag(milestone: int) -> str | None:
    """The tag `brockit lift --to=@latest-for-branch` resolves to for cr<N>."""
    output = subprocess.run(
        [
            sys.executable,
            str(_BROCKIT),
            'show',
            '--chromium-version-label',
            f'@latest-m{milestone}',
        ],
        cwd=repository.brave.root,
        check=False,
        capture_output=True,
        text=True,
    ).stdout.strip()
    return output.rsplit(' ', 1)[-1] if output.startswith('version:') else None


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
    brave.run_git('branch', f'--set-upstream-to={base}')
    upstream = brave.run_git('rev-parse', '--abbrev-ref', '@{upstream}')
    if upstream != base:
        print(f'Upstream is {upstream}, expected {base}.')
        return 1
    target = _target_tag(args.milestone)
    summary = (
        f'- master is on Chromium {master_version}.\n'
        f'- {branch} {"reset to" if exists else "created from"} {start}.\n'
        f'- Upstream (base): {base}.\n'
        f'- Target tag (@latest-m{args.milestone}): {target or "unknown"}.'
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
    print(summary)
    append_report(args.milestone, f'prepare {args.milestone}', summary)

    # A new branch has nothing of its own to rebase yet.
    if args.no_rebase or not exists:
        return 0
    return subprocess.run(
        [
            sys.executable,
            str(_BROCKIT),
            'rebase',
            '--squash-minor-bumps',
            '--discard-regen-changes',
        ],
        cwd=brave.root,
        check=False,
    ).returncode


def _cmd_publish(args: argparse.Namespace) -> int:
    brave = repository.brave
    branch = f'cr{args.milestone}'
    if brave.current_branch() != branch:
        print(f'Check out {branch} first.')
        return 1
    if _remote_branch_exists(branch):
        print(
            f'origin/{branch} already exists. Only a new milestone branch is '
            'pushed; leave this one to the CI job.'
        )
        return 1
    upstream = brave.run_git('rev-parse', '--abbrev-ref', '@{upstream}')
    if int(brave.run_git('rev-list', '--count', f'{upstream}..HEAD')) == 0:
        print(f'{branch} has no commits on top of {upstream}; lift it first.')
        return 1
    # No --set-upstream: the upstream must stay the base, which is also the
    # base of the pull request.
    brave.run_git('push', 'origin', f'{branch}:refs/heads/{branch}')
    return subprocess.run(
        [sys.executable, str(_BROCKIT), 'update-version-issue'],
        cwd=brave.root,
        check=False,
    ).returncode


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
    name = 'lift-continue' if args.continue_ else 'lift'
    stamp = datetime.now().strftime('%Y%m%d-%H%M%S')
    log = run_dir(milestone) / f'{name}-{stamp}.log'
    cmd = [
        sys.executable,
        str(_BROCKIT),
        'lift',
        '--infra-mode',
        '--with-github',
        '--to=@latest-for-branch',
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


def _cmd_verify_culprits(args: argparse.Namespace) -> int:
    output = repository.brave.run_git(
        'log', '--format=%H%x00%s%x00%B%x01', f'{args.base}..HEAD', no_trim=True
    )
    problems = 0
    for entry in output.split('\x01'):
        if not entry.strip():
            continue
        commit, subject, body = entry.strip('\n').split('\x00', 2)
        for culprit in parse_culprit_hashes(body):
            if problem := culprit_problem(culprit, args.range):
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
        help='Reset even if tracked files have changes, or the local branch has '
        'commits origin lacks (discards them).',
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
        help='Push a new milestone branch and open its version issue and PR.',
    )
    publish.add_argument('milestone', type=int, help='e.g. 159')
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
