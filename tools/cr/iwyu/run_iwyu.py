#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Run `include-what-you-use` across the Brave paths enabled in
`brave/build/include_what_you_use_paths.cfg`.

What this script does:

  1. Generates a compile database for the build dir given via `--out`, using
     the helpers behind Chromium's `tools/clang/scripts/generate_compdb.py`
     (`tools/clang/pylib/clang/compile_db.py`).  Building must already be
     done in that directory.
  2. Reads `brave/build/include_what_you_use_paths.cfg` and filters the
     compile DB down to entries whose source files fall under enabled paths.
  3. Writes the filtered DB to `<out>/iwyu_compile_commands.json` and invokes
     `iwyu_tool.py` against it with `IWYU_BINARY` pointed at the IWYU binary
     produced by `build_iwyu.py`.

Prerequisites:

  * `brave/tools/cr/iwyu/build_iwyu.py` has been run, producing the IWYU
    binary at `<src>/out/iwyu/tools/clang/third_party/llvm/build/bin/`.
  * The Brave build directory passed via `--out` has been built (so that
    `build.ninja` and any generated headers exist).
"""

from __future__ import annotations

import argparse
import json
import logging
import os
import re
import sys
from pathlib import Path

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
import repository  # noqa: E402  -- after _boot

# Make Chromium helpers importable.  Same trick
# `tools/clang/scripts/generate_compdb.py` itself uses.  Derived from
# __file__ rather than `repository.chromium.root` so the sys.path entries are
# stable regardless of the cwd `repository` was initialised against.
_SRC_ROOT: Path = Path(__file__).resolve().parents[4]
sys.path.append(str(_SRC_ROOT / 'tools' / 'clang' / 'pylib'))
sys.path.append(str(_SRC_ROOT / 'tools' / 'json_comment_eater'))

# pylint: disable=wrong-import-position
import json_comment_eater  # type: ignore  # noqa: E402
from clang import compile_db  # type: ignore  # noqa: E402
from terminal import terminal  # type: ignore  # noqa: E402
# pylint: enable=wrong-import-position

# IWYU artefacts produced by `build_iwyu.py`.  Keeping the paths in sync with
# that script is intentional -- run_iwyu.py is a no-op without it.
_IWYU_OUT_DIR: Path = (repository.chromium.root / 'out' / 'iwyu' / 'tools' /
                       'clang' / 'third_party')
IWYU_BINARY: Path = (_IWYU_OUT_DIR / 'llvm' / 'build' / 'bin' /
                     'include-what-you-use')
IWYU_TOOL: Path = _IWYU_OUT_DIR / 'iwyu' / 'iwyu_tool.py'
# Applies the textual suggestions produced by iwyu_tool to source files in
# place.  Lives alongside iwyu_tool.py in the same IWYU clone.
FIX_INCLUDES: Path = _IWYU_OUT_DIR / 'iwyu' / 'fix_includes.py'

# Brave-managed path filter file.  See module docstring.
PATHS_FILE: Path = (repository.brave.root / 'build' /
                    'include_what_you_use_paths.cfg')

# IWYU mapping file: remaps libc++ private detail headers (e.g.
# `__algorithm/ranges_sort.h`) to their public facades (`<algorithm>`).
# Passed to IWYU via `-Xiwyu --mapping_file=<absolute path>` so the lookup
# is independent of each compile DB entry's cwd. A comment-stripped copy
# is written under `--out` at run time and fed to iwyu_tool (see main).
MAPPINGS_FILE: Path = (repository.brave.root / 'build' /
                       'include_what_you_use_mappings.json5')

# Headers we never want to see in Brave's source after IWYU.
BLACKHOLE_INCLUDES: frozenset[str] = frozenset([
    '<new>',
])

# Matches `#include <hdr>` or `#include "hdr"`, capturing the delimited
# token (with its brackets/quotes intact) for comparison against
# BLACKHOLE_INCLUDES.
_INCLUDE_RE = re.compile(r'^\s*#\s*include\s+([<"][^<>"]+[>"])')


def parse_paths_file(path: Path) -> list[tuple[str, str]]:
    """Parse the path-rule file into an ordered list of (sign, path) rules.

    Each non-blank, non-comment line is either `+<path>/` (enable) or
    `-<path>/` (disable).  Trailing `#` comments are stripped.  Paths are
    normalised to end with `/` so prefix matching is unambiguous (e.g.
    `brave/browser/` does not match `brave/browser_other/foo.cc`).
    """
    rules: list[tuple[str, str]] = []
    text = path.read_bytes().decode('utf-8')
    for line_no, raw_line in enumerate(text.splitlines(), 1):
        # Strip trailing comments.
        comment_idx = raw_line.find('#')
        if comment_idx != -1:
            raw_line = raw_line[:comment_idx]
        line = raw_line.strip()
        if not line:
            continue
        if line[0] not in ('+', '-'):
            raise ValueError(
                f'{path}:{line_no}: rule lines must start with `+` or `-`, '
                f'got: {line!r}')
        sign = line[0]
        rule_path = line[1:].strip()
        if not rule_path:
            raise ValueError(f'{path}:{line_no}: empty path after `{sign}`')
        if not rule_path.endswith('/'):
            rule_path += '/'
        rules.append((sign, rule_path))
    return rules


def is_path_enabled(source_rel: str, rules: list[tuple[str, str]]) -> bool:
    """Return whether `source_rel` (relative to src/) is enabled by `rules`.

    Longest-matching prefix wins.  If no rule matches, the path is disabled.
    """
    best_len = -1
    best_sign = '-'
    for sign, rule_path in rules:
        if source_rel.startswith(rule_path) and len(rule_path) > best_len:
            best_len = len(rule_path)
            best_sign = sign
    return best_sign == '+'


def blackhole_unwanted_includes() -> None:
    """Remove BLACKHOLE_INCLUDES from any file modified in the working tree.

    Runs after `npm run format` to clean up includes IWYU/fix_includes
    re-added that we never want.  Files to scan are taken from
    `git diff --name-only` against brave-core's HEAD, so only the files
    the pipeline actually touched get rewritten.

    Lines containing `IWYU pragma:` are preserved verbatim so callers can
    defend a specific include with `// IWYU pragma: keep`.
    """
    diff = terminal.run_git('-C', str(repository.brave.root), 'diff',
                            '--name-only')
    files = diff.splitlines() if diff else []
    for rel in files:
        target = repository.brave.root / rel
        if not target.is_file():
            continue
        text = target.read_bytes().decode('utf-8')
        kept: list[str] = []
        removed_any = False
        for line in text.splitlines(keepends=True):
            if 'IWYU pragma:' in line:
                kept.append(line)
                continue
            match = _INCLUDE_RE.match(line)
            if match and match.group(1) in BLACKHOLE_INCLUDES:
                removed_any = True
                continue
            kept.append(line)
        if removed_any:
            target.write_text(''.join(kept), encoding='utf-8', newline='')
            logging.info('Blackholed unwanted includes from %s', rel)


def main():
    parser = argparse.ArgumentParser(
        description='Run IWYU on enabled Brave paths against an existing '
        'Brave build directory.')
    parser.add_argument(
        '--out',
        required=True,
        help='Brave build directory, relative to Chromium\'s src/ '
        '(e.g. `out/Component`).')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose (debug) logging.')
    args = parser.parse_args()

    out_dir = repository.chromium.root / args.out
    if not (out_dir / 'build.ninja').exists():
        raise RuntimeError(
            f'--out does not look like a build dir (no build.ninja): '
            f'{out_dir}')

    if not IWYU_BINARY.exists():
        raise RuntimeError(f'IWYU binary not found at {IWYU_BINARY}. '
                           f'Run brave/tools/cr/iwyu/build_iwyu.py first.')
    if not IWYU_TOOL.exists():
        raise RuntimeError(f'iwyu_tool.py not found at {IWYU_TOOL}. '
                           f'Run brave/tools/cr/iwyu/build_iwyu.py first.')
    if not FIX_INCLUDES.exists():
        raise RuntimeError(f'fix_includes.py not found at {FIX_INCLUDES}. '
                           f'Run brave/tools/cr/iwyu/build_iwyu.py first.')
    if not MAPPINGS_FILE.exists():
        raise RuntimeError(f'IWYU mapping file not found at {MAPPINGS_FILE}.')

    rules = parse_paths_file(PATHS_FILE)
    logging.info('Loaded %d path rule(s) from %s', len(rules), PATHS_FILE)

    logging.info('Generating compile database for %s', out_dir)
    raw_db = compile_db.GenerateWithNinja(str(out_dir))
    full_db = compile_db.ProcessCompileDatabase(raw_db, filtered_args=None)
    logging.info('Compile DB has %d entries', len(full_db))

    filtered_db = []
    for entry in full_db:
        # `entry['file']` may be relative to `entry['directory']` (the build
        # dir) or already absolute; Path / handles both.
        source_path = Path(entry['directory']) / entry['file']
        try:
            source_rel = repository.chromium.to_repo_relative(source_path)
        except ValueError:
            # Source outside the Chromium tree -- not something we own.
            continue
        if is_path_enabled(source_rel.as_posix(), rules):
            filtered_db.append(entry)

    logging.info('Running IWYU on %d source file(s)', len(filtered_db))
    if not filtered_db:
        logging.warning('No source files enabled. Edit %s to enable paths.',
                        PATHS_FILE)
        return 0

    # Write the filtered DB next to the build outputs, distinct from the
    # build's own compile_commands.json (if any).
    filtered_db_path = out_dir / 'iwyu_compile_commands.json'
    filtered_db_path.write_text(json.dumps(filtered_db, indent=2),
                                encoding='utf-8',
                                newline='')
    logging.info('Wrote filtered compile DB to %s', filtered_db_path)

    env = os.environ.copy()
    env['IWYU_BINARY'] = str(IWYU_BINARY)

    # Strip `//` and `/* */` comments from the mapping file into a copy
    # under `--out` so strict JSON parsers (presubmit, json.load) can
    # consume it; IWYU itself is permissive but downstream tooling isn't.
    # Living under `--out` keeps this artefact out of the source tree
    # without needing a .gitignore entry.
    normalised_mappings_path = out_dir / 'iwyu_normalised_mappings.json5'
    normalised_mappings_path.write_text(json_comment_eater.Nom(
        MAPPINGS_FILE.read_bytes().decode('utf-8')),
                                        encoding='utf-8',
                                        newline='')
    logging.info('Wrote normalised mapping file to %s',
                 normalised_mappings_path)

    cpu_count = os.cpu_count() or 1
    # Step 1: run iwyu_tool to produce textual fix suggestions on stdout.
    # We capture stdout (rather than `interactive=True`) so we can pipe it
    # into fix_includes.py in step 2.
    #
    # Args after `--` are forwarded to each IWYU subprocess by iwyu_tool.
    # `-Xiwyu --mapping_file=...` tells IWYU to use our libc++ mapping
    # file; the path must be absolute since IWYU runs each unit cd'd to
    # the compile DB entry's `directory`.
    logging.info('Running iwyu_tool.py')
    iwyu_result = terminal.run([
        sys.executable, IWYU_TOOL, '-p', filtered_db_path, '-j', cpu_count,
        '--', '-Xiwyu', f'--mapping_file={normalised_mappings_path.resolve()}'
    ],
                               env=env)

    # Persist the raw suggestions next to the filtered DB for inspection /
    # post-mortem diffing if a fix goes wrong.
    suggestions_path = out_dir / 'iwyu_suggestions.txt'
    suggestions_path.write_text(iwyu_result.stdout,
                                encoding='utf-8',
                                newline='')
    logging.info('Wrote IWYU suggestions to %s', suggestions_path)

    # Step 2: apply the suggestions in place.  fix_includes.py reads the
    # iwyu_tool report on stdin and rewrites the affected source files.
    #
    # The filenames in the report are relative to each compile DB entry's
    # `directory` field, which for a Chromium-style build is the build dir
    # (e.g. `../../brave/common/importer/foo.cc` rooted at `out/<config>`).
    # Run fix_includes.py with cwd=<build dir> so those paths resolve.
    logging.info('Applying suggestions via fix_includes.py')
    # FIX_INCLUDES is cwd-relative (via repository.chromium.root); absolutise
    # it before changing cwd or the lookup will fail.
    terminal.run([sys.executable, FIX_INCLUDES.resolve()],
                 stdin=iwyu_result.stdout,
                 cwd=out_dir)

    # Step 3: strip BLACKHOLE_INCLUDES from any file the pipeline modified.
    logging.info('Stripping blackholed includes')
    blackhole_unwanted_includes()

    # Step 4: re-format the rewritten files so the resulting diff matches
    logging.info('Running npm run format')
    terminal.run_npm_command('format')

    return 0


if __name__ == '__main__':
    sys.exit(main())
