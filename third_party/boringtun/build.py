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
import logging
import logging.handlers
import shutil
import subprocess
import sys
import time
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

# Host-platform tool names. The `.exe` suffix depends on the *host*
# (where these binaries run), not the target -- a Linux host
# cross-compiling to Windows still invokes `cargo`, not `cargo.exe`.
_EXE = '.exe' if sys.platform == 'win32' else ''
CLANG = f'clang{_EXE}'
CLANG_CL = f'clang-cl{_EXE}'
LLD_LINK = f'lld-link{_EXE}'
LLVM_AR = f'llvm-ar{_EXE}'
CARGO = f'cargo{_EXE}'
RUSTC = f'rustc{_EXE}'

log = logging.getLogger('boringtun')
_buffer: logging.handlers.MemoryHandler | None = None


def _setup_logging(quiet: bool):
    """Configure the 'boringtun' logger for this run.

    In non-quiet mode, INFO and above stream directly to stderr.

    In quiet mode, a MemoryHandler buffers records in memory; nothing
    reaches stderr until _flush_logging() is called explicitly. The
    buffer is sized for ~10k records (well above what a normal build
    emits). flushLevel=CRITICAL serves as an emergency override -- any
    record at that level auto-flushes -- but INFO and ERROR (the levels
    actually used) stay buffered until the failure path calls
    _flush_logging().
    """
    global _buffer
    log.setLevel(logging.INFO)
    log.handlers.clear()
    stream = logging.StreamHandler(sys.stderr)
    stream.setFormatter(logging.Formatter('[boringtun] %(message)s'))
    if quiet:
        _buffer = logging.handlers.MemoryHandler(capacity=10000,
                                                 flushLevel=logging.CRITICAL,
                                                 target=stream,
                                                 flushOnClose=False)
        log.addHandler(_buffer)
    else:
        log.addHandler(stream)


def _is_logging_buffered() -> bool:
    """Return True if logging is in quiet-buffer mode.

    Equivalent to "was _setup_logging called with quiet=True". Used
    to decide whether to capture subprocesses' stdout/stderr (so
    they can be appended to the log buffer) or let a process stream
    directly to the terminal.
    """
    return _buffer is not None


def _finalize_logging(flush: bool):
    """Drain the buffer to stderr if flush=True, then discard it.

    Called on every exit path. Failure paths pass flush=True so any
    buffered build output and subprocesses' stderr reach the user. The
    success path passes flush=False to silently drop the buffer;
    this also neutralizes logging.shutdown()'s automatic flush at
    interpreter exit (which would otherwise dump the buffer
    regardless of flushOnClose) by unsetting the handler's target.
    """
    if _is_logging_buffered():
        if flush:
            _buffer.flush()
        _buffer.target = None
        _buffer.buffer.clear()


def _has_prebuilt_std(toolchain_root: Path, triple: str) -> bool:
    """Check whether prebuilt rust-std is installed for the given target.
 
    A present rustlib/<triple>/lib/ with at least one libcore-*.rlib
    indicates the stdlib was installed via the toolchain's normal
    component-install mechanism. When this returns False, the caller
    should fall back to -Zbuild-std to compile std from rust-src.
    """
    target_lib_dir = toolchain_root / 'lib' / 'rustlib' / triple / 'lib'
    if not target_lib_dir.is_dir():
        return False
    return any(target_lib_dir.glob('libcore-*.rlib'))


