#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Reads and reruns a pull request's CI.

Subcommands:
    checks PR
        Lists the pull request's checks (PR is a number or a branch), with
        their state and CI system.
    failures PR [--output FILE]
        Reports what failed behind each failed check: the failed builds,
        their problems, failed tests, and the end of the failed step's log.
    log URL --output FILE [--no-timestamps]
        Writes the whole log of the build at URL (a check's link, or a build
        `failures` lists) to FILE, for what the report's log tail cuts.
        `--no-timestamps` leaves out its lines' timestamps.
    rerun URL [--wipe-workspace]
        Queues the build at URL (a check's link) again. For TeamCity, an
        aggregate reruns its failed dependencies; for GitHub Actions, the job
        reruns with the jobs that depend on it. `--wipe-workspace` is for
        Jenkins only.

Jenkins reads `~/.jenkins.json`; TeamCity goes through the `teamcity` CLI and
its login; GitHub Actions goes through `gh` and its login.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import github, report, systems  # noqa: E402  -- after _boot


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
    log = systems.full_log(args.url, timestamps=args.timestamps)
    args.output.write_text(log, encoding='utf-8', newline='')
    print(f'{len(log.splitlines())} lines written to {args.output}')
    return 0


def _cmd_rerun(args: argparse.Namespace) -> int:
    print(systems.rerun(args.url, wipe_workspace=args.wipe_workspace))
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
    log.set_defaults(func=_cmd_log)
    rerun = subparsers.add_parser('rerun', help='Queue a build again.')
    rerun.add_argument('url')
    rerun.add_argument('--wipe-workspace', action='store_true')
    rerun.set_defaults(func=_cmd_rerun)
    args = parser.parse_args()
    try:
        return args.func(args)
    except systems.Unsupported as e:
        print(e)
        return 1


if __name__ == '__main__':
    sys.exit(main())
