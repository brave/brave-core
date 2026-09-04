#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build a local `opam` and use it to build `comby`.

Self-contained: depends only on the Python standard library and `git`,
so it can be downloaded and run on a CI machine that does not have a
full Brave / Chromium checkout, in the same spirit as the scripts under
`tools/cr/toolchains/`.

Platform notes:

  * Linux / macOS / WSL: builds comby with `hack_parallel` (Hack's
    shared-memory + parallelism runtime) for the parallel pipeline.
    WSL identifies as Linux to Python, so it follows the same path.
  * Native Windows: opam itself is downloaded as a prebuilt binary
    from opam's GitHub release page (`OPAM_WINDOWS_BINARY_URL`, pinned
    to the same `OPAM_VERSION` as the from-source build on the other
    platforms), so no `make` / posix-toolchain bootstrap is required
    for opam. `hack_parallel` -- Hack's shared-memory + parallelism
    runtime -- uses Unix-only primitives and is not buildable on
    native Windows; comby's `dune` files already carry `select` rules
    that pick a `parany`-based fallback when `hack_parallel` is not
    installed in the switch, so we patch comby's curated `.opam` (in
    our local opam-repository clone) to filter out the dep on
    `os = "win32"`. The resulting comby is functionally complete,
    just without the parallel pipeline.

opam-repository itself is always cloned locally (into
`<third-party>/comby-toolchain-intermediate/opam-repository/`) rather
than handed to opam as a git URL, so the curated metadata has one
source of truth on every platform and the Windows overlay slots in
without touching opam's view of the repo.

```sh
curl -sL \\
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/comby/build_comby.py \\
    | python3 - --third-party-dir=/path/to/third_party/
```

