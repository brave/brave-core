#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Reads and reruns a pull request's CI, and times a job's runs.

Subcommands:
    checks PR
        Lists the pull request's checks (PR is a number or a branch), with
        their state and CI system.
    failures PR [--output FILE]
        Reports what failed behind each failed check: the failed builds,
        their problems, failed tests, and the end of the failed step's log.
    log URL --output FILE [--no-timestamps] [--failed-step]
        Writes the whole log of the build at URL (a check's link, or a build
        `failures` lists) to FILE, for what the report's log tail cuts.
        `--no-timestamps` leaves out its lines' timestamps. For a TeamCity
        build, `--failed-step` writes only the end of the step that failed,
        or each failed test's whole output when only tests failed, read
        without downloading the whole log.
    rerun URL [--wipe-workspace]
        Queues the build at URL (a check's link) again. For TeamCity, an
        aggregate reruns its failed dependencies; for GitHub Actions, the job
        reruns with the jobs that depend on it. `--wipe-workspace` is for
        Jenkins only.
    builds URL [--count N]
        Lists the newest runs of a Jenkins job (URL is the job's link or a
        run's): number, start, result and duration.
    timings URL [--match REGEX] [--longest N]
        Lists the commands the build at URL ran, from its timestamped log,
        with when each started and how long it took: those matching REGEX,
        or the N longest.
    trend URL --match REGEX [--count N] [--every K]
        How long the commands matching REGEX took in each of a Jenkins job's
        newest N finished runs (every Kth), oldest first, to see when a
        step got slower.

Jenkins reads `~/.jenkins.json`; TeamCity goes through the `teamcity` CLI and
its login; GitHub Actions goes through `gh` and its login.
"""

from __future__ import annotations

import argparse
import datetime
from pathlib import Path
import sys

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import github, report, systems, timings  # noqa: E402  -- after _boot


def _cmd_checks(args: argparse.Namespace) -> int:
    checks = github.pr_checks(args.pr)
    print(f'PR {checks.number} ({checks.branch}) at {checks.head}')
    for check in checks.checks:
        print(f'{check.state:8} {check.system:10} {check.name}  {check.url}')
    return 0


def _cmd_failures(args: argparse.Namespace) -> int:
    found = report.of(args.pr)
    if args.output:
        args.output.write_text(found.markdown(), encoding='utf-8', newline='')
        print('\n'.join(found.summary()))
        print(f'Written to {args.output}')
    else:
        print(found.markdown())
    return 0


def _cmd_log(args: argparse.Namespace) -> int:
    if args.failed_step:
        log = systems.failed_step_log(args.url)
    else:
        log = systems.full_log(args.url, timestamps=args.timestamps)
    args.output.write_text(log, encoding='utf-8', newline='')
    print(f'{len(log.splitlines())} lines written to {args.output}')
    return 0


def _cmd_rerun(args: argparse.Namespace) -> int:
    print(systems.rerun(args.url, wipe_workspace=args.wipe_workspace))
    return 0


# How much of a command's line `timings` prints.
_COMMAND_WIDTH = 200


def _when(started: datetime.datetime) -> str:
    return f'{started:%a %d %b %H:%M}'


def _cmd_builds(args: argparse.Namespace) -> int:
    for build in systems.builds(args.url, args.count):
        print(
            f'#{build.number:<6} {_when(build.started)}  '
            f'{build.result or "RUNNING":8} {build.duration:>7}  {build.url}'
        )
    return 0


def _cmd_timings(args: argparse.Namespace) -> int:
    log = systems.full_log(args.url, timestamps=True)
    found = (
        timings.matching(log, args.match)
        if args.match
        else timings.commands(log)
    )
    if args.longest:
        found = sorted(found, key=lambda command: command.seconds, reverse=True)
        found = found[: args.longest]
    if not found:
        print('No timestamped commands found.')
    for command in found:
        print(
            f'{command.started:%H:%M:%S} '
            f'{timings.format_seconds(command.seconds):>7}  '
            f'{command.line[:_COMMAND_WIDTH]}'
        )
    return 0


def _cmd_trend(args: argparse.Namespace) -> int:
    finished = [b for b in systems.builds(args.url, args.count) if b.result]
    for build in reversed(finished[:: args.every]):
        found = timings.matching(
            systems.full_log(build.url, timestamps=True), args.match
        )
        durations = ', '.join(
            timings.format_seconds(command.seconds) for command in found
        )
        print(
            f'#{build.number:<6} {_when(build.started)}  {build.result:8} '
            f'{durations or "-"}',
            flush=True,
        )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    subparsers = parser.add_subparsers(dest='command', required=True)
    checks = subparsers.add_parser('checks', help="List a PR's checks.")
    checks.add_argument('pr')
    checks.set_defaults(func=_cmd_checks)
    found = subparsers.add_parser(
        'failures', help='Report what failed behind the failed checks.'
    )
    found.add_argument('pr')
    found.add_argument('--output', type=Path)
    found.set_defaults(func=_cmd_failures)
    log = subparsers.add_parser('log', help="Write a build's whole log.")
    log.add_argument('url')
    log.add_argument('--output', type=Path, required=True)
    log.add_argument(
        '--timestamps',
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Keep the lines' timestamps, which tell when things happened "
        '(--no-timestamps leaves them out, to save context).',
    )
    log.add_argument(
        '--failed-step',
        action='store_true',
        help="Only the end of the step that failed (or, when only tests "
        "failed, each failed test's whole output), without the whole log, "
        'which a TeamCity build with thousands of tests takes many minutes '
        'to download (TeamCity only).',
    )
    log.set_defaults(func=_cmd_log)
    rerun = subparsers.add_parser('rerun', help='Queue a build again.')
    rerun.add_argument('url')
    rerun.add_argument('--wipe-workspace', action='store_true')
    rerun.set_defaults(func=_cmd_rerun)
    builds = subparsers.add_parser('builds', help="List a job's newest runs.")
    builds.add_argument('url')
    builds.add_argument('--count', type=int, default=30)
    builds.set_defaults(func=_cmd_builds)
    timed = subparsers.add_parser(
        'timings', help='How long the commands of a build took.'
    )
    timed.add_argument('url')
    timed.add_argument(
        '--match', help='Only the commands whose line matches this regex.'
    )
    timed.add_argument(
        '--longest', type=int, help='Only the N longest, longest first.'
    )
    timed.set_defaults(func=_cmd_timings)
    trend = subparsers.add_parser(
        'trend', help="How long a command took across a job's runs."
    )
    trend.add_argument('url')
    trend.add_argument(
        '--match', required=True, help='The commands to time, as a regex.'
    )
    trend.add_argument(
        '--count', type=int, default=20, help='How many newest runs to read.'
    )
    trend.add_argument(
        '--every',
        type=int,
        default=1,
        help='Read every Kth run, to cover more time with fewer logs.',
    )
    trend.set_defaults(func=_cmd_trend)
    args = parser.parse_args()
    try:
        return args.func(args)
    except systems.Unsupported as e:
        print(e)
        return 1


if __name__ == '__main__':
    sys.exit(main())
