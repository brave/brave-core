#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""git_cr mv — move a file or directory in brave-core and repair artefacts.

Performs the filesystem or git rename then updates every downstream artefact
that depends on the old path: C++ include guards, chromium_src shadow-file
includes, cross-tree #include/#import references, // comment references,
BUILD.gn source-list entries, and plaster TOML patch files.
"""

from __future__ import annotations

import argparse
import logging
from pathlib import Path

from incendiary_error_handler import IncendiaryErrorHandler
from terminal import console
import repository
import plaster
from plaster import PlasterFile
from source_rewrite import (
    CPP_EXTENSIONS,
    compute_guard,
    find_guard,
    insert_guard,
    patch_name_for,
    rewrite_guard_in_file,
    update_references,
    update_shadow_include,
)
from user_validation_error import UserValidationError

# Only .h files receive include-guard processing (spec §1.2 Step 2).
_HEADER_EXTENSIONS: frozenset[str] = frozenset({'.h'})

_FilePair = tuple[Path, Path]


def cmd_mv(args: list[str]) -> int:
    """Parse mv arguments and perform the file move with all repair steps."""
    parser = argparse.ArgumentParser(
        prog='git cr mv',
        description=
        'Move a file or directory inside brave-core and repair artefacts.',
    )
    parser.add_argument('--mkdir',
                        action='store_true',
                        help='Create destination parent directory if missing')
    parser.add_argument('--no-git',
                        action='store_true',
                        dest='no_git',
                        help='Use filesystem rename instead of git mv')
    parser.add_argument('--no-run-plaster',
                        action='store_true',
                        dest='no_run_plaster',
                        help='Skip running plaster after moving TOML files')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose logging')
    parser.add_argument('source', help='Source file or directory')
    parser.add_argument('destination', help='Destination path')
    parsed = parser.parse_args(args)

    logging.basicConfig(
        level=logging.DEBUG if parsed.verbose else logging.INFO,
        format='%(message)s',
        handlers=[IncendiaryErrorHandler(markup=True, rich_tracebacks=True)])

    cwd = Path.cwd()
    try:
        cwd.relative_to(repository.BRAVE_CORE_PATH)
    except ValueError:
        raise UserValidationError(
            'git cr mv: must be run from within the brave-core tree '
            f'({repository.BRAVE_CORE_PATH})') from None

    src = (cwd / parsed.source).resolve()
    dest = (cwd / parsed.destination).resolve()

    file_pairs = _step1_move(src, dest, parsed.mkdir, parsed.no_git)

    _step2_guards(file_pairs)
    _step3_shadow_includes(file_pairs)

    for old_file, new_file in file_pairs:
        old_rel = old_file.relative_to(repository.CHROMIUM_SRC_PATH)
        new_rel = new_file.relative_to(repository.CHROMIUM_SRC_PATH)
        update_references(old_rel, new_rel)

    _step5_plaster(file_pairs, parsed.no_git, not parsed.no_run_plaster)

    console.log(f'[bold green]✔[/] {parsed.source} → {parsed.destination}')
    return 0


def _step1_move(src: Path, dest: Path, mkdir: bool,
                no_git: bool) -> list[_FilePair]:
    """Validates paths and performs the move.

    Returns a list of (old_abs, new_abs) pairs for every file moved.
    All pairs are collected before the move so old paths are available
    for later artefact repair steps.
    """
    rewrite_path = plaster.PLASTER_FILES_PATH

    if not src.exists():
        raise UserValidationError(f'git cr mv: source does not exist: {src}')

    if dest.is_file():
        raise UserValidationError(
            f'git cr mv: destination already exists: {dest}')

    if not dest.parent.exists():
        if not mkdir:
            raise UserValidationError(
                f'git cr mv: destination parent does not exist: {dest.parent}\n'
                'Pass --mkdir to create it automatically.')
        dest.parent.mkdir(parents=True, exist_ok=True)

    if (src.is_relative_to(rewrite_path)
            and not dest.is_relative_to(rewrite_path)):
        raise UserValidationError(
            'git cr mv: cannot move a rewrite/ path to a destination outside '
            f'rewrite/ ({rewrite_path})')

    if src.is_dir():
        file_pairs: list[_FilePair] = [(f, dest / f.relative_to(src))
                                       for f in src.rglob('*') if f.is_file()]
    else:
        file_pairs = [(src, dest)]

    if no_git:
        src.rename(dest)
    else:
        repository.brave.run_git('mv', str(src), str(dest))

    return file_pairs


def _step2_guards(file_pairs: list[_FilePair]) -> None:
    """Regenerates C++ include guards for every moved .h file."""
    for _, new_file in file_pairs:
        if new_file.suffix not in _HEADER_EXTENSIONS:
            continue
        new_guard = compute_guard(
            new_file.relative_to(repository.CHROMIUM_SRC_PATH))
        content = new_file.read_text(encoding='utf-8')
        old_guard = find_guard(content)
        if old_guard:
            rewrite_guard_in_file(new_file, old_guard, new_guard)
        else:
            insert_guard(new_file, new_guard)


def _step3_shadow_includes(file_pairs: list[_FilePair]) -> None:
    """Updates the upstream angle-bracket include in moved shadow files."""
    chromium_src_path = repository.BRAVE_CORE_PATH / 'chromium_src'
    for old_file, new_file in file_pairs:
        if not old_file.is_relative_to(chromium_src_path):
            continue
        if new_file.suffix.lower() not in CPP_EXTENSIONS:
            continue
        old_chromium = old_file.relative_to(chromium_src_path)
        new_chromium = new_file.relative_to(chromium_src_path)
        update_shadow_include(new_file, old_chromium, new_chromium)


def _step5_plaster(file_pairs: list[_FilePair], no_git: bool,
                   run_plaster: bool) -> None:
    """Deletes stale patch files and optionally re-runs plaster for moved
    TOMLs."""
    rewrite_path = plaster.PLASTER_FILES_PATH
    patches_path = repository.BRAVE_CORE_PATH / 'patches'

    for old_file, new_file in file_pairs:
        if old_file.suffix != '.toml':
            continue
        if not old_file.is_relative_to(rewrite_path):
            continue
        old_chromium_path = old_file.relative_to(rewrite_path).with_suffix('')
        patch_file = patches_path / patch_name_for(old_chromium_path)
        if not patch_file.exists():
            logging.warning(
                'Expected patch file not found: %s; skipping deletion.',
                patch_file)
        else:
            if no_git:
                patch_file.unlink()
            else:
                repository.brave.run_git('rm', str(patch_file))

        if run_plaster:
            try:
                PlasterFile(new_file).apply()
            except (Exception, SystemExit) as e:
                logging.warning('plaster failed to apply %s: %s', new_file, e)
