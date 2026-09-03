#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A script to build the tree-sitter-gn grammar for ast-grep.
"""

from __future__ import annotations

import argparse
import logging
import shutil
import subprocess
import sys
from pathlib import Path

import build_utils
from build_utils import AST_GREP_PLATFORM_DIR, CHROMIUM_ROOT, THIRD_PARTY

# Pinning the `v1.0.0` tag's commit.
TREE_SITTER_GN_GIT_URL = (
    'https://github.com/tree-sitter-grammars/tree-sitter-gn.git')
TREE_SITTER_GN_REF = 'bc06955bc1e3c9ff8e9b2b2a55b38b94da923c05'

TREE_SITTER_GN_SRC_DIR: Path = THIRD_PARTY / 'tree-sitter-gn-src'

# The grammar is built by `//brave/third_party/ast-grep/BUILD.gn`, a target
# nothing else in the build depends on, so it gets its own out dir. That dir
# has to sit outside `//brave/`: devtools-frontend's `ts_library` rule filters
# its sources with `label_matches(src, ["//brave/*"])`, which a build dir under
# `//brave/` makes every generated devtools file match, breaking `gn gen`.
GN_OUT_DIR: Path = CHROMIUM_ROOT / 'out' / 'ast-grep-tree-sitter-gn'
GN_LABEL = '//brave/third_party/ast-grep:tree_sitter_gn'

_GN_ARGS = ' '.join([
    f'root_extra_deps = ["{GN_LABEL}"]',
    'is_debug = false',
    'symbol_level = 0',
    'use_siso = false',
])

# The overrides every Brave build applies to Chromium. A full checkout needs
# them: without `translate_genders = false` in particular, `gn gen` fails on
# Brave's locale paks. The recipe that packages ast-grep sparsely clones only
# `brave/third_party/ast-grep/`, where this file is absent — along with the
# Brave changes it compensates for.
_BRAVE_DEFAULTS_ARGS = '//brave/build/args/brave_defaults.gni'

_EXE = '.exe' if sys.platform == 'win32' else ''
_NINJA: Path = CHROMIUM_ROOT / 'third_party' / 'ninja' / f'ninja{_EXE}'

# What GN names the `shared_library`, and what ast-grep is handed.
_GN_LIBRARY_NAME = {
    'win32': 'gn.dll',
    'darwin': 'libgn.dylib'
}.get(sys.platform, 'libgn.so')
_SHARED_LIB_EXT = {'win32': 'dll', 'darwin': 'dylib'}.get(sys.platform, 'so')

# `The file providing details of how to load the custom tree-sitter.
_SGCONFIG_TEMPLATE = """\
customLanguages:
  gn:
    libraryPath: %(library_path)s
    extensions: [gn, gni]
    expandoChar: _
"""


def _compile() -> Path:
    """Build `GN_LABEL` in `GN_OUT_DIR`, returning the shared library's path.
    """
    gn = shutil.which('gn')
    if gn is None:
        raise RuntimeError('`gn` not found on PATH. Is depot_tools set up?')

    gn_args = _GN_ARGS
    brave_defaults = CHROMIUM_ROOT / _BRAVE_DEFAULTS_ARGS.removeprefix('//')
    if brave_defaults.is_file():
        # Prepended rather than appended: the import assigns `root_extra_deps`
        # itself, and GN refuses to overwrite a non-empty list.
        gn_args = (f'import("{_BRAVE_DEFAULTS_ARGS}") root_extra_deps = [] '
                   f'{gn_args}')

    logging.info('Generating %s', GN_OUT_DIR)
    subprocess.run([gn, 'gen', str(GN_OUT_DIR), f'--args={gn_args}'],
                   check=True,
                   cwd=CHROMIUM_ROOT)

    logging.info('Compiling tree-sitter-gn')
    ninja_target = GN_LABEL.removeprefix('//')
    subprocess.run(
        [str(_NINJA), '-C', str(GN_OUT_DIR), ninja_target], check=True)

    library = GN_OUT_DIR / _GN_LIBRARY_NAME
    if not library.is_file():
        raise RuntimeError(f'ninja finished but no library at {library}')
    return library


def build(clean: bool = False) -> Path:
    """Build the `gn` custom-language library into `AST_GREP_PLATFORM_DIR/lib/`.

    Also (re)writes `AST_GREP_PLATFORM_DIR/sgconfig.yml` registering it.
    Returns the installed library's path.
    """
    if clean and TREE_SITTER_GN_SRC_DIR.exists():
        logging.info('Removing %s', TREE_SITTER_GN_SRC_DIR)
        shutil.rmtree(TREE_SITTER_GN_SRC_DIR)

    build_utils.shallow_clone_pinned(TREE_SITTER_GN_GIT_URL,
                                     TREE_SITTER_GN_REF,
                                     TREE_SITTER_GN_SRC_DIR)

    library = _compile()

    lib_dir = AST_GREP_PLATFORM_DIR / 'lib'
    lib_dir.mkdir(parents=True, exist_ok=True)
    output = lib_dir / f'gn.{_SHARED_LIB_EXT}'
    logging.info('Installing %s -> %s', library, output)
    shutil.copy2(library, output)

    # Write the `sgconfig.yml` registering the library, so ast-grep can find it.
    sgconfig_path = AST_GREP_PLATFORM_DIR / 'sgconfig.yml'
    lib_rel = output.relative_to(AST_GREP_PLATFORM_DIR)
    sgconfig_path.write_text(_SGCONFIG_TEMPLATE %
                             {'library_path': lib_rel.as_posix()},
                             newline='\n')
    return output


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Compile the tree-sitter-gn grammar for ast-grep.')
    parser.add_argument('--clean',
                        action='store_true',
                        help='Remove third_party/tree-sitter-gn-src/ before '
                        'building.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable debug logging.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        force=True)

    gn_lib = build(clean=args.clean)

    logging.info('Done.')
    logging.info('gn library: %s', gn_lib)
    return 0


if __name__ == '__main__':
    sys.exit(main())
