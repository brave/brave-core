#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""install_xcode_accept_license.py

Installs the `xcode_accept_license.py` helper and its sudoers drop-in on a
macOS. Also doubles as a local verifier: the smoke test uses `sudo -n -l`,
which asks sudo whether the policy would permit the invocation without actually
executing the script, so it never touches
/Library/Preferences/com.apple.dt.Xcode.plist.

USAGE
    # install + verify
    python3 install_xcode_accept_license.py --username $USER

    # check existing install only
    python3 install_xcode_accept_license.py --check-only \\
        --username $USER

    # tear down
    python3 install_xcode_accept_license.py --uninstall

The script re-exec's itself under sudo if not already root, so a single
password prompt is enough. Re-running is safe (idempotent overwrite).
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT_NAME = 'xcode_accept_license.py'
DEFAULT_TARGET = Path('/usr/local/bin') / SCRIPT_NAME
DEFAULT_SUDOERS_PATH = Path('/etc/sudoers.d/xcode_accept_license')
VISUDO = '/usr/sbin/visudo'
SUDO = '/usr/bin/sudo'

# Args used by the smoke test. Both pass the sudoers glob pin and the
# helper's regex check, but because `sudo -n -l` lists the matching policy
# without executing the command, the helper itself is never run and the
# Xcode license plist is not touched.
SMOKE_TEST_ARGS = ['0', '0']


def sudoers_entry(username: str, target: Path) -> str:
    """The single line we write into /etc/sudoers.d/.

    The `[0-9]*` / `[A-Za-z0-9_.-]*` patterns are sudoers globs (not regex)
    pinning the leading character of each argument. Precise input
    validation still lives in the helper script's own regex check.
    """
    return (f'{username} ALL=(root) NOPASSWD: '
            f'{target} [0-9]* [A-Za-z0-9_.-]*\n')


def require_root() -> None:
    if os.geteuid() == 0:  # pylint: disable=no-member
        return
    # Re-exec under sudo. This process is replaced; sudo prompts for a
    # password (or uses cached credentials), then runs us again with euid 0
    # and $SUDO_USER set to the invoking account, so the --username
    # default keeps working transparently.
    print('Elevating via sudo (you may be prompted for your password)...',
          file=sys.stderr)
    os.execv('/usr/bin/sudo', ['/usr/bin/sudo', sys.executable, *sys.argv])


def require_macos() -> None:
    if sys.platform != 'darwin':
        sys.exit(f'Only supported on macOS; got {sys.platform}.')


def find_source(explicit: Path | None) -> Path:
    if explicit is not None:
        if not explicit.is_file():
            sys.exit(f'--source does not exist: {explicit}')
        return explicit.resolve()
    here = Path(__file__).resolve().parent
    candidate = here / SCRIPT_NAME
    if not candidate.is_file():
        sys.exit(f'Cannot locate {SCRIPT_NAME} alongside this installer '
                 f'({candidate}). Pass --source explicitly.')
    return candidate


