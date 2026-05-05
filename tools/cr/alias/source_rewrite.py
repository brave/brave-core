# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Shared utilities for rewriting source-file references after a rename.

Used by alias.mv and alias.follow_renames.
"""

from __future__ import annotations

import logging
import os
import re
from pathlib import Path

import _boot  # noqa: F401
import repository

# File extensions that must be skipped during all search and rewrite operations.
SEARCH_EXCLUDE_EXTENSIONS: frozenset[str] = frozenset({'.patchinfo'})

# C++ file extensions for include/guard/shadow-include handling.
CPP_EXTENSIONS: frozenset[str] = frozenset(
    {'.h', '.hh', '.cc', '.mm', '.m', '.cpp'})

# Extensions whose files may contain #include / #import directives.
_INCLUDE_EXTENSIONS: frozenset[str] = CPP_EXTENSIONS | frozenset({'.mojom'})

# Extensions whose files may contain // path references in comments.
_COMMENT_EXTENSIONS: frozenset[str] = CPP_EXTENSIONS | frozenset(
    {'.gni', '.gn'})

# Walk filter rules, relative to the brave-core root.
# Each entry is '+' (include) or '-' (exclude) followed by a path.
# Bare-name patterns (no '/') match any directory component at any depth.
# Path patterns match from the brave-core root as a prefix.
# Longer (more specific) patterns take precedence over shorter ones.
SEARCH_EXCLUDE_DIRS: tuple[str, ...] = (
    '-__pycache__',
    '-.git',
    '-node_modules',
    '-out',
    '-third_party',
    '+third_party/blink',
    '+third_party/devtools-frontend',
    '-tools/crates/vendor',
    '-vendor',
)

# Pre-parsed: (include: bool, pattern: str) sorted longest-first for matching.
_EXCLUDE_RULES: list[tuple[bool, str]] = sorted(
    [(e[0] == '+', e[1:].rstrip('/')) for e in SEARCH_EXCLUDE_DIRS],
    key=lambda r: -len(r[1]),
)


def _is_path_excluded(rel_posix: str) -> bool:
    """Returns True if this brave-core-relative path is excluded by rules."""
    for include, pattern in _EXCLUDE_RULES:
        if '/' in pattern:
            matched = rel_posix == pattern or rel_posix.startswith(pattern +
                                                                   '/')
        else:
            matched = pattern in rel_posix.split('/')
        if matched:
            return not include
    return False


def _should_walk_dir(rel_posix: str) -> bool:
    """Returns True if os.walk should descend into this directory.

    Even if rel_posix is itself excluded, we must still walk it when a more
    specific '+' rule applies to a sub-path inside it (e.g. '+third_party/blink'
    requires walking into 'third_party').
    """
    if not _is_path_excluded(rel_posix):
        return True
    prefix = rel_posix + '/'
    return any(include and pattern.startswith(prefix)
               for include, pattern in _EXCLUDE_RULES if '/' in pattern)


def compute_guard(src_relative_path: Path | str) -> str:
    """Derives the #ifndef guard token from a path relative to src/.

    E.g. "brave/chromium_src/crypto/aead.h"
         -> "BRAVE_CHROMIUM_SRC_CRYPTO_AEAD_H_"
    """
    filename = Path(src_relative_path).as_posix()
    guard = filename.upper() + '_'
    for char in '/\\.+':
        guard = guard.replace(char, '_')
    return guard


def find_guard(content: str) -> str | None:
    """Returns the include guard token if a #ifndef/#define pair is found."""
    m = re.search(r'^#ifndef\s+(\w+)\s*\n#define\s+\1\b', content,
                  re.MULTILINE)
    return m.group(1) if m else None


def rewrite_guard_in_file(path: Path, old_guard: str, new_guard: str) -> None:
    """Replaces every occurrence of old_guard with new_guard in path.

    Logs a warning (does not raise) if the replacement count is not 3.
    """
    content = path.read_text(encoding='utf-8')
    new_content = content.replace(old_guard, new_guard)
    count = new_content.count(new_guard)
    if count != 3:
        logging.warning(
            '%s: guard rewrite produced %d occurrence(s) of %s (expected 3); '
            'inspect manually.', path, count, new_guard)
    path.write_text(new_content, encoding='utf-8', newline='\n')


def insert_guard(path: Path, new_guard: str) -> None:
    """Inserts a complete #ifndef/#define/#endif guard block into path.

    The opening lines are prepended before the first non-comment, non-blank
    line; the closing #endif is appended at the end of the file.
    """
    lines = path.read_text(encoding='utf-8').splitlines(keepends=True)

    insert_idx = len(lines)
    in_block = False
    for i, line in enumerate(lines):
        stripped = line.strip()
        if in_block:
            if '*/' in stripped:
                in_block = False
        elif not stripped or stripped.startswith('//'):
            pass
        elif stripped.startswith('/*'):
            in_block = True
        else:
            insert_idx = i
            break

    new_lines = (lines[:insert_idx] + [
        f'#ifndef {new_guard}\n',
        f'#define {new_guard}\n',
        '\n',
    ] + lines[insert_idx:] + [
        '\n',
        f'#endif  // {new_guard}\n',
    ])
    path.write_text(''.join(new_lines), encoding='utf-8', newline='\n')


