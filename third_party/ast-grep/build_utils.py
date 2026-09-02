# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Shared paths and build helpers for `build_ast_grep.py` and
`build_tree_sitter_gn.py`, so the two scripts can't disagree on where things
live.
"""

from __future__ import annotations

import logging
import platform
import subprocess
import sys
from pathlib import Path

BRAVE_ROOT: Path = Path(__file__).resolve().parents[2]
CHROMIUM_ROOT: Path = BRAVE_ROOT.parent
THIRD_PARTY: Path = BRAVE_ROOT / 'third_party'

# Chromium's own clang/lld, fetched by `tools/clang/scripts/update.py` as part
# of `gclient sync`.
LLVM_BIN_DIR: Path = (CHROMIUM_ROOT / 'third_party' / 'llvm-build' /
                      'Release+Asserts' / 'bin')


def platform_dir() -> str:
    """Host-OS token used in ast-grep's per-platform dir name (`ast-grep-<os>`)."""
    if sys.platform == 'darwin':
        return 'mac_arm64' if platform.machine() == 'arm64' else 'mac'
    if sys.platform == 'win32':
        return 'win'
    return 'linux'


AST_GREP_DIR: Path = THIRD_PARTY / 'ast-grep'
# Per-OS install root: the `ast-grep` binary, the `gn` custom-language
# library, and its `sgconfig.yml` all live side by side under here.
AST_GREP_PLATFORM_DIR: Path = AST_GREP_DIR / f'ast-grep-{platform_dir()}'


def shallow_clone_pinned(git_url: str, ref: str, dest: Path) -> None:
    """Shallow-fetch `ref` from `git_url` into `dest`, if not already present.

    A pre-existing checkout is left alone so local edits / a custom branch
    survive across runs. Callers remove `dest` first (e.g. on `--clean`) to
    force a re-fetch.
    """
    if dest.is_dir():
        logging.info('%s source already present at %s', git_url, dest)
        return

    dest.mkdir(parents=True, exist_ok=True)
    logging.info('Fetching %s (%s) into %s', git_url, ref, dest)
    git = ['git', '-C', str(dest)]
    subprocess.run([*git, 'init', '-q'], check=True)
    subprocess.run([*git, 'remote', 'add', 'origin', git_url], check=True)
    subprocess.run([*git, 'fetch', '--depth=1', 'origin', ref], check=True)
    subprocess.run([*git, 'checkout', '-q', 'FETCH_HEAD'], check=True)
