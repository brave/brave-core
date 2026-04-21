#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Build BoringTun for Brave from the pinned Cargo git dependency.

Standalone usage:
    python3 brave/script/build_boringtun.py --target-os <os> --target-cpu <cpu>

Runs cargo with CARGO_HOME and CARGO_TARGET_DIR pinned to brave-local
paths, so the build never reads or writes Chromium's Rust caches.
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

TRIPLE = {
    ('win',   'x86'):   'x86_64-pc-windows-msvc',
    ('win',   'x64'):   'x86_64-pc-windows-msvc',
    ('win',   'arm64'): 'aarch64-pc-windows-msvc',
    ('linux', 'x64'):   'x86_64-unknown-linux-gnu',
    ('linux', 'arm64'): 'aarch64-unknown-linux-gnu',
    ('mac',   'x64'):   'x86_64-apple-darwin',
    ('mac',   'arm64'): 'aarch64-apple-darwin',
}

LIB_NAME = {
    'win':   'boringtun.dll',
    'linux': 'libboringtun.so',
    'mac':   'libboringtun.dylib',
}

_LOG_BUFFER = []
 
 
def log(msg):
    _LOG_BUFFER.append(f'[boringtun] {msg}')
 
 
def flush_log():
    if _LOG_BUFFER:
        print('\n'.join(_LOG_BUFFER), file=sys.stderr, flush=True)


def run_cargo(cargo, manifest, triple, env):
    cmd = [cargo, 'build', '--release', '--target', triple,
           '-p', 'boringtun', '--manifest-path', str(manifest),
           '--locked', '--offline']
    log('$ ' + ' '.join(cmd))
    # Capture cargo output so success is silent. On failure, main() flushes
    # the log buffer AND the captured cargo output together. Run from the
    # manifest's dir so cargo's config discovery finds .cargo/config.toml.
    result = subprocess.run(cmd, env=env, cwd=manifest.parent,
                            capture_output=True, text=True)
    if result.stdout:
        log(result.stdout.rstrip())
    if result.stderr:
        log(result.stderr.rstrip())
    if result.returncode != 0:
        raise subprocess.CalledProcessError(
            result.returncode, cmd, result.stdout, result.stderr)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--target-os', required=True,
                    choices=['win', 'linux', 'mac'])
    ap.add_argument('--target-cpu', required=True, choices=['x86', 'x64', 'arm64'])
    ap.add_argument('--cargo-target-dir')
    ap.add_argument('--output-lib',
                    help='Optional extra path to copy the built lib to.')
    ap.add_argument('--stamp', help='Touch this file on success (for GN).')
    args = ap.parse_args()

    # Paths derive from script location.
    script = Path(__file__).resolve()
    src_root = script.parents[2]
    boringtun = src_root / 'brave' / 'third_party' / 'boringtun'
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
    bundled = src_root / 'third_party' / 'rust-toolchain' / 'bin' / cargo_exe
    cargo = (os.environ.get('BRAVE_BORINGTUN_CARGO')
             or (str(bundled_cargo) if bundled_cargo.exists() else shutil.which('cargo')))
    if not cargo:
        sys.exit('cargo not found; set BRAVE_BORINGTUN_CARGO or install it')

    # Isolated env. Drop anything inherited from Chromium's build that could
    # change cargo's behavior; this build depends only on our own config.
    env = os.environ.copy()
    env['CARGO_HOME'] = str(cargo_home)
    env['CARGO_TARGET_DIR'] = str(target_dir)
    env['RUSTUP_AUTO_INSTALL'] = '0'
    for var in ('RUSTFLAGS', 'CARGO_BUILD_RUSTFLAGS', 'CARGO_BUILD_TARGET',
                'RUSTC_WRAPPER', 'RUSTC_WORKSPACE_WRAPPER', 'RUSTUP_HOME',
                'RUSTUP_TOOLCHAIN','RUSTUP_DIST_SERVER', 'RUSTUP_UPDATE_ROOT'):
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
    run_cargo(cargo, manifest, triple, env)
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
