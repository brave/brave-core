#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""PATH shim: translate `npm` to `pnpm` when package.json requests pnpm."""

from dataclasses import dataclass
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Optional, Sequence, Tuple

PACKAGE_JSON = 'package.json'
WRAPPER_DIR = Path(__file__).resolve().parent

# npm flags that take a following value when not written as --flag=value.
_FLAGS_WITH_VALUE = frozenset({
    '--prefix',
    '--proxy',
    '--https-proxy',
})

# Global npm flags dropped when translating to pnpm.
_GLOBAL_FLAGS_OMIT = frozenset({
    '--no-fund',
    '--no-audit',
})

# Global npm flags mapped to a different pnpm flag.
_GLOBAL_FLAG_MAP = {
    '--silent': '--reporter=silent',
}

# npm subcommands handled explicitly.
_INSTALL_COMMANDS = frozenset({'install', 'ci'})
_RUN_COMMANDS = frozenset({'run', 'run-script'})
_GLOBAL_INSTALL_FLAGS = frozenset({'-g', '--global'})


@dataclass(frozen=True)
class ParsedNpmArgs:
    prefix: Optional[str]
    pnpm_globals: List[str]
    rest: List[str]
    force_passthrough: bool
    show_version: bool


def main(argv: Optional[Sequence[str]] = None) -> int:
    if argv is None:
        argv = sys.argv[1:]

    parsed = _parse_npm_argv(argv)
    project_dir = resolve_project_dir(parsed.prefix)

    if parsed.force_passthrough:
        return run_real_npm(argv)

    if not should_use_pnpm(project_dir):
        return run_real_npm(argv)

    if parsed.rest and parsed.rest[0] in _INSTALL_COMMANDS:
        maybe_clean_node_modules(project_dir)

    return run_pnpm(argv)


def resolve_project_dir(prefix: Optional[str]) -> str:
    project_dir = Path(prefix) if prefix else Path.cwd()
    if prefix:
        project_dir = project_dir if project_dir.is_absolute() else (
            Path.cwd() / project_dir)
    return os.path.normpath(project_dir)


def run_real_npm(argv: Sequence[str]) -> int:
    cmd = [find_real_npm(), *argv]
    return subprocess.run(cmd, check=False).returncode


def find_real_npm() -> str:
    wrapper_dir = os.path.normcase(os.path.normpath(WRAPPER_DIR))
    path_without_wrapper = os.pathsep.join(
        p for p in os.environ.get('PATH', '').split(os.pathsep)
        if p and os.path.normcase(os.path.normpath(p)) != wrapper_dir)
    npm = shutil.which('npm', path=path_without_wrapper)
    if not npm:
        raise RuntimeError(
            'real npm not found on PATH (wrapper directory excluded)')
    return npm


def should_use_pnpm(project_dir: str) -> bool:
    package_path = Path(project_dir) / PACKAGE_JSON
    try:
        package = json.loads(package_path.read_text(encoding='utf-8'))
    except Exception as e:
        raise RuntimeError(
            f'failed to read package manager name from {package_path}') from e

    try:
        package_manager_name = package['devEngines']['packageManager']['name']
    except KeyError:
        return False

    return package_manager_name == 'pnpm'


def maybe_clean_node_modules(project_dir: str) -> None:
    node_modules = Path(project_dir) / 'node_modules'
    if node_modules.is_dir() and not (node_modules / '.pnpm').is_dir():
        shutil.rmtree(node_modules)


def run_pnpm(argv: Sequence[str]) -> int:
    cmd = translate_to_pnpm(argv)
    print(f'Redirecting to pnpm: {" ".join(cmd)}')
    return subprocess.run(cmd, check=False).returncode


def translate_to_pnpm(argv: Sequence[str]) -> List[str]:
    parsed = _parse_npm_argv(argv)
    pnpm = find_pnpm()

    if parsed.show_version:
        return [pnpm, *parsed.pnpm_globals, '--version']

    if not parsed.rest:
        return [pnpm, *parsed.pnpm_globals]

    subcommand = parsed.rest[0]
    subargs = parsed.rest[1:]
    command = [pnpm, *parsed.pnpm_globals]

    if subcommand in _INSTALL_COMMANDS:
        cmd, install_argv = _translate_install_args(subcommand, subargs)
        return command + [cmd] + install_argv

    if subcommand in _RUN_COMMANDS:
        if not subargs:
            return command + ['run']
        script = subargs[0]
        script_args = _strip_run_separator(subargs[1:])
        return command + ['run', script] + script_args

    if subcommand == 'version':
        return command + ['version'] + list(subargs)

    if subcommand == 'cache' and subargs[:1] == ['clean']:
        extra = [a for a in subargs[1:] if a != '--force']
        return command + ['store', 'prune'] + extra

    if subcommand == 'audit':
        return command + ['audit'] + list(subargs)

    return command + [subcommand] + list(subargs)


