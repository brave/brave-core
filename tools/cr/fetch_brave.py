#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Self-contained bootstrap for a fresh Brave checkout (Linux/macOS/Windows).

Run it from an empty directory that will become the workspace root. The script
is piped straight from GitHub into the system Python:

  Linux/macOS:
    mkdir brave-browser && cd brave-browser
    curl -fsSL https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/fetch_brave.py | python3 -

  Windows (PowerShell):
    mkdir brave-browser; cd brave-browser
    irm https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/fetch_brave.py | python -

It clones brave-core into src/brave, downloads a checkout-local node + npm (so
no system node is required), runs `npm run init` (which syncs Chromium and all
dependencies — and runs `npm install` itself, so there is no separate install
step), then installs the tools/cr/bootstrap shims on your PATH so `node`, `npm`,
`brockit` and `plaster` resolve to this checkout in new shells.

Options may be passed after `-` (e.g. `… | python3 - --ref my-branch`) or via
environment variables: BRAVE_CORE_REPO, BRAVE_CORE_REF, BRAVE_SKIP_BOOTSTRAP.
`--ref` checks out any branch, tag, or commit as a detached HEAD (no local
branch is created), defaulting to master.

Stdlib-only and runs under the system interpreter, so it works before anything
else (depot_tools, vpython) is set up.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

DEFAULT_REPO = 'https://github.com/brave/brave-core.git'
DEFAULT_REF = 'master'

# The checkout always lives here under the workspace root; this matches the
# layout the bootstrap shims resolve against (launcher.find_brave_checkout).
BRAVE_DIR = Path('src') / 'brave'
DOWNLOAD_NODE = BRAVE_DIR / 'third_party' / 'node' / 'download_node.py'
BOOTSTRAP = BRAVE_DIR / 'tools' / 'cr' / 'bootstrap' / 'bootstrap.py'


def log(message: str) -> None:
    print(f'fetch_brave: {message}', flush=True)


def die(message: str) -> None:
    print(f'fetch_brave: error: {message}', file=sys.stderr)
    sys.exit(1)


def run(cmd: list[str], **kwargs) -> None:
    """Runs `cmd`, raising SystemExit with context if it fails."""
    try:
        subprocess.run([str(part) for part in cmd], check=True, **kwargs)
    except subprocess.CalledProcessError as error:
        die(f'command failed ({error.returncode}): {" ".join(map(str, cmd))}')


def capture(cmd: list[str]) -> str:
    """Runs `cmd` and returns its stripped stdout."""
    result = subprocess.run([str(part) for part in cmd],
                            check=True,
                            capture_output=True,
                            text=True)
    return result.stdout.strip()


def checkout_ref(ref: str) -> None:
    """Detaches HEAD at `ref` (a branch, tag, or commit) in the cloned repo.

    A plain clone fetches every remote branch and tag, so `ref` resolves either
    directly (a tag or commit) or as `origin/<ref>` (a remote branch). It is
    checked out detached, so no local branch is created or needs managing.
    """
    for candidate in (ref, f'origin/{ref}'):
        probe = subprocess.run(
            ['git', '-C',
             str(BRAVE_DIR), 'rev-parse', '--verify', '--quiet',
             f'{candidate}^{{commit}}'],
            capture_output=True,
            text=True,
            check=False)
        if probe.returncode == 0:
            run([
                'git', '-C',
                str(BRAVE_DIR), '-c', 'advice.detachedHead=false', 'checkout',
                '--detach', candidate
            ])
            return
    die(f"ref '{ref}' not found in the cloned repository.")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Clone brave-core and set up a fresh checkout.')
    parser.add_argument('--repo',
                        default=os.environ.get('BRAVE_CORE_REPO',
                                               DEFAULT_REPO),
                        help='git URL to clone (default: brave/brave-core).')
    parser.add_argument(
        '--ref',
        default=os.environ.get('BRAVE_CORE_REF', DEFAULT_REF),
        help='branch, tag, or commit to check out as a detached HEAD '
        '(default: master).')
    parser.add_argument(
        '--skip-bootstrap',
        action='store_true',
        default=os.environ.get('BRAVE_SKIP_BOOTSTRAP') == '1',
        help='do not install the node/npm/brockit/plaster shims on PATH.')
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    # Bootstrap prerequisite: git to clone. Python is already running us, and
    # node/npm are what this script provides, so neither is required up front.
    if shutil.which('git') is None:
        die("'git' is required but was not found on PATH.")

    # Refuse to clobber an existing checkout: this is for a clean setup.
    if BRAVE_DIR.exists():
        die(f'{BRAVE_DIR} already exists; run this from an empty workspace '
            f'directory.')

    log(f'cloning {args.repo} into {BRAVE_DIR}')
    run(['git', 'clone', args.repo, BRAVE_DIR])

    # Detach HEAD at the requested ref before touching the repo any further.
    log(f'checking out {args.ref} (detached)')
    checkout_ref(args.ref)

    log('downloading checkout-local node + npm')
    run([sys.executable, DOWNLOAD_NODE])

    # Locate the toolchain via download_node.py so no platform layout is
    # duplicated here. npm is launched as `node npm-cli.js` — portable across
    # POSIX (no shebang reliance) and Windows (no npm.cmd shell invocation).
    node = Path(capture([sys.executable, DOWNLOAD_NODE, '--print', 'node']))
    npm_cli = Path(
        capture([sys.executable, DOWNLOAD_NODE, '--print', 'npm-cli']))
    if not node.exists():
        die(f'node was not found at {node} after download.')

    # Put the downloaded node first on PATH so the many node/npm invocations
    # `npm run init` spawns (including its own inner `npm`) use this toolchain.
    env = os.environ.copy()
    env['PATH'] = str(node.parent) + os.pathsep + env.get('PATH', '')
    log(f'using node {capture([node, "--version"])}')

    log("running 'npm run init' (this clones Chromium and may take a while)")
    run([node, npm_cli, 'run', 'init'], cwd=str(BRAVE_DIR), env=env)

    # Install the shims on PATH so they persist in new shells. bootstrap.py
    # edits the shell profile / user PATH rather than this process, so the shims
    # take effect in the next shell — not this one. If a bootstrap is already
    # installed it exits non-zero; leave it, since its shims dispatch by cwd and
    # so already serve this new checkout.
    if not args.skip_bootstrap:
        log('installing the node/npm/brockit/plaster shims on PATH')
        if subprocess.run([sys.executable, str(BOOTSTRAP), 'install'],
                          check=False).returncode != 0:
            log('a bootstrap is already installed; keeping it (its shims '
                'resolve this checkout by cwd).')

    log(f'done. Your checkout is ready under {BRAVE_DIR}.')
    log('Open a new shell (or re-source your profile) to pick up the shims.')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