def update_shadow_include(path: Path, old_chromium_path: Path,
                          new_chromium_path: Path) -> None:
    """Updates the upstream angle-bracket include inside a chromium_src/ file.

    Replaces '#include <old_chromium_path>' with '#include <new_chromium_path>'.
    No-op if the include is absent. Callers are responsible for verifying the
    file extension is a C++ type before calling.
    """
    old_include = f'#include <{old_chromium_path.as_posix()}>'
    content = path.read_text(encoding='utf-8')
    if old_include not in content:
        return
    path.write_text(content.replace(
        old_include, f'#include <{new_chromium_path.as_posix()}>'),
                    encoding='utf-8',
                    newline='\n')


def update_references(old_path: Path, new_path: Path) -> None:
    """Rewrites all references to old_path across the brave-core tree.

    Handles:
    - #include / #import directives (quoted and angle-bracket) in C++/.mojom
    - // comment lines in C++ and build files
    - BUILD.gn / .gni source-list entries in the ancestor chain of the old file
    - For moved .mojom files: derived generated-header paths

    Skips directories in SEARCH_EXCLUDE_DIRS and files in
    SEARCH_EXCLUDE_EXTENSIONS.
    """
    old_posix = old_path.as_posix()
    new_posix = new_path.as_posix()

    include_re = re.compile(r'(#?(?:include|import)\s*[\"<])' +
                            re.escape(old_posix) + r'([>\"])')
    include_sub = r'\g<1>' + new_posix + r'\g<2>'

    # For .mojom files, also rewrite derived generated-header paths.
    mojom_rewrites: list[tuple[re.Pattern[str], str]] = []
    if old_posix.endswith('.mojom'):
        old_base = old_posix[:-len('.mojom')]
        new_base = new_posix[:-len('.mojom')]
        for suffix in ('.mojom.h', '.mojom-blink.h', '.mojom-shared.h',
                       '.mojom-forward.h'):
            pat = re.compile(r'(#?(?:include|import)\s*[\"<])' +
                             re.escape(old_base + suffix) + r'([>\"])')
            mojom_rewrites.append(
                (pat, r'\g<1>' + new_base + suffix + r'\g<2>'))

    for dirpath, dirnames, filenames in os.walk(repository.BRAVE_CORE_PATH):
        rel_dir = Path(dirpath).relative_to(
            repository.BRAVE_CORE_PATH).as_posix()
        dirnames[:] = [
            d for d in dirnames
            if _should_walk_dir(d if rel_dir == '.' else f'{rel_dir}/{d}')
        ]
        if rel_dir != '.' and _is_path_excluded(rel_dir):
            continue
        for fname in filenames:
            fpath = Path(dirpath) / fname
            if fpath.suffix in SEARCH_EXCLUDE_EXTENSIONS:
                continue

            ext = fpath.suffix.lower()
            do_includes = ext in _INCLUDE_EXTENSIONS
            do_comments = ext in _COMMENT_EXTENSIONS
            if not (do_includes or do_comments):
                continue

            content = fpath.read_text(encoding='utf-8')
            new_content = content

            if do_includes:
                new_content = include_re.sub(include_sub, new_content)
                for pat, sub in mojom_rewrites:
                    new_content = pat.sub(sub, new_content)

            if do_comments:
                lines = new_content.splitlines(keepends=True)
                new_lines = [
                    line.replace(old_posix, new_posix)
                    if line.lstrip().startswith('//') and old_posix in line
                    else line for line in lines
                ]
                new_content = ''.join(new_lines)

            if new_content != content:
                fpath.write_text(new_content, encoding='utf-8', newline='\n')

    # Update BUILD.gn / .gni source-list entries in the ancestor chain.
    old_abs = repository.CHROMIUM_SRC_PATH / old_posix
    if not old_abs.is_relative_to(repository.BRAVE_CORE_PATH):
        return
    new_abs = repository.CHROMIUM_SRC_PATH / new_posix
    _update_build_ancestors(old_abs, new_abs, repository.BRAVE_CORE_PATH)


def patch_name_for(chromium_path: Path | str) -> str:
    """Converts a Chromium-relative path to the corresponding patch filename.

    E.g. "components/sync_device_info/device_info.h"
         -> "components-sync_device_info-device_info.h.patch"
    """
    return Path(chromium_path).as_posix().replace('/', '-') + '.patch'


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------


def _update_build_ancestors(old_abs: Path, new_abs: Path,
                            brave_root: Path) -> None:
    """Updates BUILD.gn/.gni entries in the ancestor dirs of old_abs."""
    cur = old_abs.parent
    while True:
        if cur.exists():
            for build_file in cur.iterdir():
                if not build_file.is_file():
                    continue
                name = build_file.name
                ext = build_file.suffix
                if name not in ('BUILD.gn', ) and ext not in ('.gni', '.gyp',
                                                              '.gypi'):
                    continue
                _update_build_entries(build_file, cur, old_abs, new_abs)
        if cur == brave_root:
            break
        parent = cur.parent
        if not (parent == brave_root or parent.is_relative_to(brave_root)):
            break
        cur = parent


def _update_build_entries(build_file: Path, build_dir: Path, old_abs: Path,
                          new_abs: Path) -> None:
    """Replaces the source-list entry for old_abs with new_abs in build_file."""
    rel_old = os.path.relpath(old_abs, build_dir).replace('\\', '/')
    rel_new = os.path.relpath(new_abs, build_dir).replace('\\', '/')
    old_str = f'"{rel_old}"'
    new_str = f'"{rel_new}"'
    content = build_file.read_text(encoding='utf-8')
    if old_str not in content:
        return
    build_file.write_text(content.replace(old_str, new_str),
                          encoding='utf-8',
                          newline='\n')