def install_script(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy(source, target)
    # gid 0 == `wheel` on macOS, matching the docs' `install -o root -g wheel`.
    os.chown(target, 0, 0)  # pylint: disable=no-member
    target.chmod(0o755)
    print(f'  installed {target}  (root:wheel mode 0755)')


def install_sudoers(path: Path, contents: str) -> None:
    """Write the sudoers drop-in atomically.

    Stages the file as `<path>.tmp` so that even if validation fails the
    half-written file is never picked up by sudo. Files under /etc/sudoers.d/
    whose name contains `.` are skipped by sudo's includedir loader, so the
    staging file is invisible to policy while it exists.
    """
    tmp = path.with_suffix(path.suffix + '.tmp')
    tmp.write_bytes(contents.encode('utf-8'))
    os.chown(tmp, 0, 0)  # pylint: disable=no-member
    tmp.chmod(0o440)
    try:
        subprocess.run([VISUDO, '-c', '-f', str(tmp)],
                       check=True,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        tmp.unlink(missing_ok=True)
        sys.exit('visudo rejected the generated sudoers entry:\n' +
                 e.stdout.decode('utf-8', errors='replace'))
    tmp.replace(path)
    print(f'  installed {path}  (root:wheel mode 0440, visudo -c passed)')


def verify_target(target: Path) -> None:
    if not target.is_file():
        sys.exit(f'  MISSING: {target}')
    st = target.stat()
    mode = st.st_mode & 0o777
    if (st.st_uid, mode) != (0, 0o755):
        sys.exit(f'  WRONG ownership/mode for {target}: '
                 f'uid={st.st_uid} mode={oct(mode)} (want uid=0 mode=0o755)')
    print(f'  ok: {target}  (uid={st.st_uid} mode={oct(mode)})')


def verify_sudoers(path: Path) -> None:
    if not path.is_file():
        sys.exit(f'  MISSING: {path}')
    st = path.stat()
    mode = st.st_mode & 0o777
    if (st.st_uid, mode) != (0, 0o440):
        sys.exit(f'  WRONG ownership/mode for {path}: '
                 f'uid={st.st_uid} mode={oct(mode)} (want uid=0 mode=0o440)')
    subprocess.run([VISUDO, '-c', '-f', str(path)],
                   check=True,
                   stdout=subprocess.DEVNULL)
    print(f'  ok: {path}  (uid={st.st_uid} mode={oct(mode)}, '
          f'visudo -c passed)')


def smoke_test(username: str, target: Path) -> None:
    """Verify the sudoers grant permits <username> to run the helper.

    `sudo -n -l <cmd>` only asks sudo's policy engine whether the invocation
    would be permitted; it never executes <cmd>. That keeps this check
    non-destructive — no plist write happens — while still exercising every
    layer: the user identity, the NOPASSWD bit, the absolute path match, and
    the sudoers argument globs.
    """
    args = [
        SUDO, '-u', username, SUDO, '-n', '-l',
        str(target), *SMOKE_TEST_ARGS
    ]
    try:
        result = subprocess.run(args,
                                check=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        sys.exit(
            f'  FAILED: sudo would not let {username} run {target} '
            'without a password.\n'
            f'    stderr: {e.stderr.decode("utf-8", errors="replace").strip()}'
        )
    permitted = result.stdout.decode('utf-8', errors='replace').strip()
    print(f'  ok: sudo policy permits {username!r} to run:')
    print(f'    {permitted}')


def do_install(args: argparse.Namespace) -> int:
    source = find_source(args.source)
    entry = sudoers_entry(args.username, args.target)
    print('Installing:')
    print(f'  source         {source}')
    print(f'  target         {args.target}')
    print(f'  sudoers file   {args.sudoers_file}')
    print(f'  username       {args.username}')
    print(f'  sudoers entry  {entry.strip()}')
    print()
    install_script(source, args.target)
    install_sudoers(args.sudoers_file, entry)
    print()
    print('Verifying:')
    verify_target(args.target)
    verify_sudoers(args.sudoers_file)
    smoke_test(args.username, args.target)
    print()
    print('Done.')
    return 0


def do_uninstall(args: argparse.Namespace) -> int:
    print('Removing:')
    for path in (args.sudoers_file, args.target):
        if path.exists():
            path.unlink()
            print(f'  removed {path}')
        else:
            print(f'  not present: {path}')
    return 0


def do_check_only(args: argparse.Namespace) -> int:
    print('Verifying:')
    verify_target(args.target)
    verify_sudoers(args.sudoers_file)
    smoke_test(args.username, args.target)
    print()
    print('Done.')
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Install / verify / remove the xcode_accept_license.py '
        'helper and its sudoers drop-in on macOS.',
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        '--username',
        default=os.environ.get('SUDO_USER'),
        help='Account that gets the NOPASSWD grant. Defaults to $SUDO_USER '
        '(the account that invoked sudo). Required for install and '
        '--check-only.')
    parser.add_argument('--source',
                        type=Path,
                        help=f'Path to {SCRIPT_NAME} to install. Defaults to '
                        'the file alongside this installer.')
    parser.add_argument('--target',
                        type=Path,
                        default=DEFAULT_TARGET,
                        help=f'Install target (default: {DEFAULT_TARGET}).')
    parser.add_argument(
        '--sudoers-file',
        type=Path,
        default=DEFAULT_SUDOERS_PATH,
        help=f'sudoers drop-in path (default: {DEFAULT_SUDOERS_PATH}).')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--uninstall',
                      action='store_true',
                      help='Remove the helper and the sudoers entry.')
    mode.add_argument(
        '--check-only',
        action='store_true',
        help='Verify an existing install; do not modify anything.')
    args = parser.parse_args()

    require_macos()
    require_root()

    if args.uninstall:
        return do_uninstall(args)
    if not args.username:
        sys.exit('--username is required (no $SUDO_USER fallback '
                 'available).')
    if args.check_only:
        return do_check_only(args)
    return do_install(args)


if __name__ == '__main__':
    sys.exit(main())
