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
import os
import shutil
import subprocess
import sys
from pathlib import Path

import build_utils
from build_utils import AST_GREP_PLATFORM_DIR, CHROMIUM_ROOT, LLVM_BIN_DIR, \
    THIRD_PARTY

# Pinning the `v1.0.0` tag's commit.
TREE_SITTER_GN_GIT_URL = (
    'https://github.com/tree-sitter-grammars/tree-sitter-gn.git')
TREE_SITTER_GN_REF = 'bc06955bc1e3c9ff8e9b2b2a55b38b94da923c05'

TREE_SITTER_GN_SRC_DIR: Path = THIRD_PARTY / 'tree-sitter-gn-src'

_SHARED_LIB_EXT = {'win32': 'dll', 'darwin': 'dylib'}.get(sys.platform, 'so')

# `The file providing details of how to load the custom tree-sitter.
_SGCONFIG_TEMPLATE = """\
customLanguages:
  gn:
    libraryPath: %(library_path)s
    extensions: [gn, gni]
    expandoChar: _
"""


def _windows_sdk_lib_dirs() -> list[Path]:
    """VC tools + Windows SDK import-library directories.
    """
    sys.path.insert(0, str(CHROMIUM_ROOT / 'build'))
    from vs_toolchain import (
        FindVCComponentRoot,  # noqa: E402
        SDK_VERSION,
        SetEnvironmentAndGetSDKDir)
    sdk_dir = Path(SetEnvironmentAndGetSDKDir())
    return [
        Path(FindVCComponentRoot('Tools')) / 'lib' / 'x64',
        sdk_dir / 'Lib' / SDK_VERSION / 'um' / 'x64',
        sdk_dir / 'Lib' / SDK_VERSION / 'ucrt' / 'x64',
    ]


def _compile(output: Path) -> None:
    """Compile `parser.c` + `scanner.c` into the shared library at `output`.

    `parser.c`'s pre-generated ABI (14) sits within ast-grep's tree-sitter
    compatible range ([13, 15] as of tree-sitter 0.26), so it is used as-is,
    with no `tree-sitter generate` step.
    """
    src_dir = TREE_SITTER_GN_SRC_DIR / 'src'
    sources = [src_dir / 'parser.c', src_dir / 'scanner.c']

    env = dict(os.environ)
    if sys.platform == 'win32':
        clang_cl = LLVM_BIN_DIR / 'clang-cl.exe'
        cmd = [
            str(clang_cl), '/LD', '/O2', f'-I{src_dir}', *map(str, sources),
            f'-Fe:{output}', '-fuse-ld=lld', '-link', '/EXPORT:tree_sitter_gn'
        ]
        env['LIB'] = os.pathsep.join(str(p) for p in _windows_sdk_lib_dirs())
    else:
        clang = LLVM_BIN_DIR / 'clang'
        shared_flag = '-dynamiclib' if sys.platform == 'darwin' else '-shared'
        cmd = [
            str(clang), shared_flag, '-fPIC', '-O2', '-fuse-ld=lld',
            f'-I{src_dir}', *map(str, sources), '-o',
            str(output)
        ]
        if sys.platform == 'darwin':
            sdk_path = subprocess.run(['xcrun', '--show-sdk-path'],
                                      check=True,
                                      capture_output=True,
                                      text=True).stdout.strip()
            cmd += ['-isysroot', sdk_path]

    logging.info('Compiling tree-sitter-gn -> %s', output)
    subprocess.run(cmd, check=True, env=env)


def build(clean: bool = False) -> Path:
    """Build the `gn` custom-language library into `AST_GREP_PLATFORM_DIR/lib/`.

    Also (re)writes `AST_GREP_PLATFORM_DIR/sgconfig.yml` registering it.
    Returns the compiled library's path.
    """
    if clean and TREE_SITTER_GN_SRC_DIR.exists():
        logging.info('Removing %s', TREE_SITTER_GN_SRC_DIR)
        shutil.rmtree(TREE_SITTER_GN_SRC_DIR)

    build_utils.shallow_clone_pinned(TREE_SITTER_GN_GIT_URL,
                                     TREE_SITTER_GN_REF,
                                     TREE_SITTER_GN_SRC_DIR)

    lib_dir = AST_GREP_PLATFORM_DIR / 'lib'
    lib_dir.mkdir(parents=True, exist_ok=True)
    output = lib_dir / f'gn.{_SHARED_LIB_EXT}'
    _compile(output)

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