def _build_merged_vendor(target_dir: Path, project_vendor: Path,
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


def _ensure_win_clang_shim(binpath: Path, shim_dir: Path) -> Path:
    """Provide a clang executable for build scripts that look it up by name.

    Brave's bundled LLVM for Windows ships only `clang-cl`. ring
    (and any cc-rs consumer that wants GNU-mode clang for .S files
    or specific .c compilations) invokes literal `clang` via PATH.
    Without `clang` next to clang-cl, that lookup fails.

    clang-cl and clang are the same LLVM driver binary; the mode is
    selected from argv[0] basename. A copy of clang-cl named clang behaves
    as GNU-mode clang.

    We need to physically provide a clang file rather than
    a shim script because CreateProcess on Windows doesn't respect
    PATHEXT when searching for executables, so a clang.cmd won't be
    found when looking for clang.

    Returns the shim directory.
    """
    src = binpath / CLANG_CL
    if not src.is_file():
        raise FileNotFoundError(f'clang-cl not found at {src}')
    shim_dir.mkdir(parents=True, exist_ok=True)
    dst = shim_dir / CLANG
    if dst.is_symlink() or dst.exists():
        dst.unlink()
    try:
        os.link(src, dst)
    except OSError:
        shutil.copy2(src, dst)
    return shim_dir


def _write_shell_wrapper(directory: Path, name: str, command: str,
                         flags: list[str], *, flags_after_args: bool) -> Path:
    """Write a host-appropriate shell wrapper invoking `command` with
    `flags` plus the user's args.
 
    Host (sys.platform) selects .cmd vs .sh; flags_after_args selects
    the position of the user's args relative to our injected flags:
 
    flags_after_args=False -> `"command" <flags> <args>`
    flags_after_args=True  -> `"command" <args> <flags>`
 
    `command` is wrapped in double quotes; any path-quoting needed
    inside individual `flags` entries is the caller's responsibility
    (e.g. f'"{sysroot}"' for paths that may contain spaces). On POSIX
    hosts the resulting .sh is chmod'd 0755.
 
    Returns the path to the written wrapper.
    """
    directory.mkdir(parents=True, exist_ok=True)
    flag_str = ' '.join(flags)
    if sys.platform == 'win32':
        path = directory / f'{name}.cmd'
        argv = f'%* {flag_str}' if flags_after_args else f'{flag_str} %*'
        path.write_text(f'@echo off\r\n"{command}" {argv}\r\n',
                        encoding='ascii',
                        newline='')
    else:
        path = directory / f'{name}.sh'
        argv = f'"$@" {flag_str}' if flags_after_args else f'{flag_str} "$@"'
        path.write_text(f'#!/bin/sh\nexec "{command}" {argv}\n',
                        encoding='ascii',
                        newline='\n')
        path.chmod(0o755)
    return path


def _win_vc_tools_dir(winsysroot: Path) -> Path:
    """Return the versioned MSVC toolset dir under a Visual Studio root.

    clang-cl and lld-link derive the SDK location from /winsysroot and
    ignore an explicit /winsdkdir when both are given, so a local VS
    install (where the SDK lives outside the VS dir) must use the split
    /vctoolsdir + /winsdkdir form instead. That requires resolving
    <root>/VC/Tools/MSVC/<version> ourselves; picks the highest version
    when several toolsets are installed.
    """
    msvc = winsysroot / 'VC' / 'Tools' / 'MSVC'
    if not msvc.is_dir():
        sys.exit(f'MSVC toolset dir not found at {msvc}')
    versions = sorted(
        (d for d in msvc.iterdir()
         if d.is_dir() and all(p.isdigit() for p in d.name.split('.'))),
        key=lambda d: [int(p) for p in d.name.split('.')])
    if not versions:
        sys.exit(f'no MSVC toolset installed under {msvc}')
    return versions[-1]


def _win_sysroot_flags(winsysroot: Path, winsdkdir: Path | None) -> list[str]:
    """Return ['/flag', '"value"', ...] locating the MSVC libs/headers.

    When the Windows SDK lives under the sysroot as "Windows Kits/10"
    (the hermetic toolchain package layout; this probe mirrors how
    clang-cl and lld-link expand /winsysroot), a single /winsysroot is
    emitted, keeping hermetic builds identical to before this option
    existed. Otherwise (a locally installed Visual Studio, where the
    SDK lives outside the VS dir) the split /vctoolsdir + /winsdkdir
    form is required: /winsysroot takes precedence over /winsdkdir in
    LLVM's MSVC path detection, so passing both would point SDK lookup
    at a "Windows Kits/10" dir that does not exist under the VS root.
    """
    if winsdkdir and not (winsysroot / 'Windows Kits' / '10').is_dir():
        return [
            '/vctoolsdir', f'"{_win_vc_tools_dir(winsysroot)}"', '/winsdkdir',
            f'"{winsdkdir}"'
        ]
    return ['/winsysroot', f'"{winsysroot}"']


def _win_sysroot_flags_gnu(winsysroot: Path,
                           winsdkdir: Path | None) -> list[str]:
    """GNU-driver (bare clang) counterpart of _win_sysroot_flags().

    Bare clang does not accept the CL-style /winsysroot family; the
    equivalent spellings are -Xmicrosoft-windows-sys-root and, for the
    split form, -Xmicrosoft-visualc-tools-root plus
    -Xmicrosoft-windows-sdk-root. Layout selection matches
    _win_sysroot_flags(): a single sys-root when the SDK lives under
    the sysroot (the hermetic package), the split form otherwise (a
    local VS install). Paths use forward slashes, consistently with
    the other GNU-driver flags in this file.
    """

    def fwd(path: Path) -> str:
        return str(path).replace('\\', '/')

    if winsdkdir and not (winsysroot / 'Windows Kits' / '10').is_dir():
        return [
            '-Xmicrosoft-visualc-tools-root',
            f'"{fwd(_win_vc_tools_dir(winsysroot))}"',
            '-Xmicrosoft-windows-sdk-root',
            f'"{fwd(winsdkdir)}"',
        ]
    return ['-Xmicrosoft-windows-sys-root', f'"{fwd(winsysroot)}"']


def _make_compiler_wrappers(wrappers_dir: Path, binpath: Path,
                            winsysroot: Path,
                            winsdkdir: Path | None) -> tuple[Path, Path]:
    """Create clang and clang-cl shell scripts that inject the Windows
    sysroot in the dialect each compiler personality understands.

    The bare-clang shim and clang-cl share CFLAGS via cc-rs, but they
    accept different sysroot flags: clang-cl takes the /winsysroot
    family (see _win_sysroot_flags), bare clang (GNU driver) takes the
    -Xmicrosoft-* spellings (see _win_sysroot_flags_gnu).
    Putting the sysroot in CFLAGS forces one dialect on both, which
    breaks ring's bare-clang override on aarch64-pc-windows-msvc.
    Wrappers move the sysroot out of CFLAGS so each personality gets
    its own form.

    The clang wrapper invokes the local `clang` (placed by
    _ensure_win_clang_shim) via a script-dir-relative path -- never
    through PATH, which would risk recursing into the wrapper itself.

    Returns (clang_wrapper, clang_cl_wrapper).
    """
    # clang-cl wrapper: absolute path to the bundled `clang-cl`,
    # plus the winsysroot (or vctoolsdir + winsdkdir) flags.
    clang_cl = _write_shell_wrapper(
        wrappers_dir,
        'clang-cl',
        str(binpath / CLANG_CL),
        _win_sysroot_flags(winsysroot, winsdkdir),
        flags_after_args=False,
    )

    # clang wrapper: invoke the sibling `clang` (the shim) by
    # script-dir-relative path. %~dp0 / $(dirname "$0") avoids the
    # cwd-relative ".\clang.exe" / "./clang" form, which would only
    # resolve correctly when the build's cwd happens to be the wrappers
    # directory.
    if sys.platform == 'win32':
        local_clang = f'%~dp0{CLANG}'
    else:
        local_clang = f'$(dirname "$0")/{CLANG}'
    clang = _write_shell_wrapper(
        wrappers_dir,
        'clang',
        local_clang,
        ['--driver-mode=gcc'] + _win_sysroot_flags_gnu(winsysroot, winsdkdir),
        flags_after_args=False,
    )

    log.info(f'wrapped clang:    {clang}')
    log.info(f'wrapped clang-cl: {clang_cl}')
    return clang, clang_cl


def _target_rustflags(target_os: str, libname: str) -> list[str]:
    """Non-default rustflags by target OS, applied to all build profiles.
 
    Windows: +crt-static statically links the MSVC runtime so the
        shipped DLL has no vcruntime/msvcp redistributable dependency.
    Linux: -Wl,--no-undefined makes the linker fail on unresolved
        symbols rather than deferring to runtime lookup.
    macOS: -Wl,-undefined,error is the macOS equivalent of the above.
        -install_name @rpath replaces the absolute build-machine path
        that ld64 would otherwise embed in the dylib.
    """
    if target_os == 'win':
        return ['-C', 'target-feature=+crt-static']
    if target_os == 'linux':
        return ['-C', 'link-arg=-Wl,--no-undefined']
    if target_os == 'mac':
        return [
            '-C',
            'link-arg=-Wl,-undefined,error',
            '-C',
            f'link-arg=-Wl,-install_name,@rpath/{libname}',
        ]
    raise ValueError(f'unknown target_os: {target_os!r}')


def _cross_compile_flags(triple, target_os, sysroot, mac_min_version) -> list:
    """Return the compiler flags shared between CFLAGS and the linker wrapper.
 
    Includes --target, --sysroot (POSIX targets only; Windows targets
    use clang-cl's /winsysroot, which the caller adds separately), and
    -mmacosx-version-min when applicable. Order is stable but argument
    order is not significant to clang.
    """
    flags = [f'--target={triple}']
    if sysroot and target_os != 'win':
        flags.append(f'--sysroot={sysroot}')
    if target_os == 'mac' and mac_min_version:
        flags.append(f'-mmacosx-version-min={mac_min_version}')
    return flags


def _make_linker_wrapper(wrappers_dir: Path,
                         linker_path: Path,
                         sysroot: Path,
                         target_os,
                         triple=None,
                         mac_min_version=None,
                         winsdkdir: Path | None = None) -> Path:
    """Create a wrapper script that invokes the given linker with appropriate
    flags. On Windows, this is used to pass /winsysroot to the linker in
    cross-builds. On macOS, this is used to select clang with -fuse-ld=lld
    and pass --sysroot and -mmacosx-version-min in cross-builds.
    On other platforms, this is used to pass sysroot and --target to the
    linker in cross-builds.
    """
    if target_os == 'win':
        # lld-link is positional about its inputs, so put our injected
        # flags after the user's args. lld-link takes these options in
        # /flag:value form, unlike clang-cl's separate-argument form.
        flags = []
        if sysroot:
            pairs = _win_sysroot_flags(sysroot, winsdkdir)
            flags = [
                f'{flag}:{value}'
                for flag, value in zip(pairs[::2], pairs[1::2])
            ]
        flags_after_args = True
    else:
        flags = ['-fuse-ld=lld'] + _cross_compile_flags(
            triple, target_os, sysroot, mac_min_version)
        flags_after_args = False

    return _write_shell_wrapper(
        wrappers_dir,
        linker_path.stem,
        str(linker_path),
        flags,
        flags_after_args=flags_after_args,
    )


def _setup_cc_env(env,
                  triple,
                  target_os,
                  wrappers_dir: Path,
                  binpath: Path,
                  sysroot: Path,
                  mac_min_version,
                  win_sdk_dir=None):
    """Configure env vars for cc-rs and cargo's target linker.
 
    Ring and other Rust crates that build C via build.rs use cc-rs,
    which looks up CC_<triple>, CFLAGS_<triple>, AR_<triple>. Cargo
    itself looks up CARGO_TARGET_<TRIPLE>_LINKER. All of these are
    expected to be set consistently for a clean cross-compile.
 
    CRATE_CC_NO_DEFAULTS=1 stops cc-rs from adding host-probed flags
    on top of what we pass, which is essential for cross-builds where
    host defaults would be wrong.
    """
    env['CRATE_CC_NO_DEFAULTS'] = '1'

    triple_env_suffix_uc = triple.upper().replace('-', '_')
    triple_env_suffix_lc = triple_env_suffix_uc.lower()

    if binpath:
        cc_name = CLANG_CL if target_os == 'win' else CLANG
        link_name = LLD_LINK if target_os == 'win' else CLANG
        ar_name = LLD_LINK if target_os == 'win' else LLVM_AR
        ar_extra = ' /lib' if target_os == 'win' else ''

        if sysroot and target_os == 'win':
            # Discards the bare-clang wrapper return value: that wrapper
            # exists only for ring's PATH-based clang lookup; cc_path
            # below is the clang-cl wrapper used as CC_<triple>.
            _, cc_path = _make_compiler_wrappers(wrappers_dir, binpath,
                                                 sysroot, win_sdk_dir)
        else:
            cc_path = binpath / cc_name
        ar_path = binpath / ar_name
        linker_path = _make_linker_wrapper(wrappers_dir, binpath / link_name,
                                           sysroot, target_os, triple,
                                           mac_min_version, win_sdk_dir)

        for tool, label in [(cc_path, 'CC'), (ar_path, 'AR'),
                            (linker_path, 'Linker')]:
            if not tool.is_file():
                raise FileNotFoundError(f'{label} not found at {tool}')

        env['CC_' + triple_env_suffix_lc] = str(cc_path)
        env['AR_' + triple_env_suffix_lc] = str(ar_path) + ar_extra
        env[f'CARGO_TARGET_{triple_env_suffix_uc}_LINKER'] = str(linker_path)

    env[f'CFLAGS_{triple_env_suffix_lc}'] = ' '.join(
        _cross_compile_flags(triple, target_os, sysroot, mac_min_version))


def _run_cargo(cargo,
               manifest,
               triple,
               target_os,
               libname,
               env,
               *,
               is_debug,
               locked,
               build_std,
               merged_vendor=None):
    """Invoke `cargo build` with our pinned configuration.
 
    Always passes --offline so cargo cannot reach the network and must
    resolve from the vendored sources. --locked is the default; pass
    locked=False only on first-time setup to allow Cargo.lock generation.
 
    When build_std is True, also passes -Zbuild-std=std,panic_abort and
    --config source.vendored-sources.directory=<merged_vendor> so std
    and project crates resolve from a single vendor view.
 
    On failure, raises subprocess.CalledProcessError. In quiet mode,
    cargo's stdout/stderr is appended to the log buffer so main()'s
    failure path can flush it alongside our progress logs.
    """
    cmd = [
        cargo, 'build', '--target', triple, '-p', 'boringtun', '--offline',
        '--manifest-path',
        str(manifest)
    ]
    if locked:
        cmd.append('--locked')
    if not is_debug:
        cmd.append('--release')
        # Release profile overrides to match BoringTun's upstream configuration.
        cmd.extend([
            '--config',
            'profile.release.lto="fat"',
            '--config',
            'profile.release.codegen-units=1',
        ])

    rustflags = _target_rustflags(target_os, libname)
    if rustflags:
        rustflags_toml = '[' + ', '.join(f'"{f}"' for f in rustflags) + ']'
        cmd.extend([
            '--config',
            f'target.{triple}.rustflags={rustflags_toml}',
        ])

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

    log.info('$ ' + ' '.join(cmd))
    # Capture cargo output so success is silent, when logging is buffered.
    # On failure, main() flushes the log buffer AND the captured cargo
    # output together. Run from the manifest's dir so cargo's config
    # discovery finds .cargo/config.toml.
    quiet = _is_logging_buffered()
    result = subprocess.run(cmd,
                            env=env,
                            cwd=manifest.parent,
                            capture_output=quiet,
                            text=True,
                            check=False)
    if quiet:
        if result.stdout:
            log.info(result.stdout.rstrip())
        if result.stderr:
            log.error(result.stderr.rstrip())
    if result.returncode != 0:
        raise subprocess.CalledProcessError(result.returncode, cmd,
                                            result.stdout, result.stderr)


def _locate_cargo_and_rustc(rust_sysroot: Path | None, src_root: Path):
    """Resolve cargo and rustc within the bundled Rust toolchain.
 
    Resolution order:
    1. --rust-sysroot (passed by BUILD.gn from Chromium's rust_sysroot).
    2. Auto-derive third_party/rust-toolchain from this script's location
       (for standalone CLI runs inside a Brave checkout).

    Never falls back to PATH: builds must use the bundled toolchain to
    guarantee a consistent Rust version across all environments. A
    locally-installed rustc/cargo (e.g. via rustup) is not acceptable.

    Returns (cargo: str, rustc: Path, toolchain_bin: Path). Exits if
    either binary is missing under the resolved sysroot.
    """
    if rust_sysroot is None:
        rust_sysroot = src_root / 'third_party' / 'rust-toolchain'
    toolchain_bin = rust_sysroot / 'bin'
    cargo = toolchain_bin / CARGO
    rustc = toolchain_bin / RUSTC
    if not cargo.exists():
        sys.exit(f'cargo not found at {cargo}: pass --rust-sysroot or '
                 f'ensure the bundled toolchain is present')
    if not rustc.exists():
        sys.exit(f'rustc not found at {rustc}: bundled toolchain is '
                 f'incomplete')
    return str(cargo), rustc, toolchain_bin


# Static scrub list -- known names that affect cargo/rustc behavior.
_STATIC_SCRUB = frozenset({
    # Flag-injection vectors. CARGO_ENCODED_* take precedence over the
    # non-encoded variants and over .cargo/config.toml, so both forms
    # must be scrubbed.
    'RUSTFLAGS',
    'CARGO_ENCODED_RUSTFLAGS',
    'RUSTDOCFLAGS',
    'CARGO_ENCODED_RUSTDOCFLAGS',
    'CARGO_BUILD_RUSTFLAGS',
    # Build target / wrapper override.
    'CARGO_BUILD_TARGET',
    'RUSTC_WRAPPER',
    'RUSTC_WORKSPACE_WRAPPER',
    # Blind rustup's shim if it gets invoked via PATH by a build script.
    # Without its env vars, it falls through to exec'ing whatever `rustc`
    # it can find, rather than trying to install toolchains or components
    # mid-build. Makes the build robust against any local rustup state.
    'RUSTUP_HOME',
    'RUSTUP_TOOLCHAIN',
    'RUSTUP_DIST_SERVER',
    'RUSTUP_UPDATE_ROOT',
})


def _make_isolated_env(toolchain_bin: Path, rustc: Path, cargo_home: Path,
                       target_dir: Path) -> dict:
    """Build a process env dict with all flag-injection vectors scrubbed.
 
        Drops anything inherited from Chromium's build that could change
    cargo's behavior; this build depends only on our own config.
    Untrusted upstream environments are also a concern -- these env
    vars are how an attacker would inject rustc flags to disable safety
    checks, alter codegen, or smuggle in a malicious linker/wrapper.
 
    Sets RUSTC to the bundled rustc and prepends the bundled bin to
    PATH so any sibling tools (rustdoc, rustfmt) resolve consistently
    rather than via rustup's shim.
    """
    env = os.environ.copy()
    env['CARGO_HOME'] = str(cargo_home)
    env['CARGO_TARGET_DIR'] = str(target_dir)
    env['RUSTUP_AUTO_INSTALL'] = '0'

    for var in _STATIC_SCRUB:
        env.pop(var, None)

    # Per-target overrides. Cargo reads CARGO_TARGET_<TRIPLE>_RUSTFLAGS
    # and CARGO_TARGET_<TRIPLE>_LINKER, where <TRIPLE> is the uppercased,
    # underscored target triple (e.g. CARGO_TARGET_X86_64_APPLE_DARWIN_
    # RUSTFLAGS). The triple varies, so scrub by pattern.
    for v in list(env):
        if v.startswith('CARGO_TARGET_') and v.endswith(
            ('_RUSTFLAGS', '_LINKER')):
            env.pop(v, None)

    # Pin cargo to the bundled rustc (not whatever rustup's shim on PATH
    # would resolve to). Prepend the bundled bin too so any sibling
    # tools (rustdoc, rustfmt) resolve consistently.
    env['RUSTC'] = str(rustc)
    env['PATH'] = str(toolchain_bin) + os.pathsep + env.get('PATH', '')

    return env


def _write_depfile(depfile_path, stamp_path, vendor_dir):
    """Write a Make-style depfile listing all vendored crate inputs.

    GN consumes this depfile to know when to re-run the action: any
    change to a listed dependency triggers a rebuild.

    The listed dependencies are each crate's `.cargo-checksum.json`
    (produced by `cargo vendor`), which itself hashes every file in
    that crate. Depending on the checksum files is a cheap proxy for
    depending on the full vendor tree -- one path per crate instead
    of thousands.

    Paths are emitted relative to cwd (GN's root_build_dir when
    invoked from the action) and escaped per Make rules: backslash
    and space each prefixed with a backslash. Crates are sorted so a
    clean rebuild produces a byte-identical depfile.
    """
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
    depfile_path.write_text('\n'.join(lines) + '\n',
                            encoding='utf-8',
                            newline='\n')
    log.info(f'depfile -> {depfile_path} ({len(checksums)} vendored crates)')


def main():
    start = time.monotonic()
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--target-os',
                    required=True,
                    choices=['win', 'linux', 'mac'])
    ap.add_argument('--target-cpu',
                    required=True,
                    choices=['x86', 'x64', 'arm64'])
    ap.add_argument('--debug',
                    action='store_true',
                    help='Build debug mode (default is release).')
    ap.add_argument('--cargo-target-dir')
    ap.add_argument('--rust-sysroot',
                    help='Path to the bundled Rust toolchain root '
                    '(typically src/third_party/rust-toolchain). When '
                    'absent, the script auto-derives it from its own '
                    'location.')
    ap.add_argument('--cc-binpath',
                    help='Path to C compiler bin directory for the target '
                    'platform, if hermetic toolchain is used.')
    ap.add_argument('--cc-sysroot',
                    help='Path to the sysroot for the target platform, if '
                    'needed. On Windows, it should be the MSVC winsysroot.')
    ap.add_argument('--win-sdk-dir',
                    help='Path to the Windows SDK root (e.g. "C:\\Program '
                    'Files (x86)\\Windows Kits\\10"). Only used when the '
                    'SDK does not live under --cc-sysroot as '
                    '"Windows Kits\\10", i.e. a locally installed Visual '
                    'Studio rather than the hermetic toolchain package; '
                    'hermetic layouts keep using /winsysroot.')
    ap.add_argument('--mac-min-version',
                    help='Minimum macOS version to target, e.g. "12.0". Only '
                    'needed for cross-building on macOS.')
    ap.add_argument('--output-lib',
                    help='Optional extra path to copy the built lib to.')
    ap.add_argument('--output-headers',
                    help='Optional extra path to copy the header files to.')
    ap.add_argument('--depfile', help='Write this depfile for GN.')
    ap.add_argument('--stamp', help='Touch this file on success (for GN).')
    ap.add_argument('--no-locked',
                    dest='locked',
                    action='store_false',
                    default=True)
    ap.add_argument('--quiet-until-error',
                    action='store_true',
                    help='Buffer all output (script progress logs AND '
                    'cargo\'s stdout/stderr) and only flush on '
                    'failure. Default is live streaming. GN actions '
                    'set this so successful builds stay clean in '
                    'ninja output.')

    args = ap.parse_args()
    _setup_logging(args.quiet_until_error)

    # Paths derive from script location.
    script = Path(__file__).resolve()
    boringtun = script.parent
    src_root = script.parents[3]
    manifest = boringtun / 'Cargo.toml'
    cargo_home = boringtun / '.cargo-home'
    target_dir = (Path(args.cargo_target_dir).resolve()
                  if args.cargo_target_dir else boringtun / 'target')
    wrappers_dir = target_dir / '.tool-wrappers'

    rust_sysroot = (Path(args.rust_sysroot).resolve()
                    if args.rust_sysroot else None)
    cargo, rustc, toolchain_bin = _locate_cargo_and_rustc(
        rust_sysroot, src_root)
    env = _make_isolated_env(toolchain_bin, rustc, cargo_home, target_dir)

    bin_path = Path(args.cc_binpath).resolve() if args.cc_binpath else None
    sysroot_path = Path(args.cc_sysroot).resolve() if args.cc_sysroot else None
    win_sdk_dir = Path(
        args.win_sdk_dir).resolve() if args.win_sdk_dir else None

    if bin_path:
        extra = str(bin_path)
        if args.target_os == 'win':
            shim_dir = _ensure_win_clang_shim(bin_path, wrappers_dir)
            extra = str(shim_dir) + os.pathsep + extra
        env['PATH'] = extra + os.pathsep + env['PATH']

    log.info(f'cargo:            {cargo}')
    log.info(f'rustc:            {env["RUSTC"]}')
    log.info(f'CARGO_HOME:       {cargo_home}')
    log.info(f'CARGO_TARGET_DIR: {target_dir}')
    log.info(f'CC path:          {bin_path}')
    log.info(f'sysroot:          {sysroot_path}')
    log.info(f'win sdk:          {win_sdk_dir}')

    lib_filename = LIB_NAME[args.target_os]
    triple = TRIPLE[(args.target_os, args.target_cpu)]

    # Set up any necessary C compiler env vars for build scripts.
    _setup_cc_env(env, triple, args.target_os, wrappers_dir, bin_path,
                  sysroot_path, args.mac_min_version, win_sdk_dir)

    # Decide whether to compile std from source. Brave's bundled rust
    # toolchain ships prebuilt rust-std only for the host triple per
    # platform. For cross-arch builds (mac x64 on arm64 hosts, win x86,
    # win arm64), prebuilt std isn't available and we fall back to
    # -Zbuild-std to compile std from the rust-src component.
    build_std = not _has_prebuilt_std(toolchain_bin.parent, triple)
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
        merged_vendor = _build_merged_vendor(target_dir, boringtun / 'vendor',
                                             rust_std_vendor)
        log.info(f'merged vendor:    {merged_vendor}')
        log.info(
            f'prebuilt rust-std not found for {triple}; using -Zbuild-std')
    else:
        log.info(f'using prebuilt rust-std for {triple}')

    _run_cargo(cargo,
               manifest,
               triple,
               args.target_os,
               lib_filename,
               env,
               is_debug=args.debug,
               locked=args.locked,
               build_std=build_std,
               merged_vendor=merged_vendor)

    if args.output_lib:
        profile = 'debug' if args.debug else 'release'
        cargo_lib = target_dir / triple / profile / lib_filename
        out = Path(args.output_lib)
        out.parent.mkdir(parents=True, exist_ok=True)
        built_lib = out / lib_filename
        shutil.copy2(cargo_lib, built_lib)
        log.info(f'staged -> {built_lib}')

    if args.output_headers:
        # Extract the FFI header. Vendored sources put it at a fixed path.
        header = boringtun / 'vendor' / 'boringtun' / 'src' / 'wireguard_ffi.h'
        if not header.is_file():
            sys.exit(f'wireguard_ffi.h not found at {header}')
        include_dir = Path(args.output_headers)
        include_dir.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(header, include_dir / 'wireguard_ffi.h')
        log.info(f'header -> {include_dir / "wireguard_ffi.h"}')

    if args.depfile and args.stamp:
        _write_depfile(Path(args.depfile), Path(args.stamp),
                       boringtun / 'vendor')

    if args.stamp:
        Path(args.stamp).parent.mkdir(parents=True, exist_ok=True)
        Path(args.stamp).touch()

    log.info(f'Build complete in {time.monotonic() - start:.1f}s')


if __name__ == '__main__':
    try:
        rc = main()
    except subprocess.CalledProcessError:
        # cargo output is already in the log buffer.
        _finalize_logging(flush=True)
        sys.exit(1)
    except BaseException:
        # SystemExit (from sys.exit() inside main) and anything else:
        # flush so the error message is visible, then re-raise.
        _finalize_logging(flush=True)
        raise
    _finalize_logging(flush=False)
    sys.exit(rc)
