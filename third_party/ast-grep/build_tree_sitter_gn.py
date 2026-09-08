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
import sys
from pathlib import Path

import build_utils
from build_utils import AST_GREP_PLATFORM_DIR, BRAVE_ROOT, CHROMIUM_ROOT, \
    THIRD_PARTY

sys.path.insert(0, str(BRAVE_ROOT / 'tools' / 'cr' / 'toolchains'))

from cherry_picks import _check_call

# Pinning the `v1.0.0` tag's commit.
TREE_SITTER_GN_GIT_URL = (
    'https://github.com/tree-sitter-grammars/tree-sitter-gn.git')
TREE_SITTER_GN_REF = 'bc06955bc1e3c9ff8e9b2b2a55b38b94da923c05'

TREE_SITTER_GN_SRC_DIR: Path = THIRD_PARTY / 'tree-sitter-gn-src'

# The output dir used when building the tree sitter
GN_OUT_DIR: Path = CHROMIUM_ROOT / 'out' / 'ast-grep-tree-sitter-gn'

# the target for the tree-sitter shared library
GN_LABEL = '//brave/third_party/ast-grep:tree_sitter_gn'

# the target for the unit test.
GN_TEST_LABEL = '//brave/third_party/ast-grep:tree_sitter_gn_unittests'

_GN_ARGS = ' '.join([
    f'root_extra_deps = ["{GN_LABEL}", "{GN_TEST_LABEL}"]',
    'is_debug = false',
    'is_component_build = false',
    'dcheck_always_on = false',
    'symbol_level = 0',
])

# `The file providing details of how to load the custom tree-sitter.
_SGCONFIG_TEMPLATE = """\
customLanguages:
  gn:
    libraryPath: %(library_path)s
    extensions: [gn, gni]
    expandoChar: _
"""


def _gn_built_output(label: str) -> Path:
    """The file `label` builds to, as GN reports it.
    """
    outputs = _check_call('gn',
                          'desc',
                          str(GN_OUT_DIR),
                          label,
                          'outputs',
                          cwd=CHROMIUM_ROOT,
                          capture_output=True).stdout.split()
    if not outputs:
        raise RuntimeError(f'`gn desc` reported no outputs for {label}')

    # GN lists the primary output first, with any others are link byproducts,
    # such as a `.TOC` file or an import library.
    built = CHROMIUM_ROOT / outputs[0].removeprefix('//')
    if not built.is_file():
        raise RuntimeError(f'ninja finished but {label} left no {built}')
    return built


def _compile() -> Path:
    """Build the grammar and its test, returning the shared library's path.
    """
    logging.info('Generating %s', GN_OUT_DIR)
    _check_call('gn',
                'gen',
                str(GN_OUT_DIR),
                f'--args={_GN_ARGS}',
                cwd=CHROMIUM_ROOT)

    logging.info('Compiling tree-sitter-gn')
    targets = [label.removeprefix('//') for label in (GN_LABEL, GN_TEST_LABEL)]
    # `autoninja` rather than `ninja`, so whichever of siso or ninja the
    # generated `args.gn` calls for is the one that runs.
    _check_call('autoninja',
                '-C',
                str(GN_OUT_DIR),
                *targets,
                cwd=CHROMIUM_ROOT)

    return _gn_built_output(GN_LABEL)


def _run_test() -> None:
    """Check ast-grep loads the freshly installed grammar.

    Runs after installation, since the test scans with the `sgconfig.yml` and
    library that land in `AST_GREP_PLATFORM_DIR`, not the build output.
    """
    test_bin = _gn_built_output(GN_TEST_LABEL)
    logging.info('Running %s', test_bin.name)
    _check_call(str(test_bin))


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
    # GN's own extension is reused, dropping its platform-specific prefix.
    output = lib_dir / f'gn{library.suffix}'
    logging.info('Installing %s -> %s', library, output)
    shutil.copy2(library, output)

    # Write the `sgconfig.yml` registering the library, so ast-grep can find it.
    sgconfig_path = AST_GREP_PLATFORM_DIR / 'sgconfig.yml'
    lib_rel = output.relative_to(AST_GREP_PLATFORM_DIR)
    sgconfig_path.write_text(_SGCONFIG_TEMPLATE %
                             {'library_path': lib_rel.as_posix()},
                             newline='\n')

    _run_test()
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
