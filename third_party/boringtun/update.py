# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Update the vendored sources under brave/third_party/boringtun/vendor/.
Do not run this script directly; use `npm run update_boringtun_resources`
instead.

Run the update after bumping the pinned BoringTun rev in Cargo.toml
(and any explicit `cargo update` you want for transitive deps). It:

  1. Backs up reviewer-owned files (per-crate README.chromium, combined
     and synthesized license files, the vendor .clang-format).
  2. Wipes and re-runs `cargo vendor --locked`, capturing the source
     replacement snippet cargo prints.
  3. Compares the captured snippet against .cargo/config.toml so a SHA
     bump that would desync the two is caught explicitly.
  4. Restores the backed-up files.
  5. Removes known-safe stray binaries.

The script does NOT regenerate per-crate README.chromium files. Run
create_licenses.py after this when you want that.
"""

import difflib
import os
import shutil
import subprocess
import sys
from pathlib import Path

import brave_chromium_utils

PRESERVE_PATTERNS = [
    'vendor/.clang-format',
    'vendor/*/README.chromium',
    'vendor/*/LICENSE-combined-brave-attribution',
    'vendor/*/LICENSE-MIT-brave-attribution',
]


def back_up_files(patterns):
    """Read text files matching `patterns` into memory.

    Text-only; if a binary pattern is ever added, switch to shutil.copy2
    to a temp dir.
    """
    backed_up_files = {}
    for pattern in patterns:
        for path in Path().glob(pattern):
            print(f'Backing up: {path}')
            backed_up_files[str(path)] = path.read_text(encoding='utf-8')
    return backed_up_files


def restore_files(backed_up_files):
    for path_str, content in backed_up_files.items():
        path = Path(path_str)
        print(f'Restoring: {path}')
        path.parent.mkdir(parents=True, exist_ok=True)
        # newline='\n' keeps line endings stable across Windows re-runs.
        with open(path, 'w', encoding='utf-8', newline='\n') as f:
            f.write(content)


def clean_up_files(patterns):
    for pattern in patterns:
        for path in Path().glob(pattern):
            print(f'Removing: {path}')
            path.unlink()


def compare_cargo_snippet(captured: str) -> None:
    """Compare cargo vendor's printed config snippet against config.toml.

    cargo vendor prints a [source.crates-io] / [source."git+..."] block
    that must be present in .cargo/config.toml for offline builds to
    resolve vendored sources. When the pinned git rev changes, the
    "git+..." key changes, and config.toml silently goes stale unless
    someone updates it.

    We don't attempt to auto-patch config.toml (its structure varies
    across edits). Instead we print a unified diff and exit non-zero if
    any [source...] section from the snippet isn't already present in
    config.toml verbatim. A human applies the diff.
    """
    config_path = Path('.cargo/config.toml')
    if not config_path.is_file():
        print(f'warning: {config_path} not found; cannot verify source '
              'replacement. Paste this snippet into it manually:\n')
        print(captured)
        sys.exit(1)

    current = config_path.read_text(encoding='utf-8')

    # Coarse but effective: every non-empty line of the snippet should
    # appear somewhere in the current config. We look for [source...]
    # section headers specifically since those are the load-bearing
    # parts.
    missing = []
    for line in captured.splitlines():
        stripped = line.strip()
        if stripped.startswith('[source') and stripped not in current:
            missing.append(stripped)

    if not missing:
        return

    print('\nERROR: .cargo/config.toml is out of sync with what cargo '
          'vendor just produced. The following [source...] stanzas are '
          'missing from config.toml:\n')
    for line in missing:
        print(f'  {line}')
    print('\nUnified diff hint (section headers only):\n')
    diff = difflib.unified_diff(
        current.splitlines(keepends=True),
        (current + '\n' + captured).splitlines(keepends=True),
        fromfile='.cargo/config.toml (current)',
        tofile='.cargo/config.toml (with snippet appended)',
        n=1)
    sys.stdout.writelines(diff)
    print('\nUpdate .cargo/config.toml to match what cargo vendor printed, '
          'then re-run this script.')
    sys.exit(1)


with brave_chromium_utils.sys_path('//tools/rust'):
    import update_rust
    CARGO = os.path.join(update_rust.RUST_TOOLCHAIN_OUT_DIR, 'bin',
                         'cargo' + ('.exe' if sys.platform == 'win32' else ''))


def main():
    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    backed_up_files = back_up_files(PRESERVE_PATTERNS)

    # Isolated env. Mirrors build.py's isolation so re-vendor
    # and re-build behave consistently. `cargo vendor` doesn't compile
    # anything, but rustup's shim on PATH can still try to manage
    # toolchains when invoked, so we blind it.
    env = os.environ.copy()
    env['CARGO_HOME'] = str(Path('.cargo-home').resolve())
    for v in ('RUSTFLAGS', 'CARGO_BUILD_RUSTFLAGS', 'CARGO_BUILD_TARGET',
              'RUSTC_WRAPPER', 'RUSTC_WORKSPACE_WRAPPER', 'RUSTUP_HOME',
              'RUSTUP_TOOLCHAIN', 'RUSTUP_DIST_SERVER', 'RUSTUP_UPDATE_ROOT'):
        env.pop(v, None)

    shutil.rmtree('vendor', ignore_errors=True)

    # --locked makes lockfile drift an error rather than a silent update.
    # If you're re-vendoring because you intentionally changed deps, run
    # `cargo update` (or edit Cargo.toml rev) beforehand -- the new
    # Cargo.lock needs to be consistent before this script runs.
    print(f'Running cargo vendor...')
    result = subprocess.run([CARGO, 'vendor', '--locked'],
                            env=env,
                            check=True,
                            capture_output=True,
                            text=True)

    restore_files(backed_up_files)
    shutil.rmtree('.cargo-home', ignore_errors=True)

    # After restoring files and cleaning up, confirm the config is in
    # sync with what vendor printed. This runs last so a config mismatch
    # doesn't leave the tree half-updated -- vendor/ is fully written
    # and reviewer files are restored before we check.
    compare_cargo_snippet(result.stdout)


if __name__ == '__main__':
    main()
