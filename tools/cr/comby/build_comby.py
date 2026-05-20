#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build a local `opam` and use it to build `comby` from
`third_party/comby/`.

In the spirit of `tools/rust/build_rust.py`: nothing is installed
system-wide. opam is fetched from a pinned GitHub release tarball, built
via its cold-bootstrap target (which vendors its own OCaml compiler), and
installed under `third_party/comby-toolchain-intermediate/opam-install/`
(it is intermediate because it exists only to drive the comby build, by
analogy with the host LLVM that `build_rust.py` builds to drive rustc).
That opam binary is then driven against a project-local `OPAMROOT` to
create an OCaml switch and build `comby` from a checkout at
`third_party/comby-src/` (cloned automatically if missing). The final
comby install lands at `third_party/comby-toolchain/`, mirroring
`third_party/rust-toolchain/`.

Key paths:

  * `third_party/comby-src/`                                comby source
  * `third_party/comby-toolchain-intermediate/opam-<v>/`    opam source
  * `third_party/comby-toolchain-intermediate/opam-install/bin/opam`
  * `third_party/comby-toolchain-intermediate/opamroot/`    OPAMROOT
  * `third_party/comby-toolchain/bin/comby`                 final binary

The user is expected to have the OS-level build dependencies listed in
`third_party/comby/README.md` already installed (on Linux, the
`libpcre3-dev`, `libgmp-dev`, `libev4`, `libsqlite3-dev`, `m4`, etc.
packages; on macOS, the `pkg-config gmp pcre libev` brew formulae). This
script does not run package managers with sudo on the user's behalf.
"""

from __future__ import annotations

import argparse
import logging
import os
import shutil
import sys
import tarfile
import urllib.request
from pathlib import Path

# Make sibling `tools/cr` modules importable when this script is run
# directly from its subdirectory.
_CR_DIR = str(Path(__file__).resolve().parent.parent)
if _CR_DIR not in sys.path:
    sys.path.insert(0, _CR_DIR)

from repository import brave  # noqa: E402
from terminal import terminal  # noqa: E402

# Pinned opam release. The download URL is constructed from this and the
# extracted directory name is `opam-<version>` (GitHub's archive tarballs
# strip the leading `v` from the tag name).
OPAM_VERSION = '2.5.1'
OPAM_URL = (
    f'https://github.com/ocaml/opam/archive/refs/tags/{OPAM_VERSION}.tar.gz')

# OCaml compiler version used for the comby switch. comby's `comby.opam`
# requires `ocaml >= 4.08.1`; 4.14.2 is a current 4.x LTS and is
# available at the opam-repository tag pinned below.
OCAML_VERSION = '4.14.2'

# Upstream comby checkout. Pinned to `1.7.0` because that is the most
# recent comby version *published in opam-repository* (1.8.x exists as
# git tags upstream but was never uploaded to opam-repository, so it
# lacks the curated version constraints that keep the dep solver picking
# compatible package versions; see `_build_comby` for how we lean on
# those constraints).
COMBY_GIT_URL = 'https://github.com/comby-tools/comby.git'
COMBY_REF = '1.7.0'

# Pinned opam-repository snapshot. opam-repository keeps adding upper
# bounds and constraints to old packages as new versions of their deps
# break their APIs; resolving comby's deps against a tagged snapshot
# (rather than live HEAD) gives us a stable, reproducible solve. This
# is a real opam-repository tag, not an arbitrary hash.
OPAM_REPO_URL = 'https://github.com/ocaml/opam-repository.git'
OPAM_REPO_REF = '2026-05-before-lower-bound-ocaml411'

# comby ships three packages from the same source tree; we install the
# deps for all three so that the `make release` step (which builds all
# three) finds every library it needs.
COMBY_PACKAGES = ('comby', 'comby-kernel', 'comby-semantic')

# Paths used by the build. Layout mirrors `tools/rust/build_rust.py`:
#
#   * `<name>-src/`                       source clone
#   * `<name>-toolchain-intermediate/`    build-time-only artifacts
#   * `<name>-toolchain/`                 final install prefix
#
# opam falls under "toolchain-intermediate" because it only exists to
# drive the comby build, by analogy with the host LLVM build that
# `build_rust.py` produces to drive rustc.
# Resolve to an absolute path: `brave.root` is cwd-relative, but the
# build shells out into subprocesses whose cwd is set to a subdirectory
# (e.g. `OPAM_SRC_DIR`). Any relative path passed in `--prefix=` /
# `OPAMROOT` / etc. would then resolve against the subprocess's cwd
# instead of ours.
_THIRD_PARTY = (brave.root / 'third_party').resolve()
COMBY_SRC_DIR: Path = _THIRD_PARTY / 'comby-src'
COMBY_TOOLCHAIN_DIR: Path = _THIRD_PARTY / 'comby-toolchain'
COMBY_INTERMEDIATE_DIR: Path = _THIRD_PARTY / 'comby-toolchain-intermediate'
OPAM_TARBALL_PATH: Path = (COMBY_INTERMEDIATE_DIR /
                           f'opam-{OPAM_VERSION}.tar.gz')
OPAM_SRC_DIR: Path = COMBY_INTERMEDIATE_DIR / f'opam-{OPAM_VERSION}'
OPAM_INSTALL_DIR: Path = COMBY_INTERMEDIATE_DIR / 'opam-install'
OPAM_BIN: Path = OPAM_INSTALL_DIR / 'bin' / 'opam'
OPAMROOT_DIR: Path = COMBY_INTERMEDIATE_DIR / 'opamroot'
COMBY_BIN: Path = COMBY_TOOLCHAIN_DIR / 'bin' / 'comby'

# Name of the opam switch we create under OPAMROOT.
OPAM_SWITCH_NAME = 'comby'


def _download_opam() -> None:
    """Fetch the opam source tarball to `OPAM_TARBALL_PATH`.

    Skipped if the tarball already exists -- the URL is pinned by version
    so a present file is by definition the right bytes. Pass `--clean` to
    force a re-download.
    """
    if OPAM_TARBALL_PATH.is_file():
        logging.info('opam tarball already present at %s', OPAM_TARBALL_PATH)
        return

    COMBY_INTERMEDIATE_DIR.mkdir(parents=True, exist_ok=True)
    logging.info('Downloading opam %s from %s', OPAM_VERSION, OPAM_URL)
    # urlretrieve streams to disk -- no need to hold the whole tarball in
    # memory. The trailing rename guarantees we never leave a half-written
    # file at the final path if the download is interrupted.
    tmp_path = OPAM_TARBALL_PATH.with_suffix(OPAM_TARBALL_PATH.suffix +
                                             '.partial')
    urllib.request.urlretrieve(OPAM_URL, tmp_path)
    tmp_path.replace(OPAM_TARBALL_PATH)


def _extract_opam() -> None:
    """Extract the opam tarball into `COMBY_INTERMEDIATE_DIR`.

    Skipped if the expected source directory already exists.
    """
    if OPAM_SRC_DIR.is_dir():
        logging.info('opam source already extracted at %s', OPAM_SRC_DIR)
        return

    logging.info('Extracting %s', OPAM_TARBALL_PATH)
    with tarfile.open(OPAM_TARBALL_PATH, 'r:gz') as tar:
        # `filter='data'` is the safe, post-CVE-2007-4559 default: strips
        # absolute paths, blocks `..` traversal, and drops device files.
        tar.extractall(COMBY_INTERMEDIATE_DIR, filter='data')

    if not OPAM_SRC_DIR.is_dir():
        raise RuntimeError(
            f'opam tarball did not extract to expected directory: '
            f'{OPAM_SRC_DIR}')


def _build_opam(jobs: int) -> None:
    """Cold-bootstrap opam and install it under `OPAM_INSTALL_DIR`.

    Uses opam's `make cold` target, which downloads and builds a vendored
    OCaml compiler before building opam itself. This avoids any
    requirement on a system OCaml install.
    """
    if OPAM_BIN.is_file():
        logging.info('opam already built at %s', OPAM_BIN)
        return

    OPAM_INSTALL_DIR.mkdir(parents=True, exist_ok=True)
    # `make cold` takes the configure arguments via CONFIGURE_ARGS. We
    # pin the install prefix to our local directory so nothing escapes
    # into the user's system.
    configure_args = f'--prefix={OPAM_INSTALL_DIR}'
    # `interactive=True` inherits stdio so the live build output (and
    # any compiler / configure errors) reach the user's terminal
    # directly. `terminal.run`'s default capture mode would swallow
    # everything and only surface stderr at DEBUG level on failure.
    terminal.run(
        ['make', 'cold', f'CONFIGURE_ARGS={configure_args}', f'-j{jobs}'],
        cwd=OPAM_SRC_DIR,
        interactive=True)
    terminal.run(['make', 'cold-install'], cwd=OPAM_SRC_DIR, interactive=True)

    if not OPAM_BIN.is_file():
        raise RuntimeError(
            f'opam build finished but binary not found at {OPAM_BIN}')


def _opam_env() -> dict[str, str]:
    """Return an environment dict for invoking the locally-built opam.

    Prepends our opam's `bin/` to `PATH`, points `OPAMROOT` at the
    project-local opam state directory, and sets `OPAMYES=1` so opam
    treats every interactive confirmation as a yes. This keeps the build
    non-interactive without us having to thread `--yes` through every
    individual command.
    """
    env = {**os.environ}
    env['PATH'] = os.pathsep.join(
        [str(OPAM_INSTALL_DIR / 'bin'),
         env.get('PATH', '')])
    env['OPAMROOT'] = str(OPAMROOT_DIR)
    env['OPAMYES'] = '1'
    return env


def _init_opam(env: dict[str, str]) -> None:
    """Initialise OPAMROOT and create the comby switch if needed.

    `opam init --bare` sets up the root without creating a default
    switch, then we create a dedicated `comby` switch pinned to
    `OCAML_VERSION`. Sandboxing is disabled because bwrap (the Linux
    sandbox helper opam shells out to) is not part of our hermetic
    toolchain and may not be installed on the user's host.
    """
    if not (OPAMROOT_DIR / 'config').is_file():
        logging.info('Initialising OPAMROOT at %s', OPAMROOT_DIR)
        # `default <url>` makes opam use the pinned opam-repository
        # snapshot as the default repo, instead of the live HEAD of
        # opam-repository it would otherwise fetch.
        pinned_repo = f'git+{OPAM_REPO_URL}#{OPAM_REPO_REF}'
        terminal.run([
            str(OPAM_BIN), 'init', '--bare', '--no-setup',
            '--disable-sandboxing', 'default', pinned_repo
        ],
                     env=env,
                     interactive=True)
    else:
        logging.info('OPAMROOT already initialised at %s', OPAMROOT_DIR)

    # `switch list --short` is the one call here we *do* want to
    # capture: the parsed stdout drives the "create switch if missing"
    # branch below.
    switches = terminal.run([str(OPAM_BIN), 'switch', 'list', '--short'],
                            env=env).stdout
    if OPAM_SWITCH_NAME in switches.split():
        logging.info('opam switch %s already exists', OPAM_SWITCH_NAME)
        return

    logging.info('Creating opam switch %s with OCaml %s', OPAM_SWITCH_NAME,
                 OCAML_VERSION)
    terminal.run([
        str(OPAM_BIN), 'switch', 'create', OPAM_SWITCH_NAME, OCAML_VERSION,
        '--no-switch'
    ],
                 env=env,
                 interactive=True)


def _clone_comby() -> None:
    """Clone comby into `COMBY_SRC_DIR` if it is not already there.

    A pre-existing directory is left untouched -- the user may have
    local edits or be on a custom branch. To force a fresh clone, pass
    `--clean` or delete `third_party/comby/` manually.
    """
    if COMBY_SRC_DIR.is_dir():
        logging.info('comby source already present at %s', COMBY_SRC_DIR)
        return

    COMBY_SRC_DIR.parent.mkdir(parents=True, exist_ok=True)
    logging.info('Cloning comby (%s) into %s', COMBY_REF, COMBY_SRC_DIR)
    terminal.run([
        'git', 'clone', '--depth=1', '--branch', COMBY_REF, COMBY_GIT_URL,
        str(COMBY_SRC_DIR)
    ],
                 interactive=True)


def _build_comby(env: dict[str, str], jobs: int) -> None:
    """Install comby's deps in the switch, build it, and install the
    binary under `COMBY_TOOLCHAIN_DIR`.
    """
    if not COMBY_SRC_DIR.is_dir():
        raise RuntimeError(f'comby source not found at {COMBY_SRC_DIR}.')

    # Pin every subsequent opam command to our switch so we don't depend
    # on the user having sourced `opam env`.
    switch_args = ['--switch', OPAM_SWITCH_NAME]

    logging.info('Installing comby build dependencies into switch %s',
                 OPAM_SWITCH_NAME)
    # Resolve deps against opam-repository's *curated* metadata for the
    # comby packages, not the source tree's `.opam` files. The curated
    # versions in opam-repository carry upper bounds (`yojson < 2.0`,
    # `dune < 3.13`, `tls < 1.0.0`, etc.) that have been added post-hoc
    # to keep comby buildable as its deps moved on; the source-tree
    # `.opam` files do not. `--deps-only` with the package names listed
    # excludes the listed packages themselves from being installed, so
    # comby / comby-kernel / comby-semantic still come from our source
    # clone via `make release` below.
    package_versions = [f'{p}.{COMBY_REF}' for p in COMBY_PACKAGES]
    terminal.run([
        str(OPAM_BIN), 'install', *package_versions, '--deps-only',
        f'--jobs={jobs}', *switch_args
    ],
                 env=env,
                 interactive=True)

    logging.info('Building comby (release profile)')
    # `opam exec --` runs the command with the switch's environment
    # (PATH, OCAMLPATH, ...) applied, so `dune` / `make` find the
    # compiler and libraries we just installed.
    terminal.run(
        [str(OPAM_BIN), 'exec', *switch_args, '--', 'make', 'release'],
        cwd=COMBY_SRC_DIR,
        env=env,
        interactive=True)

    logging.info('Installing comby to %s', COMBY_TOOLCHAIN_DIR)
    COMBY_TOOLCHAIN_DIR.mkdir(parents=True, exist_ok=True)
    # `dune install --prefix` redirects the install from the switch's
    # default location into our own tree. This is the same mechanism
    # comby's own `make install` uses, with the prefix pinned.
    terminal.run([
        str(OPAM_BIN), 'exec', *switch_args, '--', 'dune', 'install',
        f'--prefix={COMBY_TOOLCHAIN_DIR}', 'comby'
    ],
                 cwd=COMBY_SRC_DIR,
                 env=env,
                 interactive=True)

    if not COMBY_BIN.is_file():
        raise RuntimeError(
            f'comby install finished but binary not found at {COMBY_BIN}')


def _clean() -> None:
    """Remove the comby source clone and both toolchain directories.

    All three are recreated from scratch on the next run: the clone via
    `_clone_comby`, the intermediate tree by the opam steps, and the
    final install by `dune install`.
    """
    for path in (COMBY_SRC_DIR, COMBY_INTERMEDIATE_DIR, COMBY_TOOLCHAIN_DIR):
        if path.exists():
            logging.info('Removing %s', path)
            shutil.rmtree(path)


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Build a local opam and use it to build comby.')
    parser.add_argument(
        '--clean',
        action='store_true',
        help='Remove third_party/comby-src/, third_party/comby-toolchain/ and '
        'third_party/comby-toolchain-intermediate/ before building (forces a '
        'fresh clone and rebuild).')
    parser.add_argument(
        '--skip-opam',
        action='store_true',
        help='Skip the opam download/build/install step (assumes opam is '
        'already present at the expected path).')
    parser.add_argument(
        '--skip-comby',
        action='store_true',
        help='Skip the comby build step. Useful for rebuilding '
        'opam in isolation.')
    parser.add_argument('-j',
                        '--jobs',
                        type=int,
                        default=os.cpu_count() or 1,
                        help='Number of parallel build jobs (default: nproc).')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable debug logging.')
    args = parser.parse_args()

    if sys.platform == 'win32':
        parser.error(
            'Windows is not supported by this script; build comby under WSL '
            'as described in third_party/comby/README.md.')

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        force=True)

    if args.clean:
        _clean()

    if not args.skip_opam:
        _download_opam()
        _extract_opam()
        _build_opam(args.jobs)

    if not OPAM_BIN.is_file():
        raise RuntimeError(
            f'opam binary not found at {OPAM_BIN}; re-run without '
            f'--skip-opam.')

    env = _opam_env()
    _init_opam(env)

    if not args.skip_comby:
        _clone_comby()
        _build_comby(env, args.jobs)

    logging.info('Done.')
    logging.info('opam:  %s', OPAM_BIN)
    if COMBY_BIN.is_file():
        logging.info('comby: %s', COMBY_BIN)
    return 0


if __name__ == '__main__':
    sys.exit(main())