def find_pnpm() -> str:
    pnpm = shutil.which('pnpm')
    if not pnpm:
        raise RuntimeError('pnpm not found on PATH')
    return pnpm


def _parse_npm_argv(argv: Sequence[str]) -> ParsedNpmArgs:
    prefix: Optional[str] = None
    pnpm_globals: List[str] = []
    force_passthrough = False
    show_version = False
    i = 0
    argv_list = list(argv)

    while i < len(argv_list):
        arg = argv_list[i]
        flag, inline_value = _split_option_value(arg)

        if flag in _GLOBAL_INSTALL_FLAGS:
            force_passthrough = True
            i += 1
            continue

        if flag in ('--version', '-v'):
            show_version = True
            i += 1
            continue

        if flag in _GLOBAL_FLAGS_OMIT or flag in _GLOBAL_FLAG_MAP:
            _append_mapped_global(flag, inline_value, pnpm_globals)
            i += 1
            continue

        if flag == '--ignore-scripts':
            pnpm_globals.append(arg)
            i += 1
            continue

        if flag in _FLAGS_WITH_VALUE:
            value, original_args, i = _consume_value_flag(argv_list, i)
            if value is None:
                pnpm_globals.extend(original_args)
                continue
            mapped_prefix = _append_mapped_global(flag, value, pnpm_globals)
            prefix = mapped_prefix or prefix
            continue

        if arg.startswith('-'):
            pnpm_globals.append(arg)
            i += 1
            continue

        break

    rest = argv_list[i:]
    if _is_global_install(rest):
        force_passthrough = True

    return ParsedNpmArgs(prefix, pnpm_globals, rest, force_passthrough,
                         show_version)


def _split_option_value(arg: str) -> Tuple[str, Optional[str]]:
    if not arg.startswith('--') or '=' not in arg:
        return arg, None
    name, value = arg.split('=', 1)
    return name, value


def _consume_value_flag(args: Sequence[str],
                        index: int) -> Tuple[Optional[str], List[str], int]:
    arg = args[index]
    _, inline_value = _split_option_value(arg)
    if inline_value is not None:
        return inline_value, [arg], index + 1
    if index + 1 < len(args):
        return args[index + 1], [arg, args[index + 1]], index + 2
    return None, [arg], index + 1


def _append_mapped_global(flag: str, value: Optional[str],
                          pnpm_globals: List[str]) -> Optional[str]:
    if flag == '--prefix':
        if value is None:
            return None
        pnpm_globals.extend(['--dir', value])
        return value

    if flag in _GLOBAL_FLAGS_OMIT:
        return None

    if flag in _GLOBAL_FLAG_MAP:
        pnpm_globals.append(_GLOBAL_FLAG_MAP[flag])
        return None

    if value is None:
        pnpm_globals.append(flag)
    else:
        pnpm_globals.append(f'{flag}={value}')
    return None


def _is_global_install(args: Sequence[str]) -> bool:
    return bool(args and args[0] in _INSTALL_COMMANDS
                and any(arg in _GLOBAL_INSTALL_FLAGS for arg in args[1:]))


def _strip_run_separator(args: Sequence[str]) -> List[str]:
    return [a for a in args if a != '--']


def _translate_install_args(subcommand: str,
                            args: Sequence[str]) -> Tuple[str, List[str]]:
    """Map install/ci argv to pnpm install/add argv."""
    subargs = list(args)
    frozen = subcommand == 'ci'
    packages: List[str] = []
    install_flags: List[str] = []
    i = 0

    while i < len(subargs):
        arg = subargs[i]
        flag, _ = _split_option_value(arg)

        if flag in _GLOBAL_FLAGS_OMIT:
            i += 1
            continue

        if flag in _GLOBAL_INSTALL_FLAGS:
            i += 1
            continue

        if flag == '--ignore-scripts':
            install_flags.append(arg)
            i += 1
            continue

        if flag in _FLAGS_WITH_VALUE:
            _, original_args, i = _consume_value_flag(subargs, i)
            install_flags.extend(original_args)
            continue

        if arg.startswith('-'):
            install_flags.append(arg)
            i += 1
            continue

        packages.append(arg)
        i += 1

    if packages:
        return 'add', install_flags + packages
    cmd = 'install'
    if frozen:
        install_flags.append('--frozen-lockfile')
    return cmd, install_flags


if __name__ == '__main__':
    sys.exit(main())
