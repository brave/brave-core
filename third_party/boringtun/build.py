#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build BoringTun for Brave from the pinned Cargo git dependency.

Standalone usage:
    python3 build.py --target-os <os> --target-cpu <cpu>

Runs cargo with CARGO_HOME and CARGO_TARGET_DIR pinned to brave-local
paths, so the build never reads or writes Chromium's Rust caches.

First-time setup needs --no-locked so cargo can generate Cargo.lock.
Commit the resulting Cargo.lock; subsequent builds use --locked.
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

TRIPLE = {
    ('win', 'x86'): 'i686-pc-windows-msvc',
    ('win', 'x64'): 'x86_64-pc-windows-msvc',
    ('win', 'arm64'): 'aarch64-pc-windows-msvc',
    ('linux', 'x64'): 'x86_64-unknown-linux-gnu',
    ('linux', 'arm64'): 'aarch64-unknown-linux-gnu',
    ('mac', 'x64'): 'x86_64-apple-darwin',
    ('mac', 'arm64'): 'aarch64-apple-darwin',
}

LIB_NAME = {
    'win': 'boringtun.dll',
    'linux': 'libboringtun.so',
    'mac': 'libboringtun.dylib',
}

_LOG_BUFFER = []
_QUIET_UNTIL_ERROR = False


def log(msg):
    if _QUIET_UNTIL_ERROR:
        _LOG_BUFFER.append(f'[boringtun] {msg}')
    else:
        print(f'[boringtun] {msg}', file=sys.stderr, flush=True)


def flush_log():
    if _LOG_BUFFER:
        print('\n'.join(_LOG_BUFFER), file=sys.stderr, flush=True)
        _LOG_BUFFER.clear()


def has_prebuilt_std(toolchain_root: Path, triple: str) -> bool:
    """
    Check whether prebuilt rust-std is installed for the given target.
 
    A present rustlib/<triple>/lib/ with at least one libcore-*.rlib
    indicates the stdlib was installed via the toolchain's normal
    component-install mechanism. When this returns False, the caller
    should fall back to -Zbuild-std to compile std from rust-src.
    """
    target_lib_dir = toolchain_root / 'lib' / 'rustlib' / triple / 'lib'
    if not target_lib_dir.is_dir():
        return False
    return any(target_lib_dir.glob('libcore-*.rlib'))


def build_merged_vendor(target_dir: Path, project_vendor: Path,
                        rust_std_vendor: Path) -> Path:
    """Create a merged view containing both vendor directories.
 
    Required because `-Zbuild-std` needs std's dependencies AND
    boringtun's dependencies (in "vendor") resolvable from the same
    crates-io source replacement. Cargo doesn't support two disjoint
    directory sources for crates-io.
 
    The merged view uses symlinks: fast to create, zero byte duplication,
    and cargo's checksum verification reads through them transparently.
 
    Placed under target_dir so it's ephemeral -- wiped when the cargo
    target dir is cleaned, never committed to the source tree.
    """
    merged = target_dir / '.merged-vendor'
    # Rebuild every invocation -- cheap, and avoids stale-symlink bugs if
    # either source vendor changed between runs.
    if merged.exists():
        shutil.rmtree(merged)
    merged.mkdir(parents=True)

    seen: set[str] = set()
    # Project vendor first so rust-std entries take precedence on conflict.
    for source_vendor in (project_vendor, rust_std_vendor):
        if not source_vendor.is_dir():
            continue
        for crate_dir in source_vendor.iterdir():
            if not crate_dir.is_dir():
                continue
            name = crate_dir.name
            link = merged / name
            if name in seen:
                link.unlink()
            (merged / name).symlink_to(crate_dir.resolve(),
                                       target_is_directory=True)
            seen.add(name)
    return merged


def run_cargo(cargo,
              manifest,
              triple,
              env,
              *,
              locked,
              build_std,
              merged_vendor=None):
    cmd = [
        cargo, 'build', '--release', '--target', triple, '-p', 'boringtun',
        '--manifest-path',
        str(manifest), '--offline'
    ]
    if locked:
        cmd.append('--locked')
    if build_std:
        cmd.extend([
            '-Zbuild-std=std,panic_abort',
            '-Zbuild-std-features=panic-unwind',
        ])
        if merged_vendor is None:
            raise ValueError('build_std requires merged_vendor')
        # Fix slashes for Cargo's config parser on all platforms.
        merged_str = str(merged_vendor).replace('\\', '/')
        cmd.extend([
            '--config',
            f'source.vendored-sources.directory="{merged_str}"',
        ])

    log('$ ' + ' '.join(cmd))
    # Capture cargo output so success is silent. On failure, main() flushes
    # the log buffer AND the captured cargo output together. Run from the
    # manifest's dir so cargo's config discovery finds .cargo/config.toml.
    result = subprocess.run(cmd,
                            env=env,
                            cwd=manifest.parent,
                            capture_output=_QUIET_UNTIL_ERROR,
                            text=True,
                            check=False)
    if _QUIET_UNTIL_ERROR:
        if result.stdout:
            log(result.stdout.rstrip())
        if result.stderr:
            log(result.stderr.rstrip())
    if result.returncode != 0:
        raise subprocess.CalledProcessError(result.returncode, cmd,
                                            result.stdout, result.stderr)