Locally, `--third-party-dir` defaults to `brave/third_party/` (resolved
from the script's own location), so the usual invocation needs no flags:

```sh
tools/cr/comby/build_comby.py
```

This script is designed to run both locally and on CI with just a one-liner,
nothing else required, and then installed in isolation. `opam` is cloned from
a pinned git tag, built via its cold-bootstrap target (which vendors its own
OCaml compiler), and installed under
`<third-party>/comby-toolchain-intermediate/opam-install/` (it is intermediate
because it exists only to drive the comby build). That opam binary is then
driven against a project-local `OPAMROOT` to create an OCaml switch and build
`comby` from a checkout at `<third-party>/comby-src/` (cloned automatically if
missing). The final comby install lands at `<third-party>/comby-toolchain/`.

Key paths (relative to `--third-party-dir`):

  * `comby-src/`                                comby source
  * `comby-toolchain-intermediate/opam-<v>/`    opam source
  * `comby-toolchain-intermediate/opam-install/bin/opam`
  * `comby-toolchain-intermediate/opamroot/`    OPAMROOT
  * `comby-toolchain/bin/comby`                 final binary

The user is expected to have the OS-level build dependencies listed in
`tools/cr/comby/README.md` already installed. Check `README.md` for any
platform-specific notes and troubleshooting tips.
"""

from __future__ import annotations

import argparse
import hashlib
import logging
import os
import platform
import shutil
import subprocess
import sys
import urllib.request
from pathlib import Path

# Pinned opam release. Used in two ways:
#   * Linux / macOS: cloned as a shallow checkout of the tag from
#     `OPAM_GIT_URL` and built from source via `make cold` -- the
#     vendored OCaml compiler keeps the bootstrap hermetic.
#   * Windows: downloaded as a prebuilt binary from
#     `OPAM_WINDOWS_BINARY_URL` (see release page below). Building from
#     source on Windows would need GNU `make` in a posix env (msys2 /
#     cygwin), which the prebuilt avoids entirely. The URL embeds
#     `OPAM_VERSION` so the Windows binary and the from-source build on
#     other platforms are always the same opam release.
OPAM_VERSION = '2.5.1'
OPAM_GIT_URL = 'https://github.com/ocaml/opam.git'
OPAM_WINDOWS_BINARY_URL = (
    f'https://github.com/ocaml/opam/releases/download/{OPAM_VERSION}/'
    f'opam-{OPAM_VERSION}-x86_64-windows.exe')
# SHA-256 of the Windows binary at the URL above. Pinned so an
# upstream rewrite of the release asset (which would be unusual but
# possible) fails the build loudly instead of silently shipping a
# different binary. Bump this together with `OPAM_VERSION`.
OPAM_WINDOWS_BINARY_SHA256 = (
    'b3e72e15333cbddfce4b70e3b0840c3cf142e6b56bd6f3d6c3cd96f30b7626d7')

# OCaml compiler version used for the comby switch. comby's `comby.opam`
# requires `ocaml >= 4.08.1`; 4.14.2 is a current 4.x LTS.
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

# Name of the opam switch we create under OPAMROOT.
OPAM_SWITCH_NAME = 'comby'

# Executable suffix for binaries we install (`opam`, `comby`).
_EXE = '.exe' if platform.system() == 'Windows' else ''


class Paths:
    """All filesystem paths derived from `--third-party-dir`.

    The layout mirrors `tools/rust/build_rust.py`:

      * `<name>-src/`                       source clone
      * `<name>-toolchain-intermediate/`    build-time-only artifacts
      * `<name>-toolchain/`                 final install prefix

    opam falls under "toolchain-intermediate" because it only exists to
    drive the comby build.
    """

    def __init__(self, third_party: Path):
        # Resolve to an absolute path: the build shells out into subprocesses
        # whose cwd is set to a subdirectory (e.g. `opam_src`). Any relative
        # path passed in `--prefix=` / `OPAMROOT` / etc. would then resolve
        # against the subprocess's cwd instead of ours.
        third_party = third_party.expanduser().resolve()
        self.third_party: Path = third_party
        self.comby_src: Path = third_party / 'comby-src'
        self.comby_toolchain: Path = third_party / 'comby-toolchain'
        self.comby_intermediate: Path = (third_party /
                                         'comby-toolchain-intermediate')
        self.opam_src: Path = self.comby_intermediate / f'opam-{OPAM_VERSION}'
        self.opam_install: Path = self.comby_intermediate / 'opam-install'
        self.opam_bin: Path = self.opam_install / 'bin' / f'opam{_EXE}'
        self.opamroot: Path = self.comby_intermediate / 'opamroot'
        # Local clone of opam-repository at the pinned ref. Used as opam's
        # `default` repo on all platforms (so the curated metadata is sourced
        # from one place we own) and as the host for the Windows-only
        # `hack_parallel` overlay. See `_resolve_opam_repo_address` and
        # `_patch_comby_opam_for_windows`.
        self.opam_repository: Path = (self.comby_intermediate /
                                      'opam-repository')
        self.comby_bin: Path = self.comby_toolchain / 'bin' / f'comby{_EXE}'


def _default_third_party_dir() -> Path | None:
    """Return `<brave>/third_party/` if this script lives inside a Brave
    checkout, otherwise `None`.

    Returns `None` when the script has no `__file__` (e.g.
    `curl ... | python3 -`) or when it has been copied to a path that
    does not end in `tools/cr/comby/`. In those cases the caller must
    pass `--third-party-dir` explicitly.
    """
    try:
        script = Path(__file__).resolve()
    except NameError:
        return None
    # tools/cr/comby/build_comby.py -> tools/cr/comby -> tools/cr ->
    # tools -> brave.
    if (script.parent.name != 'comby' or script.parent.parent.name != 'cr'
            or script.parent.parent.parent.name != 'tools'):
        return None
    return script.parent.parent.parent.parent / 'third_party'


def _run(cmd: list[str | Path],
         *,
         cwd: Path | None = None,
         env: dict[str, str] | None = None,
         capture: bool = False) -> subprocess.CompletedProcess:
    """Run *cmd* as a subprocess, logging the invocation.
    """
    str_cmd = [str(c) for c in cmd]
    logging.info(' >>>> %s', ' '.join(str_cmd))

    if platform.system() == 'Windows':
        # On Windows, resolve the command to an absolute path to avoid
        # issues with bat/cmd wrappers not matching the bare command name
        # (e.g. `git` vs `git.cmd`). This avoids the use of `shell=True`.
        resolved = shutil.which(str_cmd[0])
        if resolved is None:
            raise RuntimeError(f'Command not found: {str_cmd[0]}')
        if resolved != str_cmd[0]:
            str_cmd = [resolved] + str_cmd[1:]

    if capture:
        return subprocess.run(str_cmd,
                              check=True,
                              cwd=cwd,
                              env=env,
                              capture_output=True,
                              text=True,
                              encoding='utf-8')
    return subprocess.run(str_cmd, check=True, cwd=cwd, env=env)


def _clone_opam(paths: Paths) -> None:
    """Shallow-clone opam at the pinned tag into `paths.opam_src`.

    Skipped if the source directory already exists. Used only on
    Linux / macOS; on Windows we download a prebuilt binary instead --
    see `_download_opam_binary`.
    """
    if paths.opam_src.is_dir():
        logging.info('opam source already present at %s', paths.opam_src)
        return

    paths.opam_src.parent.mkdir(parents=True, exist_ok=True)
    logging.info('Cloning opam (%s) into %s', OPAM_VERSION, paths.opam_src)
    _run([
        'git', 'clone', '--depth=1', '--branch', OPAM_VERSION, OPAM_GIT_URL,
        paths.opam_src
    ])


def _download_opam_binary(paths: Paths) -> None:
    """Download the prebuilt opam binary for native Windows.

    Replaces the `_clone_opam` + `_build_opam` pair on Windows. The
    binary is fetched from opam's official GitHub release at
    `OPAM_WINDOWS_BINARY_URL`, which embeds `OPAM_VERSION` -- the
    same constant the Linux / macOS path builds from source -- so the
    opam used across platforms is always the same release.

    Building opam from source on Windows would need GNU `make` (and a
    posix shell + coreutils) inside an msys2 / cygwin environment. The
    prebuilt sidesteps all of that. Trust posture mirrors the official
    `shell/install.sh` upstream installer, which fetches the same
    binaries from the same release URL.

    Streams to a `.partial` file and renames on success so an
    interrupted download never leaves a half-written file at the final
    path.
    """
    if paths.opam_bin.is_file():
        logging.info('opam binary already present at %s', paths.opam_bin)
        return

    paths.opam_bin.parent.mkdir(parents=True, exist_ok=True)
    logging.info('Downloading opam %s from %s', OPAM_VERSION,
                 OPAM_WINDOWS_BINARY_URL)
    tmp_path = paths.opam_bin.with_suffix(paths.opam_bin.suffix + '.partial')
    urllib.request.urlretrieve(OPAM_WINDOWS_BINARY_URL, tmp_path)

    # Verify against the pinned hash before promoting to the final
    # path. A mismatch means the upstream asset was rewritten (or the
    # download was tampered with in transit) -- either way we abort
    # rather than silently shipping a different binary, and delete the
    # partial so the next run re-attempts the download.
    actual_sha256 = hashlib.sha256(tmp_path.read_bytes()).hexdigest()
    if actual_sha256 != OPAM_WINDOWS_BINARY_SHA256:
        tmp_path.unlink()
        raise RuntimeError(
            f'sha256 mismatch for {OPAM_WINDOWS_BINARY_URL}: expected '
            f'{OPAM_WINDOWS_BINARY_SHA256}, got {actual_sha256}. '
            f'If the upstream release asset was intentionally updated, '
            f'bump OPAM_VERSION / OPAM_WINDOWS_BINARY_SHA256 to match.')

    tmp_path.replace(paths.opam_bin)


def _build_opam(paths: Paths, jobs: int) -> None:
    """Cold-bootstrap opam and install it under `paths.opam_install`.

    Uses opam's `make cold` target, which downloads and builds a vendored
    OCaml compiler before building opam itself. This avoids any
    requirement on a system OCaml install.
    """
    if paths.opam_bin.is_file():
        logging.info('opam already built at %s', paths.opam_bin)
        return

    paths.opam_install.mkdir(parents=True, exist_ok=True)
    # `make cold` takes the configure arguments via CONFIGURE_ARGS. We
    # pin the install prefix to our local directory so nothing escapes
    # into the user's system.
    configure_args = f'--prefix={paths.opam_install}'
    _run(['make', 'cold', f'CONFIGURE_ARGS={configure_args}', f'-j{jobs}'],
         cwd=paths.opam_src)
    _run(['make', 'cold-install'], cwd=paths.opam_src)

    if not paths.opam_bin.is_file():
        raise RuntimeError(
            f'opam build finished but binary not found at {paths.opam_bin}')


def _opam_env(paths: Paths) -> dict[str, str]:
    """Return an environment dict for invoking the locally-built opam.

    Prepends our opam's `bin/` to `PATH`, points `OPAMROOT` at the
    project-local opam state directory, and sets `OPAMYES=1` so opam
    treats every interactive confirmation as a yes. This keeps the build
    non-interactive without us having to thread `--yes` through every
    individual command.

    On Windows we also prepend the cross-target binutils dir from
    opam's managed Cygwin install. mingw-w64 gcc spawns its assembler
    by the bare name `as` (not the prefixed `x86_64-w64-mingw32-as`),
    and the only unprefixed `as.exe` lives in that cross-sysroot bin
    dir -- the switch's `bin/` only carries the prefixed shims. Without
    this, package compiles fail with
    `cannot execute 'as': spawn: No such file or directory`.
    """
    env = {**os.environ}
    path_entries = [str(paths.opam_install / 'bin')]
    if platform.system() == 'Windows':
        path_entries.append(
            str(paths.opamroot / '.cygwin' / 'root' / 'usr' /
                'x86_64-w64-mingw32' / 'bin'))
    path_entries.append(env.get('PATH', ''))
    env['PATH'] = os.pathsep.join(path_entries)
    env['OPAMROOT'] = str(paths.opamroot)
    env['OPAMYES'] = '1'
    return env


# Substitutions applied to comby.<COMBY_REF>'s curated `.opam` on
# native Windows builds. Each entry maps a dep line that fails to
# resolve on `os = "win32"` to its Windows-friendly rewrite.
#
#   * `hack_parallel`: Hack's shared-memory + parallelism runtime,
#     Unix-only. comby's `src/dune` and `lib/app/pipeline/dune` carry
#     `select` rules that pick a `parany`-based fallback when the
#     package is not installed in the switch, so dropping the dep is
#     sufficient -- comby still builds, just without the parallel
#     pipeline.
#   * `conf-libev`: probes for system libev, declared
#     `available: os != "win32"` upstream. lwt builds without the
#     libev backend and falls back to its built-in select-based event
#     loop, which is what comby uses indirectly through `lwt-unix`.
_COMBY_OPAM_WINDOWS_SUBS: tuple[tuple[str, str], ...] = (
    (
        '"hack_parallel" {arch != "arm32" & arch != "arm64"}',
        '"hack_parallel" {arch != "arm32" & arch != "arm64" & os != "win32"}',
    ),
    (
        '"conf-libev" {os-distribution != "ol"}',
        '"conf-libev" {os-distribution != "ol" & os != "win32"}',
    ),
)

# Lines deleted outright from comby's curated `.opam` on Windows. Same
# motivation as `_COMBY_OPAM_WINDOWS_SUBS`, but the dep can't be
# salvaged with a filter -- removing it is the only option.
#
#   * `parany`: parallel map/fold library. Declared
#     `available: os != "win32"` and depends on `Unix.fork`, so opam
#     can't install it on Windows. With `hack_parallel` already
#     filtered out, comby's `parallel_hack.parany_fallback.ml` would
#     normally use it; `_patch_comby_src_for_windows` overwrites that
#     file with a sequential `List.fold` so the dep is no longer
#     referenced at build time, and we drop the opam dep here so the
#     dep solver stops complaining.
_COMBY_OPAM_WINDOWS_DELETIONS: tuple[str,
                                     ...] = ('  "parany" {>= "12.0.3"}\n', )


def _patch_comby_opam_for_windows(repo_dir: Path) -> None:
    """Apply `_COMBY_OPAM_WINDOWS_SUBS` + `_COMBY_OPAM_WINDOWS_DELETIONS`
    to comby.<COMBY_REF>'s curated `.opam` so deps that aren't
    satisfiable on `os = "win32"` drop out of the solve.

    Both kinds of patches are idempotent: running this twice is a
    no-op. Substitutions are required (a missing needle raises, so a
    drifting upstream pin is loud); deletions are best-effort (a
    missing needle is silently skipped, since "already deleted" and
    "never present" look the same to a text search).
    """
    opam_path = (repo_dir / 'packages' / 'comby' / f'comby.{COMBY_REF}' /
                 'opam')
    # Git for Windows defaults `core.autocrlf=true`, so checkouts of
    # opam-repository land on disk with CRLF endings. Our deletion
    # needles include `\n` and would silently fail to match against
    # `\r\n`; normalize once so every patch sees LF endings, and write
    # back with `newline=''` so the file stays LF on disk.
    raw = opam_path.read_bytes().decode('utf-8')
    text = raw.replace('\r\n', '\n')
    patched = text

    for needle, replacement in _COMBY_OPAM_WINDOWS_SUBS:
        if replacement in patched:
            continue
        if needle not in patched:
            raise RuntimeError(
                f'Expected dep line not found in {opam_path}: {needle!r}. '
                f'Has the pinned opam-repository ref changed?')
        patched = patched.replace(needle, replacement)

    for line in _COMBY_OPAM_WINDOWS_DELETIONS:
        patched = patched.replace(line, '')

    if patched == raw:
        logging.info('comby.%s opam already patched for Windows', COMBY_REF)
        return

    opam_path.write_text(patched, encoding='utf-8', newline='')
    logging.info('Patched %s for Windows', opam_path)


# Unified diff applied to base_bigstring v0.14.0's C stubs on Windows.
# v0.14 unconditionally `#include <endian.h>` on any platform that
# isn't macOS/glibc/OpenBSD/Cygwin -- mingw-w64 (which opam uses as
# its Windows C toolchain) doesn't ship `endian.h`, so the build dies
# with `endian.h: No such file or directory`. We backport the
# `__MINGW32__` branch Jane Street added in v0.17.0, which sidesteps
# the header entirely by using GCC's `__builtin_bswap*` intrinsics.
_BASE_BIGSTRING_MINGW_PATCH = '''\
--- a/src/base_bigstring_stubs.c
+++ b/src/base_bigstring_stubs.c
@@ -31,6 +31,10 @@
 #define bswap_64 swap64
 #elif __CYGWIN__
 #include <endian.h>
+#elif __MINGW32__
+#define bswap_16 __builtin_bswap16
+#define bswap_32 __builtin_bswap32
+#define bswap_64 __builtin_bswap64
 #else
 #include <sys/types.h>
 #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
'''

# Unified diff applied to core_kernel v0.14.2 on Windows. Two
# coordinated fixes in one patch file:
#
#   1) `src/dune` -- the upstream rule uses
#      `(action (bash "cp X Y ."))`, which on Windows expands
#      `%{lib:jst-config:config.h}` to a Windows path like
#      `C:\Users\...\config.h`. opam invokes bash via Cygwin and hands
#      it the command via `bash -c "..."`; bash then treats the
#      backslashes inside the unquoted argument as escape characters
#      and strips them, turning the path into `C:Usersconfig.h` and
#      failing with `cp: cannot stat ...`. Switching to dune's native
#      `(copy)` action avoids the shell entirely.
#   2) `src/bigstring_stubs.c` -- same upstream `endian.h`-include
#      gap as `base_bigstring`'s (see `_BASE_BIGSTRING_MINGW_PATCH`).
#      core_kernel ships its own near-identical copy of the
#      byte-order detection block. We backport the same v0.17
#      `__MINGW32__` branch using `__builtin_bswap*`.
_CORE_KERNEL_WINDOWS_PATCH = '''\
--- a/src/dune
+++ b/src/dune
@@ -1,4 +1,5 @@
 (rule (targets config.h rt-flags) (deps)
- (action (bash "cp %{lib:jst-config:config.h} %{lib:jst-config:rt-flags} .")))
+ (action (progn (copy %{lib:jst-config:config.h} config.h)
+                (copy %{lib:jst-config:rt-flags} rt-flags))))

 (library (name core_kernel) (public_name core_kernel)
--- a/src/bigstring_stubs.c
+++ b/src/bigstring_stubs.c
@@ -29,6 +29,10 @@
 #define bswap_64 swap64
 #elif __CYGWIN__
 #include <endian.h>
+#elif __MINGW32__
+#define bswap_16 __builtin_bswap16
+#define bswap_32 __builtin_bswap32
+#define bswap_64 __builtin_bswap64
 #else
 #include <sys/types.h>
 #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
'''

# Source-level patches applied to packages in our local opam-repository
# clone on native Windows builds. Each entry: `(pkg, ver, filename, diff)`.
# `_register_opam_windows_patch` writes the file, declares it via
# `patches:` + `extra-files:` (sha256), and is idempotent on re-runs.
#
# Why we keep accumulating these: comby v1.7.0 transitively pins the
# Jane Street v0.14 series, which predates JS's Windows port work
# (v0.15-v0.17). Each Windows-portability fix Jane Street landed in a
# newer series has to be backported here as a small patch.
_OPAM_WINDOWS_PATCHES: tuple[tuple[str, str, str, str], ...] = (
    ('base_bigstring', 'v0.14.0', 'mingw-bswap.patch',
     _BASE_BIGSTRING_MINGW_PATCH),
    ('core_kernel', 'v0.14.2', 'windows-fixes.patch',
     _CORE_KERNEL_WINDOWS_PATCH),
)


def _register_opam_windows_patch(repo_dir: Path, pkg: str, ver: str,
                                 patch_filename: str,
                                 patch_content: str) -> None:
    """Register a Windows-only source patch against one opam-repository
    package in our local clone.

    Three coordinated edits, all idempotent:

      1. Write `patch_content` (utf-8 bytes) to the package's
         `files/<patch_filename>`.
      2. Insert `patches: ["<patch_filename>" {os = "win32"}]` into
         the package's `opam` file -- opam reads this directive during
         the build phase and runs `patch -p1 < <patch_filename>`.
      3. Insert `extra-files: [["<patch_filename>" "sha256=..."]]`
         alongside. opam 2.1+ refuses to copy a file from `files/`
         into the build dir unless it's declared (with a checksum) via
         `extra-files:` -- without this declaration opam treats the
         file as not part of the package and the patch step fails with
         `Patch file ... not found`. The sha256 is computed from the
         exact bytes written in step 1, so editing `patch_content` and
         re-running updates the file and the declaration in lockstep.
    """
    pkg_dir = repo_dir / 'packages' / pkg / f'{pkg}.{ver}'

    # 1) Write the diff file as exact bytes (so the sha256 below
    # describes what's actually on disk regardless of platform line
    # endings). Always overwrite so an edit to `patch_content`
    # propagates without --clean.
    files_dir = pkg_dir / 'files'
    files_dir.mkdir(parents=True, exist_ok=True)
    patch_path = files_dir / patch_filename
    patch_bytes = patch_content.encode('utf-8')
    patch_path.write_bytes(patch_bytes)
    patch_sha256 = hashlib.sha256(patch_bytes).hexdigest()

    # 2) Build the declarations we want present in the opam file.
    new_patches = f'patches: ["{patch_filename}" {{os = "win32"}}]'
    new_extra = (f'extra-files: [["{patch_filename}" '
                 f'"sha256={patch_sha256}"]]')

    # 3) Read the opam file (CRLF-normalized so our LF-using needles
    # match against a Git-for-Windows checkout).
    opam_path = pkg_dir / 'opam'
    raw = opam_path.read_bytes().decode('utf-8')
    text = raw.replace('\r\n', '\n')

    if new_patches in text and new_extra in text:
        return

    # 4) Strip any prior brave-written declarations for this patch
    # (line-based so we don't rely on a specific format from an older
    # version of this script). A previous run may have written just
    # `patches:` without `extra-files:`, or with a stale checksum.
    stripped_lines = [
        line for line in text.split('\n')
        if not (line.startswith('patches:') and patch_filename in line)
        and not (line.startswith('extra-files:') and patch_filename in line)
    ]
    text = '\n'.join(stripped_lines)

    # 5) Insert fresh declarations before the build stanza.
    needle = 'build: [\n  ["dune" "build" "-p" name "-j" jobs]\n]'
    if needle not in text:
        raise RuntimeError(
            f'Expected build stanza not found in {opam_path}; has the '
            f'pinned opam-repository ref changed?')
    replacement = f'{new_patches}\n{new_extra}\n{needle}'
    text = text.replace(needle, replacement)

    opam_path.write_text(text, encoding='utf-8', newline='')
    logging.info('Patched %s: registered %s (sha256=%s)', opam_path,
                 patch_filename, patch_sha256)


def _apply_opam_windows_patches(repo_dir: Path) -> None:
    """Walk `_OPAM_WINDOWS_PATCHES` and register each entry against
    the local opam-repository clone. See `_register_opam_windows_patch`
    for the per-entry mechanics.
    """
    for pkg, ver, filename, content in _OPAM_WINDOWS_PATCHES:
        _register_opam_windows_patch(repo_dir, pkg, ver, filename, content)


# Replacement contents for `lib/app/pipeline/parallel_hack.parany_fallback.ml`
# on native Windows. The upstream file uses `Parany.Parmap.parfold`, and
# parany is unavailable on Windows (Unix-only, uses `Unix.fork`). We
# preserve the same module interface but compute the result with a
# plain `List.fold` -- sequential, but correct. See
# `_patch_comby_src_for_windows`.
_COMBY_PARANY_FALLBACK_WINDOWS = '''\
(* Windows-only sequential replacement for `parany_fallback.ml`,
   installed by build_comby.py's `_patch_comby_src_for_windows`.
   `Parany` is unavailable on native Windows (uses `Unix.fork`), so
   the parany-based fallback can't compile here. comby's pipeline
   still runs, just sequentially. *)

open Core

open Configuration
open Command_input

let debug =
  Sys.getenv "DEBUG_COMBY"
  |> Option.is_some

let process_interactive ~f paths _number_of_workers =
  if debug then
    Format.printf "[*] Hack_parallel unavailable. Sequential fallback.@.";
  let reduce (acc, c) (path, result) =
    match result with
    | Some rewritten_source, c' ->
      Interactive.{path; rewritten_source}::acc, c+c'
    | None, c' ->
      acc, c+c'
  in
  let init = ([], 0) in
  let map path = path, f ~input:(Path path) ~path:(Some path) in
  List.fold paths ~init ~f:(fun acc path -> reduce acc (map path))

let process ~f _number_of_workers _bound_count sources =
  if debug then
    Format.printf "[*] Hack_parallel unavailable. Sequential fallback.@.";
  match sources with
  | `Paths paths ->
    List.fold paths ~init:0
      ~f:(fun acc path -> acc + f ~input:(Path path) ~output_path:(Some path))
  | `Zip _ -> failwith "Not supported"
'''


def _patch_comby_src_for_windows(comby_src: Path) -> None:
    """Patch comby's source tree so the build does not require `parany`.

    Two coordinated edits, both idempotent:

      1. `lib/app/pipeline/dune` -- drop `parany` from the pipeline
         library's `(libraries ...)` list so dune stops looking for
         the dependency at link time.
      2. `lib/app/pipeline/parallel_hack.parany_fallback.ml` --
         overwrite with `_COMBY_PARANY_FALLBACK_WINDOWS` so the file
         no longer references `Parany.Parmap.parfold`. dune's `select`
         rule still picks this file when `hack_parallel` is absent;
         the new body computes the same result with a plain
         `List.fold`.

    Combined with the `parany` removal from `_COMBY_OPAM_WINDOWS_DELETIONS`,
    this makes comby buildable end-to-end on Windows without any
    parallelism libraries.
    """
    pipeline_dir = comby_src / 'lib' / 'app' / 'pipeline'

    # 1. Drop parany from the pipeline library's deps.
    dune_path = pipeline_dir / 'dune'
    dune_text = dune_path.read_bytes().decode('utf-8')
    needle = ' ppx_deriving_yojson parany'
    replacement = ' ppx_deriving_yojson'
    if needle in dune_text:
        dune_path.write_text(dune_text.replace(needle, replacement),
                             encoding='utf-8',
                             newline='')
        logging.info('Patched %s: removed parany from libraries', dune_path)
    elif replacement not in dune_text:
        raise RuntimeError(
            f'Expected libraries line not found in {dune_path}; has the '
            f'pinned comby ref changed?')

    # 2. Replace the parany-based fallback with a sequential one.
    fallback_path = pipeline_dir / 'parallel_hack.parany_fallback.ml'
    current = fallback_path.read_bytes().decode('utf-8').replace('\r\n', '\n')
    if current == _COMBY_PARANY_FALLBACK_WINDOWS:
        return
    fallback_path.write_text(_COMBY_PARANY_FALLBACK_WINDOWS,
                             encoding='utf-8',
                             newline='')
    logging.info('Patched %s: sequential body (no parany)', fallback_path)


def _resolve_opam_repo_address(paths: Paths) -> str:
    """Clone opam-repository locally, apply any platform overlays, and
    return the address `opam init` should use for its `default` repo.

    We always clone locally (rather than handing opam a `git+URL#REF`
    and letting it clone itself into OPAMROOT) so the source of truth
    for the curated metadata is one place we own, and platform-specific
    overlays slot in without touching opam's view. opam treats a local
    path as an rsync-style repo and syncs it into OPAMROOT on init.

    The overlays only apply on Windows -- see
    `_patch_comby_opam_for_windows` and `_apply_opam_windows_patches`
    for the rationale. On other platforms the clone is used verbatim.
    """
    if not paths.opam_repository.is_dir():
        paths.opam_repository.parent.mkdir(parents=True, exist_ok=True)
        logging.info('Cloning opam-repository (%s) into %s', OPAM_REPO_REF,
                     paths.opam_repository)
        _run([
            'git', 'clone', '--depth=1', '--branch', OPAM_REPO_REF,
            OPAM_REPO_URL, paths.opam_repository
        ])
    else:
        logging.info('opam-repository already cloned at %s',
                     paths.opam_repository)
    if platform.system() == 'Windows':
        _patch_comby_opam_for_windows(paths.opam_repository)
        _apply_opam_windows_patches(paths.opam_repository)
    return str(paths.opam_repository)


def _init_opam(paths: Paths, env: dict[str, str]) -> None:
    """Initialise OPAMROOT and create the comby switch if needed.

    `opam init --bare` sets up the root without creating a default
    switch, then we create a dedicated `comby` switch pinned to
    `OCAML_VERSION`. Sandboxing is disabled because bwrap (the Linux
    sandbox helper opam shells out to) is not part of our hermetic
    toolchain and may not be installed on the user's host.
    """
    if not (paths.opamroot / 'config').is_file():
        logging.info('Initialising OPAMROOT at %s', paths.opamroot)
        # The address resolution varies by platform; see
        # `_resolve_opam_repo_address` for the Windows-specific overlay.
        repo_address = _resolve_opam_repo_address(paths)
        _run([
            paths.opam_bin, 'init', '--bare', '--no-setup',
            '--disable-sandboxing', 'default', repo_address
        ],
             env=env)
    else:
        logging.info('OPAMROOT already initialised at %s', paths.opamroot)
        # Re-sync opam's internal catalog from our local opam-repository
        # clone in case the source has been re-patched since the last
        # run (e.g. a new entry added to `_COMBY_OPAM_WINDOWS_PATCHES`).
        # Without this, opam keeps the snapshot it took on first `opam
        # init` and the dep solver never sees newly-applied patches
        # until the user passes `--clean`. The address resolution call
        # here only re-applies the overlay; the actual update is the
        # `opam update default` below.
        _resolve_opam_repo_address(paths)
        _run([paths.opam_bin, 'update', 'default'], env=env)

    # `switch list --short` is the one call here we *do* want to
    # capture: the parsed stdout drives the "create switch if missing"
    # branch below.
    switches = _run([paths.opam_bin, 'switch', 'list', '--short'],
                    env=env,
                    capture=True).stdout
    if OPAM_SWITCH_NAME in switches.split():
        logging.info('opam switch %s already exists', OPAM_SWITCH_NAME)
        return

    logging.info('Creating opam switch %s with OCaml %s', OPAM_SWITCH_NAME,
                 OCAML_VERSION)
    _run([
        paths.opam_bin, 'switch', 'create', OPAM_SWITCH_NAME, OCAML_VERSION,
        '--no-switch'
    ],
         env=env)


def _clone_comby(paths: Paths) -> None:
    """Clone comby into `paths.comby_src` if it is not already there,
    then apply any platform-specific source overlays.

    A pre-existing checkout is left untouched (modulo the overlays the
    user may have local edits or be on a custom branch); to force a
    fresh clone, pass `--clean` or delete the directory manually. The
    overlay step always runs and is idempotent -- see
    `_patch_comby_src_for_windows` for what it does on Windows.
    """
    if paths.comby_src.is_dir():
        logging.info('comby source already present at %s', paths.comby_src)
    else:
        paths.comby_src.parent.mkdir(parents=True, exist_ok=True)
        logging.info('Cloning comby (%s) into %s', COMBY_REF, paths.comby_src)
        _run([
            'git', 'clone', '--depth=1', '--branch', COMBY_REF, COMBY_GIT_URL,
            paths.comby_src
        ])

    if platform.system() == 'Windows':
        _patch_comby_src_for_windows(paths.comby_src)


def _build_comby(paths: Paths, env: dict[str, str], jobs: int) -> None:
    """Install comby's deps in the switch, build it, and install the
    binary under `paths.comby_toolchain`.
    """
    if not paths.comby_src.is_dir():
        raise RuntimeError(f'comby source not found at {paths.comby_src}.')

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
    _run([
        paths.opam_bin, 'install', *package_versions, '--deps-only',
        f'--jobs={jobs}', *switch_args
    ],
         env=env)

    logging.info('Building comby (release profile)')
    # `opam exec --` runs the command with the switch's environment
    # (PATH, OCAMLPATH, ...) applied, so `dune` / `make` find the
    # compiler and libraries we just installed.
    _run([paths.opam_bin, 'exec', *switch_args, '--', 'make', 'release'],
         cwd=paths.comby_src,
         env=env)

    logging.info('Installing comby to %s', paths.comby_toolchain)
    paths.comby_toolchain.mkdir(parents=True, exist_ok=True)
    # `dune install --prefix` redirects the install from the switch's
    # default location into our own tree. This is the same mechanism
    # comby's own `make install` uses, with the prefix pinned.
    _run([
        paths.opam_bin, 'exec', *switch_args, '--', 'dune', 'install',
        f'--prefix={paths.comby_toolchain}', 'comby'
    ],
         cwd=paths.comby_src,
         env=env)

    if not paths.comby_bin.is_file():
        raise RuntimeError(f'comby install finished but binary not found at '
                           f'{paths.comby_bin}')


def _clean(paths: Paths) -> None:
    """Remove the comby source clone and both toolchain directories.

    All three are recreated from scratch on the next run: the clone via
    `_clone_comby`, the intermediate tree by the opam steps, and the
    final install by `dune install`.
    """
    for path in (paths.comby_src, paths.comby_intermediate,
                 paths.comby_toolchain):
        if path.exists():
            logging.info('Removing %s', path)
            shutil.rmtree(path)


def main() -> int:
    default_third_party = _default_third_party_dir()
    parser = argparse.ArgumentParser(
        description='Build a local opam and use it to build comby.')
    parser.add_argument(
        '--third-party-dir',
        type=Path,
        default=default_third_party,
        required=default_third_party is None,
        help='Directory under which comby-src/, comby-toolchain/ and '
        'comby-toolchain-intermediate/ are created. Defaults to '
        'brave/third_party/ when this script runs from a Brave checkout; '
        'required otherwise (e.g. when piped from curl in CI).')
    parser.add_argument(
        '--clean',
        action='store_true',
        help='Remove comby-src/, comby-toolchain/ and '
        'comby-toolchain-intermediate/ under --third-party-dir before '
        'building (forces a fresh clone and rebuild).')
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

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        force=True)

    paths = Paths(args.third_party_dir)

    if args.clean:
        _clean(paths)

    if not args.skip_opam:
        if platform.system() == 'Windows':
            # On Windows we use the prebuilt binary published on opam's
            # GitHub release page -- same `OPAM_VERSION` as the
            # build-from-source path on Linux / macOS, no need for
            # `make` / posix tooling. See `_download_opam_binary`.
            _download_opam_binary(paths)
        else:
            _clone_opam(paths)
            _build_opam(paths, args.jobs)

    if not paths.opam_bin.is_file():
        raise RuntimeError(
            f'opam binary not found at {paths.opam_bin}; re-run without '
            f'--skip-opam.')

    env = _opam_env(paths)
    _init_opam(paths, env)

    if not args.skip_comby:
        _clone_comby(paths)
        _build_comby(paths, env, args.jobs)

    logging.info('Done.')
    logging.info('opam:  %s', paths.opam_bin)
    if paths.comby_bin.is_file():
        logging.info('comby: %s', paths.comby_bin)
    return 0


if __name__ == '__main__':
    sys.exit(main())
