# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""git_cr follow-renames — repair brave-core artefacts after upstream renames.

For each file rename found in the Chromium git log for the given revision
reference or range, updates every brave-core artefact that references the old
path: chromium_src/ shadow files (move + shadow include + include guard),
rewrite/ TOML files (move + patch deletion), and all cross-brave-core
#include/#import/comment/BUILD.gn references.

Usage
-----
  git cr follow-renames [--no-git] [--verbose] <ref-or-range>

  Both a single ref and a range are accepted natively.

  # All renames between two Chromium version tags (typical version bump):
  git cr follow-renames 130.0.6723.58..131.0.6778.85

  # All renames in the last N commits of the Chromium repo:
  git cr follow-renames HEAD~5..HEAD

  # Renames introduced by a single upstream commit:
  git cr follow-renames abc123
"""

from __future__ import annotations

import argparse
import logging
from pathlib import Path
import subprocess

import _boot  # noqa: F401
from incendiary_error_handler import IncendiaryErrorHandler
import plaster
from plaster import PlasterFile
from terminal import console, terminal
import repository
from alias.source_rewrite import (
    CPP_EXTENSIONS,
    compute_guard,
    find_guard,
    insert_guard,
    patch_name_for,
    rewrite_guard_in_file,
    update_references,
    update_shadow_include,
)

_RenamePair = tuple[Path, Path]


def cmd_follow_renames(args: list[str]) -> int:
    """Parse follow-renames arguments and process all upstream renames."""
    parser = argparse.ArgumentParser(
        prog='git cr follow-renames',
        description=('Repair brave-core artefacts after upstream Chromium '
                     'file renames.'),
    )
    parser.add_argument('--no-git',
                        action='store_true',
                        dest='no_git',
                        help='Use filesystem ops instead of git mv/rm')
    parser.add_argument('--no-run-plaster',
                        action='store_true',
                        dest='no_run_plaster',
                        help='Skip running plaster after moving TOML files')
    parser.add_argument(
        '--no-format',
        action='store_true',
        dest='no_format',
        help='Skip running `npm run format` after processing renames')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose logging')
    parser.add_argument(
        'ref_or_range',
        help=('Git revision range or single ref in the Chromium repo. '
              'A bare ref (e.g. HEAD or a tag) is treated as a single '
              'commit; a range (e.g. old_tag..new_tag) is used as-is.'))
    parsed = parser.parse_args(args)

    logging.basicConfig(
        level=logging.DEBUG if parsed.verbose else logging.INFO,
        format='%(message)s',
        handlers=[IncendiaryErrorHandler(markup=True, rich_tracebacks=True)])

    renames = _get_chromium_renames(parsed.ref_or_range)

    for old_chromium, new_chromium in renames:
        _repair_chromium_src(old_chromium, new_chromium, parsed.no_git)
        _repair_plaster_files(old_chromium, new_chromium, parsed.no_git,
                              not parsed.no_run_plaster)
        update_references(old_chromium, new_chromium)
        _repair_patch_files(old_chromium, new_chromium, parsed.no_git)

    if renames and not parsed.no_format:
        _run_format()

    console.log(f'[bold green]✔[/] {len(renames)} rename(s) processed')
    return 0


def _run_format() -> None:
    """Runs `npm run format` to clean up files touched by rename repairs.

    Failures are downgraded to warnings: format must not block successful
    rename processing.
    """
    try:
        terminal.run_npm_command('format')
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        logging.warning('npm run format failed: %s', e)


def _get_chromium_renames(ref_or_range: str) -> list[_RenamePair]:
    """Returns net (original → final) rename pairs from the Chromium git log.

    Runs git show --diff-filter=R --name-status in the Chromium repository.
    Accepts both a single ref (e.g. "HEAD", a tag) and a range
    (e.g. "old_tag..new_tag"); git show handles both forms natively.

    For ranges, multi-step renames are collapsed: a→b→c becomes a→c, and
    round-trips (a→b→a) are dropped entirely.
    """
    raw = repository.chromium.run_git('show', '--diff-filter=R',
                                      '--name-status', '--format=',
                                      ref_or_range)
    renames: list[_RenamePair] = []
    for line in raw.splitlines():
        parts = line.split('\t')
        if len(parts) == 3 and parts[0].startswith('R'):
            renames.append((Path(parts[1]), Path(parts[2])))
    return _collapse_renames(renames) if '..' in ref_or_range else renames


def _collapse_renames(renames: list[_RenamePair]) -> list[_RenamePair]:
    """Collapses chained renames into net (original → final) pairs.

    git show outputs renames newest-first. Processing in reverse
    (chronological) order lets us compose chains: if a→b was recorded
    earlier and we now see b→c, we update the entry to a→c. Round-trips
    (a→b→a) are dropped.
    """
    net: dict[Path, Path] = {}
    for old, new in reversed(renames):
        origin = next((k for k, v in net.items() if v == old), None)
        if origin is not None:
            if origin == new:
                del net[origin]
            else:
                net[origin] = new
        else:
            net[old] = new
    return list(net.items())


def _repair_chromium_src(old_chromium: Path, new_chromium: Path,
                         no_git: bool) -> None:
    """Moves and repairs the chromium_src/ shadow file for one rename.

    If no shadow file exists at chromium_src/old_chromium, this is a no-op.
    For moved files: updates the shadow #include line (C++ files) and
    regenerates the include guard (.h files only).
    """
    old_shadow = repository.BRAVE_CORE_PATH / 'chromium_src' / old_chromium
    if not old_shadow.exists():
        return

    new_shadow = repository.BRAVE_CORE_PATH / 'chromium_src' / new_chromium
    new_shadow.parent.mkdir(parents=True, exist_ok=True)

    if no_git:
        old_shadow.rename(new_shadow)
    else:
        repository.brave.run_git('mv', str(old_shadow), str(new_shadow))

    if new_shadow.suffix.lower() in CPP_EXTENSIONS:
        update_shadow_include(new_shadow, old_chromium, new_chromium)

    if new_shadow.suffix == '.h':
        new_guard = compute_guard(
            new_shadow.relative_to(repository.CHROMIUM_SRC_PATH))
        content = new_shadow.read_text(encoding='utf-8')
        old_guard = find_guard(content)
        if old_guard:
            rewrite_guard_in_file(new_shadow, old_guard, new_guard)
        else:
            insert_guard(new_shadow, new_guard)


def _repair_plaster_files(old_chromium: Path,
                          new_chromium: Path,
                          no_git: bool,
                          run_plaster: bool = True) -> None:
    """Moves the plaster file and deletes the corresponding patch file.

    Plaster file convention: chromium path A/foo.h lives at
    rewrite/A/foo.h.toml. If no plaster file exists, this is a no-op.
    The old patch file for that plaster gets automatically deleted.
    """
    old_toml = (plaster.PLASTER_FILES_PATH / old_chromium.parent /
                (old_chromium.name + '.toml'))
    if not old_toml.exists():
        return

    new_toml = (plaster.PLASTER_FILES_PATH / new_chromium.parent /
                (new_chromium.name + '.toml'))
    new_toml.parent.mkdir(parents=True, exist_ok=True)

    if no_git:
        old_toml.rename(new_toml)
    else:
        repository.brave.run_git('mv', str(old_toml), str(new_toml))

    patch_file = (repository.BRAVE_CORE_PATH / 'patches' /
                  patch_name_for(old_chromium))
    if not patch_file.exists():
        logging.warning(
            'Expected patch file not found: %s; skipping deletion.',
            patch_file)
    else:
        if no_git:
            patch_file.unlink()
        else:
            repository.brave.run_git('rm', str(patch_file))
        patchinfo_file = patch_file.with_suffix('.patchinfo')
        if patchinfo_file.exists():
            patchinfo_file.unlink()

    if run_plaster:
        try:
            PlasterFile(new_toml).apply()
            if not no_git:
                new_patch = (repository.BRAVE_CORE_PATH / 'patches' /
                             patch_name_for(new_chromium))
                if new_patch.exists():
                    repository.brave.run_git('add', str(new_patch))
        except (Exception, SystemExit) as e:
            logging.warning('plaster failed to apply %s: %s', new_toml, e)


def _repair_patch_files(old_chromium: Path, new_chromium: Path,
                        no_git: bool) -> None:
    """Renames and re-applies the .patch file for one upstream rename.

    If no patch file exists at patches/patch_name_for(old_chromium), this is
    a no-op. Patches already handled by _repair_plaster_files will have been
    deleted before this runs, so they are naturally skipped.

    The patch --- and +++ headers are updated to reference the new chromium
    path, the file is renamed, then git apply --3way is attempted. A warning
    is logged if the apply fails.
    """
    patches_path = repository.BRAVE_CORE_PATH / 'patches'
    old_patch = patches_path / patch_name_for(old_chromium)
    if not old_patch.exists():
        return

    new_patch = patches_path / patch_name_for(new_chromium)
    new_patch.parent.mkdir(parents=True, exist_ok=True)

    old_path_str = old_chromium.as_posix()
    new_path_str = new_chromium.as_posix()
    updated_lines = []
    for line in old_patch.read_text(encoding='utf-8').splitlines(
            keepends=True):
        if (line.startswith('diff --git ') or line.startswith('--- ')
                or line.startswith('+++ ')):
            line = line.replace(old_path_str, new_path_str)
        updated_lines.append(line)
    updated_content = ''.join(updated_lines)

    if no_git:
        old_patch.rename(new_patch)
    else:
        repository.brave.run_git('mv', str(old_patch), str(new_patch))
    new_patch.write_text(updated_content, encoding='utf-8')
    if not no_git:
        repository.brave.run_git('add', str(new_patch))

    try:
        repository.chromium.run_git('apply', '--3way', '--ignore-space-change',
                                    '--ignore-whitespace', str(new_patch))
    except subprocess.CalledProcessError as e:
        logging.warning('Failed to apply %s after rename: %s', new_patch,
                        e.stderr.strip() if e.stderr else str(e))
    finally:
        repository.chromium.run_git('reset', 'HEAD', '--',
                                    new_chromium.as_posix())
