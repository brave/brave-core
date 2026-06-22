#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Bootstrap depot_tools and forward to a Python script under vpython3.

Designed for easy deployment of vpython3, specially in CI environments.
The script:

  1. Locates or clones `depot_tools`.
  2. Prepends it to `PATH` and triggers its self-bootstrap so that `vpython3`
     and its on-demand resources are ready.
  3. Resolves the target Python script named by `--run`: an `http(s)`
     URL is downloaded into a temp directory, while a local filesystem
     path is used in place.
  4. Invokes it under `depot_tools/vpython3`, forwarding any arguments that
     follow `--` verbatim, and propagates its exit code.

Keep this script standalone, single file, Python standard library only script,
so it can run straight from `curl | python3 -` on a fresh CI worker, before
any project checkout exists.

Example:

```sh
curl -sSLf \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/bootstrap_depot_tools.py \
    | python3 - \
        --run=https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/toolchains/build_rust_toolchain.py \
        -- \
        --out-dir=./out \
        --chromium-src=chromium/src \
        --clone-chromium \
        --use-ref=150.0.7850.1
```
"""

from __future__ import annotations

import argparse
import contextlib
import logging
import os
import platform
import shlex
import shutil
import subprocess
import sys
import tempfile
import urllib.parse
import urllib.request
from collections.abc import Iterator
from pathlib import Path

# Latest Chromium depot_tools bundle.
DEPOT_TOOLS_URL = 'https://chromium.googlesource.com/chromium/tools/depot_tools'

# Name of the vpython3 launcher shipped inside depot_tools.
VPYTHON_NAME = 'vpython3.bat' if sys.platform == 'win32' else 'vpython3'

# Target script whose invocation is wrapped in a PTY on Linux (see
# _maybe_wrap_pty). Gated on purpose so this experimental workaround stays
# limited to the build that needs it and never affects other callers of this
# generic bootstrap.
RUST_TOOLCHAIN_RUNNER = 'build_rust_toolchain.py'


def _check_call(*command: str, cwd: Path | None = None) -> None:
    """Run *command* synchronously, logging the invocation.
    """
    logging.info(' >>>> %s', ' '.join(str(a) for a in command))

    if platform.system() == 'Windows':
        resolved = shutil.which(command[0])
        if resolved is None:
            raise RuntimeError(f'Command not found: {command[0]}')
        if resolved != command[0]:
            command = (resolved, *command[1:])

    subprocess.run(command, cwd=cwd, check=True)


def _bootstrap_depot_tools(depot_tools_dir: Path) -> Path:
    """Ensure depot_tools is installed and reachable; return the install dir.

    Resolution order:
      1. If `gclient` is already on PATH, use that install location.
      2. If `depot_tools_dir` already contains a `gclient`, reuse it.
      3. Otherwise clone `depot_tools` into `depot_tools_dir`.

    The chosen install directory is prepended to PATH, then a no-op
    `gclient` invocation is issued so that depot_tools can self-update and
    lay down its bundled Python interpreter before the target script runs.
    """
    existing = shutil.which('gclient')
    if existing is not None:
        install = Path(existing).resolve().parent
        logging.info('depot_tools already on PATH at %s', install)
    elif (depot_tools_dir / 'gclient').is_file():
        install = depot_tools_dir.resolve()
        logging.info('Reusing depot_tools at %s', install)
    else:
        install = depot_tools_dir.resolve()
        logging.info('Cloning depot_tools into %s', install)
        install.parent.mkdir(parents=True, exist_ok=True)
        _check_call('git', 'clone', DEPOT_TOOLS_URL, str(install))

    os.environ['PATH'] = os.pathsep.join([str(install), os.environ['PATH']])

    # A no-arg gclient call triggers depot_tools' self-bootstrap, which
    # downloads vpython3's bundled Python and other on-demand resources.
    _check_call('gclient')

    return install


@contextlib.contextmanager
def _resolve_script(script: str) -> Iterator[Path]:
    """Yield a local Path to *script*, downloading it via HTTP(S) if needed.

    Strings with an `http` / `https` scheme are fetched into a temporary
    directory that is removed when the context manager exits. Anything else
    is treated as a local filesystem path and yielded in place.
    """
    parsed = urllib.parse.urlparse(script)
    if parsed.scheme in ('http', 'https'):
        with tempfile.TemporaryDirectory(
                prefix='bootstrap_depot_tools_') as tmp:
            name = Path(parsed.path).name or 'script.py'
            dest = Path(tmp) / name
            logging.info('Downloading %s -> %s', script, dest)
            with urllib.request.urlopen(script) as response:
                dest.write_bytes(response.read())
            yield dest
        return

    local = Path(script).expanduser().resolve()
    if not local.is_file():
        raise RuntimeError(f'Script not found: {local}')
    yield local


def _maybe_wrap_pty(cmd: tuple[str, ...],
                    script_path: Path) -> tuple[str, ...]:
    """Re-exec *cmd* under a pseudo-tty via util-linux `script`, when gated.

    Experimental workaround, intentionally gated to Linux *and* the rust
    toolchain runner (`RUST_TOOLCHAIN_RUNNER`) so its blast radius stays
    limited to the build that needs it -- this keeps the test limited and
    leaves every other caller of this generic bootstrap untouched.

    Some upstream `library/std` process/session tests (e.g.
    `sys::process::unix::common::tests::test_setsid_*`) hang for a very long
    time when run without a controlling terminal -- e.g. under a cron-launched,
    TTY-less Jenkins swarm agent with stdin wired to /dev/null. Allocating a
    PTY with `script` makes the environment resemble an interactive shell
    closely enough to avoid the hang.

    Outside the gate, or when `script` is unavailable, *cmd* is returned
    unchanged so behaviour is never worse than today.
    """
    if platform.system() != 'Linux':
        return cmd
    if script_path.name != RUST_TOOLCHAIN_RUNNER:
        return cmd
    script_bin = shutil.which('script')
    if script_bin is None:
        logging.warning('PTY workaround skipped: `script` not found on PATH.')
        return cmd
    logging.info('Wrapping %s in a PTY via `script` (Linux workaround).',
                 RUST_TOOLCHAIN_RUNNER)
    inner = ' '.join(shlex.quote(a) for a in cmd)
    # util-linux `script`: -q quiet, -e return the child's exit code (so build
    # failures still propagate), -f flush output after each write (keeps CI log
    # streaming responsive), -c run the given command. The typescript itself is
    # discarded to /dev/null.
    return (script_bin, '-qefc', inner, '/dev/null')


def _split_argv(argv: list[str]) -> tuple[list[str], list[str]]:
    """Split *argv* on the first `--`.

    This bypasses argparse for forwarded arguments so that they cannot
    accidentally collide with this script's own options (e.g. a forwarded
    `--verbose` going to the target script instead of being claimed here).
    """
    if '--' in argv:
        idx = argv.index('--')
        return argv[:idx], argv[idx + 1:]
    return argv, []


def main() -> int:
    own_args, forwarded = _split_argv(sys.argv[1:])

    parser = argparse.ArgumentParser(
        description='Bootstrap depot_tools and forward to a Python script '
        'under vpython3. Arguments after `--` are forwarded verbatim to '
        'the target script.')
    parser.add_argument(
        '--run',
        required=True,
        help='URL (http/https) or local path to the Python script to run '
        'under vpython3 once depot_tools is ready.')
    parser.add_argument(
        '--depot-tools-dir',
        default='./depot_tools',
        help='Directory to install depot_tools into when no existing copy '
        'is found (default: ./depot_tools).')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose (debug) logging.')
    args = parser.parse_args(own_args)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        format='%(message)s')

    install = _bootstrap_depot_tools(Path(args.depot_tools_dir).expanduser())

    vpython = install / VPYTHON_NAME
    if not vpython.exists():
        raise RuntimeError(f'vpython3 not found at {vpython}')

    with _resolve_script(args.run) as script_path:
        cmd = (str(vpython), str(script_path), *forwarded)
        cmd = _maybe_wrap_pty(cmd, script_path)
        logging.info(' >>>> %s', ' '.join(cmd))
        result = subprocess.run(cmd, check=False)
        return result.returncode


if __name__ == '__main__':
    sys.exit(main())
