#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Install / uninstall the brave tool shims on the user's `$PATH`.

Install tool shims (e.g. brockit, plaster) on `$PATH` and dispatch to the
`brave-core` checkout the current directory is in (see `launcher.py`). A single
install therefore serves every checkout on the machine.

Usage::

    vpython3 tools/cr/bootstrap/bootstrap.py install [--shell bash|zsh|fish|all]
    vpython3 tools/cr/bootstrap/bootstrap.py uninstall

On POSIX this edits the shell profile(s): an idempotent, marker-delimited block
in `~/.bashrc` / `~/.zshrc`, or a drop-in file under `~/.config/fish/conf.d/`.
On Windows it edits the user `Path` value under `HKCU\\Environment`. Stdlib-only
so it can run before anything else is set up.
"""

from __future__ import annotations

import argparse
from collections.abc import Callable
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys

# The directory this script lives in — the one we add to `$PATH`.
BOOTSTRAP_DIR = Path(__file__).resolve().parent

# The marker in path used to detect an existing boostrap install.
_INSTALL_MARKER = 'brockit'

# Delimiters used to identify the managed block in rc files.
BEGIN_MARKER = '# >>> brave bootstrap >>>'
END_MARKER = '# <<< brave bootstrap <<<'

# Shells we know how to wire up on POSIX.
SHELLS = ('bash', 'zsh', 'fish')

# The fish drop-in file is owned entirely by this script: its presence is the
# managed state, so uninstall just deletes it.
_FISH_DROP_IN = Path('.config') / 'fish' / 'conf.d' / 'brave-bootstrap.fish'

# -- Pure helpers (unit-tested) ----------------------------------------------


def remove_block(text: str) -> str:
    """Removes our managed block (and any blank lines before it) from `text`.

    Returns `text` unchanged when no block is present.
    """
    pattern = re.compile(
        r'\n*' + re.escape(BEGIN_MARKER) + r'.*?' + re.escape(END_MARKER) +
        r'[^\n]*', re.DOTALL)
    return pattern.sub('', text)


def apply_block(text: str, bootstrap_dir: Path) -> str:
    """Returns `text` with exactly one fresh managed block appended.

    The marker-delimited block prepends `bootstrap_dir` to `$PATH` for
    `~/.bashrc` / `~/.zshrc`. Idempotent: any existing block is stripped first,
    so repeated installs do not accumulate duplicates.
    """
    block = (f'{BEGIN_MARKER}\n'
             f'export PATH="{bootstrap_dir.as_posix()}:$PATH"\n'
             f'{END_MARKER}')
    base = remove_block(text)
    if not base:
        return block + '\n'
    if not base.endswith('\n'):
        base += '\n'
    return f'{base}\n{block}\n'


def fish_drop_in(bootstrap_dir: Path) -> str:
    """Contents of the fish conf.d drop-in file.

    Prepends the directory to `$PATH` (highest precedence) on each new shell,
    dropping any pre-existing occurrence first so it lands exactly once at the
    front. Everything lives in this file: it deliberately avoids
    `fish_add_path`, which would persist the entry in the *universal*
    `fish_user_paths` variable and survive deletion of this file (making
    uninstall ineffective).
    """
    path = bootstrap_dir.as_posix()
    line = f'set -gx PATH "{path}" (string match --invert -- "{path}" $PATH)'
    return ('# Managed by brave bootstrap — do not edit.\n'
            '# Remove with: bootstrap.py uninstall\n'
            f'{line}\n')


def _norm_win(entry: str | Path) -> str:
    """Normalises a Windows PATH entry for case/slash-insensitive compares."""
    return str(entry).rstrip('\\/').lower()


def add_windows_entry(current: str, bootstrap_dir: Path) -> str:
    """Returns `current` (a `;`-joined PATH) with `bootstrap_dir` placed first.

    Any existing occurrence is dropped first, so the directory ends up at the
    front (highest precedence) exactly once. Idempotent.
    """
    target = _norm_win(bootstrap_dir)
    entries = [e for e in current.split(';') if e and _norm_win(e) != target]
    return ';'.join([str(bootstrap_dir), *entries])


def remove_windows_entry(current: str, bootstrap_dir: Path) -> str:
    """Returns `current` (a `;`-joined PATH) with `bootstrap_dir` removed."""
    entries = [
        e for e in current.split(';')
        if e and _norm_win(e) != _norm_win(bootstrap_dir)
    ]
    return ';'.join(entries)


# -- Install detection --------------------------------------------------------


def find_existing_bootstrap() -> str | None:
    """Returns the path of the bootstrap marker shim reachable on `$PATH`.

    Resolves the marker via `shutil.which`, so it reports whatever a
    freshly-spawned shell would actually run — the signal we use to refuse a
    second, conflicting install. Returns None when nothing is found.
    """
    return shutil.which(_INSTALL_MARKER)


def installed_bootstrap_dir() -> Path:
    """The bootstrap directory that is actually on `$PATH`.

    Derived from the live shim located by `find_existing_bootstrap` (its parent
    directory), so `uninstall` targets the install that is really in effect —
    even when run from a different checkout than the one originally installed.
    Falls back to this script's own directory when no shim is on `$PATH`.
    """
    found = find_existing_bootstrap()
    if found:
        return Path(found).resolve().parent
    return BOOTSTRAP_DIR


# -- POSIX install / uninstall ------------------------------------------------


def _profile_path(shell: str) -> Path:
    """The rc file (bash/zsh) or drop-in file (fish) for `shell`."""
    home = Path.home()
    if shell == 'bash':
        return home / '.bashrc'
    if shell == 'zsh':
        return home / '.zshrc'
    if shell == 'fish':
        return home / _FISH_DROP_IN
    raise ValueError(f'unsupported shell: {shell}')


def _read(path: Path) -> str:
    return path.read_bytes().decode('utf-8') if path.exists() else ''


def _write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8', newline='')


def _install_posix(shells: list[str]) -> int:
    for shell in shells:
        path = _profile_path(shell)
        if shell == 'fish':
            _write(path, fish_drop_in(BOOTSTRAP_DIR))
        else:
            _write(path, apply_block(_read(path), BOOTSTRAP_DIR))
        print(f'Installed: {path}')
    print()
    print('Open a new shell, or apply it to the current one now:')
    print(f'  export PATH="{BOOTSTRAP_DIR.as_posix()}:$PATH"  # bash/zsh')
    print(f'  set -gx PATH "{BOOTSTRAP_DIR.as_posix()}" $PATH  # fish')
    return 0


def _erase_fish_user_path(bootstrap_dir: Path) -> bool:
    """Removes `bootstrap_dir` from the universal `fish_user_paths`, if present.

    Older versions of this script wired fish up with `fish_add_path`, which
    persists the entry in the universal `fish_user_paths` variable — deleting
    the conf.d drop-in then left it on `$PATH`. This best-effort cleanup
    scrubs that legacy state. Returns True only if an entry was actually
    removed. No-op when fish isn't installed.
    """
    fish = shutil.which('fish')
    if not fish:
        return False
    target = bootstrap_dir.as_posix()
    try:
        present = subprocess.run(
            [fish, '-c', f'contains -- "{target}" $fish_user_paths'],
            check=False,
            capture_output=True)
        if present.returncode != 0:
            return False
        # Rewrite the universal variable without `target`. Version-independent
        # (avoids `fish_add_path --erase`, which is missing on older fish).
        subprocess.run([
            fish, '-c', f'set -U fish_user_paths '
            f'(string match --invert -- "{target}" $fish_user_paths)'
        ],
                       check=False,
                       capture_output=True)
    except OSError:
        return False
    print(f'Removed from fish_user_paths: {target}')
    return True


def _uninstall_posix() -> int:
    # Symmetric and safe: clean every known location regardless of $SHELL.
    # Target the install actually on $PATH, not necessarily this checkout.
    target_dir = installed_bootstrap_dir()
    removed = False
    for shell in SHELLS:
        path = _profile_path(shell)
        if shell == 'fish':
            if path.exists():
                path.unlink()
                print(f'Removed: {path}')
                removed = True
            if _erase_fish_user_path(target_dir):
                removed = True
            continue
        if not path.exists():
            continue
        original = _read(path)
        stripped = remove_block(original)
        if stripped != original:
            _write(path, stripped)
            print(f'Updated: {path}')
            removed = True
    if not removed:
        print('Nothing to uninstall (no managed entries found).')
    return 0


# -- Windows install / uninstall ----------------------------------------------


def _update_windows_path(transform: Callable[[str], str]) -> int:
    # Deferred: `winreg`/`ctypes` are Windows-only and importing them at module
    # scope would break this script on POSIX.
    import winreg  # pylint: disable=import-outside-toplevel
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, 'Environment', 0,
                        winreg.KEY_READ | winreg.KEY_WRITE) as key:
        try:
            current, value_type = winreg.QueryValueEx(key, 'Path')
        except FileNotFoundError:
            current, value_type = '', winreg.REG_EXPAND_SZ
        updated = transform(current)
        if updated == current:
            print('No change to user Path.')
            return 0
        winreg.SetValueEx(key, 'Path', 0, value_type, updated)

    # Notify running processes so they pick up the change; without this, only
    # newly-spawned shells see the updated PATH. Best-effort.
    try:
        import ctypes  # pylint: disable=import-outside-toplevel
        HWND_BROADCAST = 0xFFFF
        WM_SETTINGCHANGE = 0x001A
        SMTO_ABORTIFHUNG = 0x0002
        ctypes.windll.user32.SendMessageTimeoutW(HWND_BROADCAST,
                                                 WM_SETTINGCHANGE, 0,
                                                 'Environment',
                                                 SMTO_ABORTIFHUNG, 5000, None)
    except Exception:  # pylint: disable=broad-except
        pass

    print(f'Updated user Path under HKCU\\Environment for {BOOTSTRAP_DIR}')
    print('Open a new terminal for the change to take effect.')
    return 0


def _install_windows() -> int:
    return _update_windows_path(
        lambda cur: add_windows_entry(cur, BOOTSTRAP_DIR))


def _uninstall_windows() -> int:
    # Target the install actually on Path, not necessarily this checkout.
    target = installed_bootstrap_dir()
    return _update_windows_path(lambda cur: remove_windows_entry(cur, target))


# -- CLI ----------------------------------------------------------------------


def _default_shell() -> str:
    name = Path(os.environ.get('SHELL', '')).name
    return name if name in SHELLS else 'bash'


def _resolve_shells(selection: str | None) -> list[str]:
    if selection == 'all':
        return list(SHELLS)
    return [selection or _default_shell()]


def main() -> int:
    """Dispatch the install/uninstall subcommand for the host platform."""
    parser = argparse.ArgumentParser(
        description='Install/uninstall the brave tool shims on $PATH.')
    sub = parser.add_subparsers(dest='command', required=True)

    install = sub.add_parser('install', help='Add the shims to $PATH.')
    install.add_argument(
        '--shell',
        choices=[*SHELLS, 'all'],
        default=None,
        help='POSIX shell(s) to configure (default: current $SHELL).')
    install.add_argument(
        '--force',
        action='store_true',
        help='Install even if a bootstrap shim is already on $PATH.')
    sub.add_parser('uninstall', help='Remove the shims from $PATH.')

    args = parser.parse_args()
    is_windows = platform.system() == 'Windows'

    if args.command == 'install':
        existing = find_existing_bootstrap()
        if existing and not args.force:
            sys.stderr.write(
                'A bootstrap shim is already installed and on $PATH:\n'
                f'  {existing}\n'
                'Run "uninstall" first, or pass --force to install anyway.\n')
            return 1
        if is_windows:
            return _install_windows()
        return _install_posix(_resolve_shells(args.shell))
    if args.command == 'uninstall':
        if is_windows:
            return _uninstall_windows()
        return _uninstall_posix()
    return 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
