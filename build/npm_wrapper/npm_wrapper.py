#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""PATH shim: translate `npm` to `pnpm` when pnpm-lock.yaml is present."""

from dataclasses import dataclass
import os
import shutil
import subprocess
import sys
from typing import List, Optional, Sequence, Tuple

PNPM_LOCKFILE = 'pnpm-lock.yaml'
WRAPPER_DISABLE_ENV = 'BRAVE_NPM_WRAPPER'

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


def _wrapper_dir() -> str:
    return os.path.dirname(os.path.abspath(__file__))


def _trim_path() -> str:
    wrapper = _wrapper_dir()
    parts = [
        p for p in os.environ.get('PATH', '').split(os.pathsep)
        if p and os.path.normcase(os.path.normpath(p)) != os.path.normcase(
            os.path.normpath(wrapper))
    ]
    return os.pathsep.join(parts)


def _which(cmd: str) -> Optional[str]:
    return shutil.which(cmd, path=_trim_path())


def find_real_npm() -> str:
    npm = _which('npm')
    if not npm:
        raise RuntimeError(
            'real npm not found on PATH (wrapper directory excluded)')
    return npm


def find_pnpm() -> str:
    pnpm = _which('pnpm')
    if not pnpm:
        raise RuntimeError('pnpm not found on PATH')
    return pnpm


def wrapper_disabled() -> bool:
    value = os.environ.get(WRAPPER_DISABLE_ENV, '')
    return value.lower() in ('0', 'false', 'no', 'off')


def resolve_project_dir(prefix: Optional[str]) -> str:
    if prefix:
        if os.path.isabs(prefix):
            return os.path.normpath(prefix)
        return os.path.normpath(os.path.join(os.getcwd(), prefix))
    return os.getcwd()


def should_use_pnpm(project_dir: str) -> bool:
    return os.path.isfile(os.path.join(project_dir, PNPM_LOCKFILE))


def is_pnpm_node_modules(node_modules: str) -> bool:
    if not os.path.isdir(node_modules):
        return False
    if os.path.isfile(os.path.join(node_modules, '.modules.yaml')):
        return True
    return os.path.isdir(os.path.join(node_modules, '.pnpm'))


def is_npm_node_modules(node_modules: str) -> bool:
    return os.path.isdir(
        node_modules) and not is_pnpm_node_modules(node_modules)


def maybe_clean_node_modules(project_dir: str) -> None:
    node_modules = os.path.join(project_dir, 'node_modules')
    if is_npm_node_modules(node_modules):
        shutil.rmtree(node_modules)


def _split_option_value(arg: str) -> Tuple[str, Optional[str]]:
    if not arg.startswith('--') or '=' not in arg:
        return arg, None
    name, value = arg.split('=', 1)
    return name, value


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


def _parse_global_argv(
    argv: Sequence[str]
) -> Tuple[Optional[str], List[str], List[str], bool, bool]:
    """Returns prefix, pnpm_global_flags, rest_argv, force_passthrough,
    show_version."""
    parsed = _parse_npm_argv(argv)
    return (parsed.prefix, parsed.pnpm_globals, parsed.rest,
            parsed.force_passthrough, parsed.show_version)


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
            if inline_value is not None:
                value = inline_value
                i += 1
            elif i + 1 < len(argv_list):
                value = argv_list[i + 1]
                i += 2
            else:
                pnpm_globals.append(arg)
                i += 1
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
        flag, inline_value = _split_option_value(arg)

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
            if inline_value is not None:
                install_flags.append(arg)
                i += 1
            elif i + 1 < len(subargs):
                install_flags.extend([arg, subargs[i + 1]])
                i += 2
            else:
                install_flags.append(arg)
                i += 1
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


def translate_to_pnpm(argv: Sequence[str], project_dir: str) -> List[str]:
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
        maybe_clean_node_modules(project_dir)
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


def run_real_npm(argv: Sequence[str]) -> int:
    cmd = [find_real_npm(), *argv]
    return subprocess.run(cmd, check=False).returncode


def run_pnpm(argv: Sequence[str], project_dir: str) -> int:
    cmd = translate_to_pnpm(argv, project_dir)
    cwd = None
    has_dir = '--dir' in cmd or any(arg.startswith('--dir=') for arg in cmd)
    if not has_dir and os.path.normpath(project_dir) != os.path.normpath(
            os.getcwd()):
        cwd = project_dir
    print(f'{" ".join(cmd)}')
    return subprocess.run(cmd, cwd=cwd, check=False).returncode


def main(argv: Optional[Sequence[str]] = None) -> int:
    if argv is None:
        argv = sys.argv[1:]

    if wrapper_disabled():
        return run_real_npm(argv)

    parsed = _parse_npm_argv(argv)
    project_dir = resolve_project_dir(parsed.prefix)

    if parsed.force_passthrough:
        return run_real_npm(argv)

    if not should_use_pnpm(project_dir):
        return run_real_npm(argv)

    return run_pnpm(argv, project_dir)


if __name__ == '__main__':
    sys.exit(main())
