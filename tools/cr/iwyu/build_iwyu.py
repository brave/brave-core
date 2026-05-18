#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build `include-what-you-use` against Chromium's pinned LLVM revision.

Keep this script standalone, with no dependencies on other local python sources
in tools/cr, as we may want to run the build in CI, in the future, using only
a vanilla Chromium checkout.

What this script does:

  1. Clones the LLVM monorepo into `out/iwyu/tools/clang/third_party/llvm`,
     pinned to `CLANG_REVISION` from `tools/clang/scripts/update.py`.  This
     matches the revision Chromium's bundled Clang is built from, so the
     resulting IWYU binary speaks the same Clang AST as the rest of the
     toolchain.
  2. Clones include-what-you-use into
     `out/iwyu/tools/clang/third_party/iwyu`, alongside the LLVM checkout
     (pinned to the `IWYU_REVISION` constant below).
  3. Configures a CMake build that pulls IWYU in as an LLVM external project
     (`LLVM_EXTERNAL_PROJECTS=iwyu` +
     `LLVM_EXTERNAL_IWYU_SOURCE_DIR=…/iwyu`).  Building IWYU as part of the
     LLVM build avoids the standalone-build resource-dir gymnastics
     (see IWYU README's "How to install" section).
  4. Invokes `ninja include-what-you-use` inside the LLVM build tree.

The resulting binary lands at:

  out/iwyu/tools/clang/third_party/llvm/build/bin/include-what-you-use

"""

from __future__ import annotations

import argparse
import logging
import platform
import shutil
import subprocess
import sys
from pathlib import Path, PurePath

CHROMIUM_SRC: Path = Path(__file__).resolve().parents[4]

# Importing clang toolchain helper from Chromium.
sys.path.append(str(CHROMIUM_SRC / 'tools' / 'clang' / 'scripts'))

# Static analysers can't see through the sys.path mutation above.
# pylint: disable=wrong-import-position
from build import AddCMakeToPath, CheckoutGitRepo, LLVM_GIT_URL  # type: ignore  # noqa: E402
from update import CLANG_REVISION  # type: ignore  # noqa: E402
# pylint: enable=wrong-import-position

# The repo to clone from for the build.
IWYU_GIT_URL = ('https://github.com/include-what-you-use/'
                'include-what-you-use.git')

# Pinned IWYU revision.  IWYU master tracks LLVM main; we pin to a specific
# commit that compiles against the Clang revision Chromium has currently
# pinned (`update.CLANG_REVISION`).  Bump when bumping Chromium's Clang.
#
# `ece9edb` is the commit immediately before
# c36eacb "[clang compat] Handle new HLSL builtin trait", which references
# `clang::TypeTrait::UTT_IsConstantBufferElementCompatible` -- a symbol that
# doesn't exist in `llvmorg-23-init-5669-g8a0be0bc` (our current pin).
IWYU_REVISION = 'ece9edb'

# Constants for the the directory layout of where these tools live under.
THIRD_PARTY_REL = PurePath('tools') / 'clang' / 'third_party'
LLVM_REL = THIRD_PARTY_REL / 'llvm'
IWYU_REL = THIRD_PARTY_REL / 'iwyu'
BUILD_REL = LLVM_REL / 'build'

# We always build it at `out/iwyu` in Chromium.
OUT_DIR: Path = CHROMIUM_SRC / 'out' / 'iwyu'


def _check_call(*command, cwd=None):
    """Run *command* as a subprocess, logging the invocation.

    Stdout and stderr are inherited from this process so the user sees ninja
    progress and compiler diagnostics live -- important for long-running
    cmake/ninja invocations where errors otherwise scroll past or get lost.
    """
    logging.info(' >>>> %s', ' '.join(str(a) for a in command))

    if platform.system() == 'Windows':
        # Resolve to an absolute path so .bat shims (e.g. `cmake.bat`) are
        # found without `shell=True`.
        resolved = shutil.which(command[0])
        if resolved is None:
            raise RuntimeError(f'Command not found: {command[0]}')
        if resolved != command[0]:
            command = [resolved] + list(command[1:])

    subprocess.run(command, cwd=cwd, check=True)


class IwyuBuilder:
    """Clone LLVM + IWYU and build the IWYU binary against pinned Clang.

    Three phases:

    1. **Checkout** (`_checkout_llvm`, `_checkout_iwyu`): clones (or updates)
       LLVM and IWYU at the requested revisions into the layout described
       in the module docstring.  Uses `build.CheckoutGitRepo` from the
       Chromium clang scripts, which idempotently fetches/updates an
       existing clone or re-clones from scratch if the working tree is
       broken.

    2. **Configure** (`_configure`): runs `cmake -GNinja` against
       `<llvm-clone>/llvm/CMakeLists.txt` with IWYU wired in as an LLVM
       external project.  Build dir lives at `<llvm-clone>/build/`, matching
       `build_clang_tools_extra.py`.

    3. **Build** (`_build`): `ninja <targets>` (default:
       `include-what-you-use`).
    """

    def __init__(self):
        self.llvm_dir: Path = OUT_DIR / LLVM_REL
        self.iwyu_dir: Path = OUT_DIR / IWYU_REL
        self.build_dir: Path = OUT_DIR / BUILD_REL

    def _checkout_llvm(self):
        """Clone or update LLVM at the pinned Chromium Clang revision."""
        self.llvm_dir.parent.mkdir(parents=True, exist_ok=True)
        CheckoutGitRepo('LLVM monorepo', LLVM_GIT_URL, CLANG_REVISION,
                        str(self.llvm_dir))

    def _checkout_iwyu(self):
        """Clone or update IWYU at the pinned revision."""
        self.iwyu_dir.parent.mkdir(parents=True, exist_ok=True)
        CheckoutGitRepo('include-what-you-use', IWYU_GIT_URL, IWYU_REVISION,
                        str(self.iwyu_dir))

    def _configure(self):
        """Configure the LLVM build with IWYU added as an external project.

        The LLVM monorepo's CMake root is `<llvm-clone>/llvm`, not the
        repo root.  IWYU is pulled in via the standard LLVM external-project
        knobs, so the build picks up the in-tree Clang's resource directory
        without any of the `IWYU_RESOURCE_RELATIVE_TO` gymnastics needed for
        a standalone IWYU build.
        """
        # Download Chromium's pinned cmake into third_party/llvm-build-tools/
        # and prepend it to PATH.  Sidesteps "wrong cmake on PATH" failures --
        # most often on macOS where Homebrew cmake versions skew vs. the
        # llvm-project requirements.
        AddCMakeToPath()

        self.build_dir.mkdir(parents=True, exist_ok=True)
        llvm_cmake_root = self.llvm_dir / 'llvm'
        cmake_args = [
            'cmake',
            '-GNinja',
            '-DLLVM_ENABLE_PROJECTS=clang',
            '-DLLVM_EXTERNAL_PROJECTS=iwyu',
            f'-DLLVM_EXTERNAL_IWYU_SOURCE_DIR={self.iwyu_dir}',
            '-DCMAKE_BUILD_TYPE=Release',
            '-DLLVM_ENABLE_ASSERTIONS=On',
            # IWYU is a Clang frontend tool and needs no codegen backends.
            # Restricting to the host backend skips AMDGPU/MIPS/PowerPC/etc.
            # which would otherwise take the bulk of the build time (and have
            # caused AMDGPU compile failures on some hosts).
            '-DLLVM_TARGETS_TO_BUILD=host',
            # Trim other optional bits that are irrelevant for a tool build.
            '-DLLVM_INCLUDE_TESTS=OFF',
            '-DLLVM_INCLUDE_EXAMPLES=OFF',
            '-DLLVM_INCLUDE_BENCHMARKS=OFF',
            '-DLLVM_INCLUDE_DOCS=OFF',
            str(llvm_cmake_root),
        ]
        _check_call(*cmake_args, cwd=self.build_dir)

    def _build(self):
        """Build the IWYU binary."""
        _check_call('ninja', 'include-what-you-use', cwd=self.build_dir)

    def run(self):
        """Execute the full checkout / configure / build pipeline."""
        OUT_DIR.mkdir(parents=True, exist_ok=True)

        logging.info('LLVM revision: %s', CLANG_REVISION)
        logging.info('IWYU revision: %s', IWYU_REVISION)
        logging.info('LLVM checkout: %s', self.llvm_dir)
        logging.info('IWYU checkout: %s', self.iwyu_dir)
        logging.info('Build dir:     %s', self.build_dir)

        self._checkout_llvm()
        self._checkout_iwyu()
        self._configure()
        self._build()

        produced = self.build_dir / 'bin' / 'include-what-you-use'
        logging.info('Build complete. Expected binary at: %s', produced)


def main():
    parser = argparse.ArgumentParser(
        description='Build include-what-you-use against Chromium-pinned LLVM.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose (debug) logging.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    IwyuBuilder().run()
    return 0


if __name__ == '__main__':
    sys.exit(main())
