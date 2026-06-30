#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build ast-grep and package `third_party/ast-grep/` into a versioned tarball.

Runs the same build as `build_ast_grep.py`, then archives the *contents* of
`third_party/ast-grep/` (the built binary tree) into a `.tar.gz` whose name
carries the ast-grep version and host platform:

    ast-grep-<version>-<platform>.tar.gz
"""

from __future__ import annotations

import argparse
import logging
import os
from pathlib import Path
import platform
import sys
import tarfile

import build_ast_grep


def _platform_tag() -> str:
    """GCS-style host platform tag, mirroring build_rust_toolchain.py."""
    if sys.platform == 'darwin':
        return 'mac-arm64' if platform.machine() == 'arm64' else 'mac'
    if sys.platform == 'win32':
        return 'win'
    return 'linux-x64'


def _package_name() -> str:
    """Output archive filename: `ast-grep-<version>-<platform>.tar.gz`."""
    return (f'ast-grep-{build_ast_grep.AST_GREP_REF}-{_platform_tag()}'
            '.tar.gz')


def _create_archive(out_dir: Path) -> Path:
    """Archive this host's `ast-grep-<os>/` binary tree into *out_dir*.

    Raises:
        RuntimeError: If the ast-grep build output directory is missing.
    """
    source = build_ast_grep.AST_GREP_PLATFORM_DIR
    if not source.is_dir():
        raise RuntimeError(f'ast-grep build output not found at {source}')

    out_dir.mkdir(parents=True, exist_ok=True)
    archive = out_dir / _package_name()

    logging.info('Packaging %s -> %s', source, archive)
    with tarfile.open(archive, 'w:gz') as tar:
        tar.add(source, arcname=source.name)
    return archive


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Build ast-grep and package third_party/ast-grep into a '
        'versioned tar.gz.')
    parser.add_argument('--out-dir',
                        type=Path,
                        default=Path.cwd(),
                        help='Directory the tar.gz is written to '
                        '(default: current directory).')
    parser.add_argument('--clean',
                        action='store_true',
                        help='Remove prior ast-grep build dirs before '
                        'building.')
    parser.add_argument('-j',
                        '--jobs',
                        type=int,
                        default=os.cpu_count() or 1,
                        help='Number of parallel build jobs (default: nproc).')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable debug logging.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        force=True)

    build_ast_grep.build(args.jobs, clean=args.clean)
    archive = _create_archive(args.out_dir.expanduser().resolve())

    logging.info('Done.')
    logging.info('ast-grep package: %s', archive)
    return 0


if __name__ == '__main__':
    sys.exit(main())