def write_depfile(depfile_path, stamp_path, vendor_dir):
    checksums = sorted(vendor_dir.glob('*/.cargo-checksum.json'))
    cwd = Path.cwd()

    def rel(p: Path) -> str:
        return os.path.relpath(str(Path(p).resolve()), str(cwd))

    def escape(s: str) -> str:
        return s.replace('\\', '\\\\').replace(' ', '\\ ')

    lines = [f'{escape(rel(stamp_path))}: \\']
    for i, f in enumerate(checksums):
        suffix = ' \\' if i < len(checksums) - 1 else ''
        lines.append(f'  {escape(rel(f))}{suffix}')

    depfile_path.parent.mkdir(parents=True, exist_ok=True)
    depfile_path.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    log(f'depfile -> {depfile_path} ({len(checksums)} vendored crates)')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--target-os',
                    required=True,
                    choices=['win', 'linux', 'mac'])
    ap.add_argument('--target-cpu',
                    required=True,
                    choices=['x86', 'x64', 'arm64'])
    ap.add_argument('--cargo-target-dir')
    ap.add_argument('--output-lib',
                    help='Optional extra path to copy the built lib to.')
    ap.add_argument('--depfile', help='Write this depfile for GN.')
    ap.add_argument('--stamp', help='Touch this file on success (for GN).')
    ap.add_argument('--no-locked',
                    dest='locked',
                    action='store_false',
                    default=True)
    ap.add_argument('--quiet-until-error',
                    action='store_true',
                    help='Buffer all output (script progress logs AND '
                    "cargo's stdout/stderr) and only flush on "
                    'failure. Default is live streaming. GN actions '
                    'set this so successful builds stay clean in '
                    'ninja output.')

    args = ap.parse_args()

    global _QUIET_UNTIL_ERROR
    _QUIET_UNTIL_ERROR = args.quiet_until_error

    # Paths derive from script location.
    script = Path(__file__).resolve()
    boringtun = script.parent
    src_root = script.parents[3]
    manifest = boringtun / 'Cargo.toml'
    cargo_home = boringtun / '.cargo-home'
    target_dir = (Path(args.cargo_target_dir).resolve()
                  if args.cargo_target_dir else boringtun / 'target')

    # Locate cargo: env override -> Brave's bundled toolchain -> PATH.
    cargo_exe = 'cargo.exe' if sys.platform == 'win32' else 'cargo'
    rustc_exe = 'rustc.exe' if sys.platform == 'win32' else 'rustc'
    toolchain_bin = src_root / 'third_party' / 'rust-toolchain' / 'bin'
    bundled_cargo = toolchain_bin / cargo_exe
    bundled_rustc = toolchain_bin / rustc_exe
    cargo = (os.environ.get('BRAVE_BORINGTUN_CARGO')
             or (str(bundled_cargo)
                 if bundled_cargo.exists() else shutil.which('cargo')))
    if not cargo:
        sys.exit('cargo not found; set BRAVE_BORINGTUN_CARGO or install it')

    # Isolated env. Drop anything inherited from Chromium's build that could
    # change cargo's behavior; this build depends only on our own config.
    # Untrusted upstream environments are also a concern -- these env vars
    # are how an attacker would inject rustc flags to disable safety
    # checks, alter codegen, or smuggle in a malicious linker/wrapper.
    env = os.environ.copy()
    env['CARGO_HOME'] = str(cargo_home)
    env['CARGO_TARGET_DIR'] = str(target_dir)
    env['RUSTUP_AUTO_INSTALL'] = '0'

    # Static scrub list -- known names that affect cargo/rustc behavior.
    static_scrub = (
        # Flag-injection vectors. CARGO_ENCODED_* take precedence over
        # the non-encoded variants and over .cargo/config.toml, so both
        # forms must be scrubbed.
        'RUSTFLAGS',
        'CARGO_ENCODED_RUSTFLAGS',
        'RUSTDOCFLAGS',
        'CARGO_ENCODED_RUSTDOCFLAGS',
        'CARGO_BUILD_RUSTFLAGS',
        # Build target / wrapper override.
        'CARGO_BUILD_TARGET',
        'RUSTC_WRAPPER',
        'RUSTC_WORKSPACE_WRAPPER',
        # Blind rustup's shim if it gets invoked via PATH by a build
        # script. Without its env vars, it falls through to exec'ing
        # whatever `rustc` it can find, rather than trying to install
        # toolchains or components mid-build. Makes the build robust
        # against any local rustup state.
        'RUSTUP_HOME',
        'RUSTUP_TOOLCHAIN',
        'RUSTUP_DIST_SERVER',
        'RUSTUP_UPDATE_ROOT',
    )
    for var in static_scrub:
        env.pop(var, None)

    # Per-target overrides. Cargo reads CARGO_TARGET_<TRIPLE>_RUSTFLAGS
    # and CARGO_TARGET_<TRIPLE>_LINKER, where <TRIPLE> is the uppercased,
    # underscored target triple (e.g. CARGO_TARGET_X86_64_APPLE_DARWIN_
    # RUSTFLAGS). The triple varies, so scrub by pattern.
    for var in [
            v for v in env if v.startswith('CARGO_TARGET_') and (
                v.endswith('_RUSTFLAGS') or v.endswith('_LINKER'))
    ]:
        env.pop(var, None)

    # Force cargo to use Brave's bundled rustc rather than whatever is on
    # PATH (which is typically rustup's shim, and would try to download/manage
    # toolchains mid-build). Also prepend the bundled bin so any sibling
    # tools (rustdoc, rustfmt) resolve consistently.
    if bundled_rustc.exists():
        env['RUSTC'] = str(bundled_rustc)
        env['PATH'] = str(toolchain_bin) + os.pathsep + env.get('PATH', '')

    log(f'cargo:            {cargo}')
    log(f'rustc:            {env.get("RUSTC", "(default, via PATH)")}')
    log(f'CARGO_HOME:       {cargo_home}')
    log(f'CARGO_TARGET_DIR: {target_dir}')

    lib_filename = LIB_NAME[args.target_os]
    out_dir = boringtun / 'bin'
    out_dir.mkdir(parents=True, exist_ok=True)
    built_lib = out_dir / lib_filename
    triple = TRIPLE[(args.target_os, args.target_cpu)]

    # Decide whether to compile std from source. Brave's bundled rust
    # toolchain ships prebuilt rust-std only for the host triple per
    # platform. For cross-arch builds (mac x64 on arm64 hosts, win x86,
    # win arm64), prebuilt std isn't available and we fall back to
    # -Zbuild-std to compile std from the rust-src component.
    build_std = not has_prebuilt_std(toolchain_bin.parent, triple)
    merged_vendor = None
    if build_std:
        # -Z flags require nightly unless RUSTC_BOOTSTRAP is set.
        # Chromium's own Rust builds use this pattern; we follow suit.
        env['RUSTC_BOOTSTRAP'] = '1'
        rust_std_vendor = (toolchain_bin.parent / 'lib' / 'rustlib' / 'src' /
                           'rust' / 'library' / 'vendor')
        if not rust_std_vendor.is_dir():
            sys.exit(f'rust-src vendor not found at {rust_std_vendor}; '
                     'cannot build std from source.')
        merged_vendor = build_merged_vendor(target_dir, boringtun / 'vendor',
                                            rust_std_vendor)
        log(f'prebuilt rust-std not found for {triple}; using -Zbuild-std')
        log(f'merged vendor:    {merged_vendor}')
    else:
        log(f'using prebuilt rust-std for {triple}')

    run_cargo(cargo,
              manifest,
              triple,
              env,
              locked=args.locked,
              build_std=build_std,
              merged_vendor=merged_vendor)
    cargo_lib = target_dir / triple / 'release' / lib_filename
    shutil.copy2(cargo_lib, built_lib)
    log(f'staged -> {built_lib}')
    if args.target_os == 'win':
        for extra in ('boringtun.dll.lib', 'boringtun.pdb'):
            src = cargo_lib.parent / extra
            if src.exists():
                shutil.copy2(src, out_dir / extra)

    # Extract the FFI header. Vendored sources put it at a fixed path.
    header = boringtun / 'vendor' / 'boringtun' / 'src' / 'wireguard_ffi.h'
    if not header.is_file():
        sys.exit(f'wireguard_ffi.h not found at {header}')
    include_dir = boringtun / 'include'
    include_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(header, include_dir / 'wireguard_ffi.h')
    log(f'header -> {include_dir / "wireguard_ffi.h"}')

    if args.output_lib:
        out = Path(args.output_lib)
        out.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(built_lib, out)

    if args.depfile and args.stamp:
        write_depfile(Path(args.depfile), Path(args.stamp),
                      boringtun / 'vendor')

    if args.stamp:
        Path(args.stamp).parent.mkdir(parents=True, exist_ok=True)
        Path(args.stamp).touch()


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError:
        # cargo output is already in the log buffer from run_cargo.
        flush_log()
        sys.exit(1)
    except SystemExit:
        # sys.exit() from main() — flush so the error message is visible.
        flush_log()
        raise
    except BaseException:
        flush_log()
        raise
