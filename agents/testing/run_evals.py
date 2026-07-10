#!/usr/bin/env python3

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Thin Brave-only runner for the skill-eval suite.

Discovers `*.promptfoo.yaml` under agents/prompts/eval/, runs each the number
of times its metadata asks for, and reports pass/fail using the pass-k rule
(pass = at least `pass_k_threshold` of `runs_per_test` runs pass).

Unlike chromium's `eval_prompts.py`, this does NOT drag in btrfs snapshots or a
container sandbox; it just shells to a pinned promptfoo. The heavy lifting
(driving Claude Code, stubbing gh, harvesting results) lives in
claude_provider.py, which each test wires in as its promptfoo provider.

Usage (from src/brave):
    python3 agents/testing/run_evals.py
    python3 agents/testing/run_evals.py --tag-filter stable
    python3 agents/testing/run_evals.py --list
    # one config directly, bypassing this runner:
    npx promptfoo eval -c agents/prompts/eval/review-prs/CS-003.promptfoo.yaml

Requires: node/npx on PATH, the Claude Code CLI on PATH (or $CLAUDE_BIN), and
whatever auth Claude Code needs for headless runs.
"""

import argparse
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import eval_config  # noqa: E402

_TESTING_DIR = Path(__file__).resolve().parent
_BRAVE_SRC = _TESTING_DIR.parents[1]
_EVAL_ROOT = _BRAVE_SRC / 'agents' / 'prompts' / 'eval'
_SETUP_PY = _BRAVE_SRC / 'agents' / 'skills' / 'setup.py'

# Pin promptfoo for reproducibility. Bump deliberately, never float to ToT.
# Validate the env override — it becomes part of the subprocess argv
# (`promptfoo@<version>`), so restrict it to a version spec / dist-tag rather
# than trusting arbitrary environment data.
import os
import re

_PROMPTFOO_VERSION_RE = re.compile(r'^[A-Za-z0-9][A-Za-z0-9.\-]*$')
PROMPTFOO_VERSION = os.environ.get('PROMPTFOO_VERSION', '0.121.18')
if not _PROMPTFOO_VERSION_RE.match(PROMPTFOO_VERSION):
    raise SystemExit(f'invalid PROMPTFOO_VERSION: {PROMPTFOO_VERSION!r}')


def discover_configs():
    return sorted(_EVAL_ROOT.glob('**/*.promptfoo.yaml'))


def load_tests(paths):
    tests = []
    for p in paths:
        try:
            tests.append(eval_config.TestConfig.from_file(p))
        except ValueError as e:
            print(f'  SKIP (bad config) {p}: {e}', file=sys.stderr)
    return tests


# A full agent run (fetch, chunk, launch subagents, read docs) easily exceeds
# promptfoo's default 5-min python-worker timeout. That worker timeout is
# `config.timeout` (ms) per provider, falling back to REQUEST_TIMEOUT_MS; raise
# the env floor here so no config forgets it. (Not PROMPTFOO_EVAL_TIMEOUT_MS —
# that does NOT control the python worker.)
REQUEST_TIMEOUT_MS = os.environ.get('REQUEST_TIMEOUT_MS', str(40 * 60 * 1000))
if not REQUEST_TIMEOUT_MS.isdigit():
    raise SystemExit(f'invalid REQUEST_TIMEOUT_MS: {REQUEST_TIMEOUT_MS!r}')


def run_one(test):
    """Run one config `runs_per_test` times; return the number that passed.

    The npx argv is built only from a validated version spec and the discovered
    config path (from glob, not the environment); shell=False list form.
    """
    cfg = test.test_file
    env = os.environ.copy()
    env['REQUEST_TIMEOUT_MS'] = REQUEST_TIMEOUT_MS
    passes = 0
    for i in range(test.runs_per_test):
        print(f'    run {i + 1}/{test.runs_per_test} ...', flush=True)
        proc = subprocess.run(
            ['npx', '--yes', f'promptfoo@{PROMPTFOO_VERSION}', 'eval',
             '-c', str(cfg), '--no-cache'],
            cwd=str(cfg.parent), env=env,
        )
        if proc.returncode == 0:
            passes += 1
    return passes


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--tag-filter', default='',
                    help='Comma-separated tags; run only tests carrying one.')
    ap.add_argument('--list', action='store_true',
                    help='List discovered tests and exit.')
    args = ap.parse_args()

    wanted = [t.strip() for t in args.tag_filter.split(',') if t.strip()]
    tests = load_tests(discover_configs())
    tests = [t for t in tests if t.matches_tags(wanted)]
    tests.sort()

    if not tests:
        print('No matching eval configs found.')
        return 0

    if args.list:
        for t in tests:
            print(f'{t.src_relative_test_file}  '
                  f'(runs={t.runs_per_test} pass_k={t.pass_k_threshold} '
                  f'tags={t.tags})')
        return 0

    # Make sure skills are discoverable before any provider run.
    if _SETUP_PY.exists():
        subprocess.run([sys.executable, str(_SETUP_PY), 'link', '-q'],
                       cwd=str(_BRAVE_SRC), check=False)

    results = []
    for t in tests:
        print(f'\n=== {t.src_relative_test_file} '
              f'(pass {t.pass_k_threshold}/{t.runs_per_test}) ===')
        passes = run_one(t)
        ok = passes >= t.pass_k_threshold
        results.append((t, passes, ok))
        print(f'  -> {passes}/{t.runs_per_test} passed '
              f'({"PASS" if ok else "FAIL"})')

    print('\n' + '=' * 60)
    print('Eval summary')
    all_ok = True
    for t, passes, ok in results:
        all_ok = all_ok and ok
        print(f'  [{"PASS" if ok else "FAIL"}] {t.src_relative_test_file} '
              f'({passes}/{t.runs_per_test})')
    print('=' * 60)
    return 0 if all_ok else 1


if __name__ == '__main__':
    sys.exit(main())
