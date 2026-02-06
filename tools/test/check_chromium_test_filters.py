#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Validates Chromium test filter files against the source tree.

Scans all test filter files in test/filters/ and checks each entry against
actual test definitions found in the Chromium source tree. Identifies:
  - Renamed tests (class renamed upstream but method still exists)
  - Stale entries (test class or method no longer exists)

The script builds an index of all test definitions by scanning .cc/.mm/.cpp
files for standard test macros (TEST, TEST_F, TEST_P, IN_PROC_BROWSER_TEST_F,
etc.) with multiline support. It handles MAYBE_/DISABLED_/MANUAL_ prefixed
names by stripping the prefix (these are compile-time macros that expand to
the real test name).

Usage:
    # Report what would change (dry run):
    python3 tools/test/check_chromium_test_filters.py

    # Apply renames and remove stale entries:
    python3 tools/test/check_chromium_test_filters.py --fix
"""

import argparse
import os
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

# Resolved paths relative to this script's location.
_SCRIPT_DIR = Path(__file__).resolve().parent
_BRAVE_DIR = _SCRIPT_DIR.parents[1]  # brave/
_SRC_DIR = _BRAVE_DIR.parent  # src/
_FILTERS_DIR = _BRAVE_DIR / 'test' / 'filters'

# Standard gtest/Chromium test macros that define test cases.
_TEST_MACROS = (
    'TEST',
    'TEST_F',
    'TEST_P',
    'TYPED_TEST',
    'TYPED_TEST_P',
    'TYPED_TEST_SUITE',
    'IN_PROC_BROWSER_TEST_F',
    'IN_PROC_BROWSER_TEST_P',
    'FUZZ_TEST',
)

_MACRO_ALTERNATION = '|'.join(_TEST_MACROS)

# Compile-time prefixes that are stripped from test names. MAYBE_ expands to
# either the real name or DISABLED_<name> depending on platform.
_STRIP_PREFIXES = ('MAYBE_', 'DISABLED_', 'MANUAL_')

# Minimum class name length to consider as a rename candidate. Short names
# like "Test" produce false positives.
_MIN_CLASS_NAME_LENGTH = 8

# Minimum similarity score (0-1) for a class rename to be considered
# high-confidence.
_MIN_RENAME_SIMILARITY = 0.8


def _find_files_with_test_macros(src_dir):
    """Find all C++ files containing test macro definitions."""
    pattern = (
        r'\b(TEST_F|TEST_P|IN_PROC_BROWSER_TEST_F|IN_PROC_BROWSER_TEST_P'
        r'|TEST|TYPED_TEST|TYPED_TEST_P|FUZZ_TEST'
        r'|INSTANTIATE_TEST_SUITE_P)\s*\(')

    all_files = set()
    for entry in src_dir.iterdir():
        if not entry.is_dir():
            continue
        if entry.name in ('out', '.git', '.cipd', 'buildtools', 'node_modules',
                          '.vscode'):
            continue
        try:
            result = subprocess.run([
                '/usr/bin/grep', '-r', '-l', '-E', pattern, '--include=*.cc',
                '--include=*.mm', '--include=*.cpp',
                str(entry)
            ],
                                    capture_output=True,
                                    text=True,
                                    timeout=300,
                                    check=False)
            all_files.update(f for f in result.stdout.strip().splitlines()
                             if f)
        except subprocess.TimeoutExpired:
            print(f'  WARNING: grep timed out scanning {entry.name}/',
                  file=sys.stderr)
    return all_files


def _strip_macro_prefix(name):
    """Strip MAYBE_/DISABLED_/MANUAL_ prefix from a test name."""
    for pfx in _STRIP_PREFIXES:
        if name.startswith(pfx):
            return name[len(pfx):]
    return name


def build_test_index(src_dir):
    """Build an index of all test definitions from the source tree.

    Returns:
        (test_index, param_prefixes) where:
        - test_index: dict mapping class_name -> set of method_names
        - param_prefixes: dict mapping class_name -> set of instantiation
          prefixes
    """
    print('Building test index from source tree...', file=sys.stderr)
    print(f'  Source directory: {src_dir}', file=sys.stderr)

    test_index = defaultdict(set)
    param_prefixes = defaultdict(set)

    macro_re = re.compile(
        rf'\b({_MACRO_ALTERNATION})\s*\(\s*(\w+)\s*,\s*(\w+)')
    instantiate_re = re.compile(
        r'\bINSTANTIATE_TEST_SUITE_P\s*\(\s*(\w+)\s*,\s*(\w+)')

    all_files = _find_files_with_test_macros(src_dir)
    print(f'  Scanning {len(all_files)} files...', file=sys.stderr)

    for filepath in sorted(all_files):
        try:
            with open(filepath, 'r', errors='ignore') as fh:
                content = fh.read()
        except (OSError, IOError):
            continue

        for m in macro_re.finditer(content):
            class_name = _strip_macro_prefix(m.group(2))
            method_name = _strip_macro_prefix(m.group(3))
            test_index[class_name].add(method_name)

        for m in instantiate_re.finditer(content):
            prefix = m.group(1)
            class_name = _strip_macro_prefix(m.group(2))
            param_prefixes[class_name].add(prefix)

    total_methods = sum(len(v) for v in test_index.values())
    print(
        f'  Found {len(test_index)} test classes with {total_methods} '
        f'methods',
        file=sys.stderr)
    print(f'  Found {len(param_prefixes)} parameterized test classes',
          file=sys.stderr)

    return test_index, param_prefixes


def parse_test_name(test_name):
    """Parse a gtest filter name into its components.

    Handles formats like:
        ClassName.MethodName
        Prefix/ClassName.MethodName/ParamIndex
        ClassName.MethodName*  (wildcard)
        ClassName.*

    Returns:
        (prefix, class_name, method_name, has_wildcard, param_suffix)
    """
    has_wildcard = test_name.endswith('*')
    if has_wildcard:
        test_name = test_name[:-1]

    prefix = param_suffix = None
    if '/' in test_name:
        parts = test_name.split('/')
        if len(parts) == 2:
            if '.' in parts[0]:
                test_name, param_suffix = parts[0], parts[1]
            else:
                prefix, test_name = parts[0], parts[1]
        elif len(parts) == 3:
            prefix, test_name, param_suffix = parts[0], parts[1], parts[2]

    if '.' in test_name:
        class_name, method_name = test_name.split('.', 1)
    else:
        class_name, method_name = test_name, None

    return prefix, class_name, method_name, has_wildcard, param_suffix


def reconstruct_test_name(prefix, class_name, method_name, has_wildcard,
                          param_suffix):
    """Reconstruct a full gtest filter name from components."""
    name = ''
    if prefix:
        name += prefix + '/'
    name += class_name
    if method_name is not None:
        name += '.' + method_name
    if param_suffix is not None:
        name += '/' + param_suffix
    if has_wildcard:
        name += '*'
    return name


def test_exists(test_index, class_name, method_name, has_wildcard):
    """Check if a test exists in the index."""
    if class_name not in test_index:
        return False
    methods = test_index[class_name]
    if method_name == '' or method_name is None:
        return has_wildcard
    if method_name in methods:
        return True
    if has_wildcard:
        return any(m.startswith(method_name) for m in methods)
    return False


def _class_similarity(old_name, new_name):
    """Compute similarity score between two class names.

    Uses common prefix ratio, boosted if one name is a substring of the
    other (common for feature-flag suffix additions/removals).
    """
    common = 0
    for i in range(min(len(old_name), len(new_name))):
        if old_name[i] == new_name[i]:
            common += 1
        else:
            break
    max_len = max(len(old_name), len(new_name))
    ratio = common / max_len if max_len else 0
    if old_name in new_name or new_name in old_name:
        return max(ratio, 0.85)
    return ratio


def find_class_rename(test_index, prefix, class_name, method_name,
                      has_wildcard, param_suffix):
    """Find a high-confidence class rename for a missing test.

    Only considers cases where the class was renamed but the exact method
    name still exists in the new class.

    Returns:
        New test name string if a rename was found, None otherwise.
    """
    if class_name in test_index or not method_name:
        return None

    candidates = []
    for existing_class, methods in test_index.items():
        if len(existing_class) < _MIN_CLASS_NAME_LENGTH:
            continue
        if any(existing_class.startswith(p) for p in _STRIP_PREFIXES):
            continue

        method_match = (method_name in methods or
                        (has_wildcard
                         and any(m.startswith(method_name) for m in methods)))
        if not method_match:
            continue

        sim = _class_similarity(class_name, existing_class)
        if sim > _MIN_RENAME_SIMILARITY:
            candidates.append((existing_class, sim))

    if not candidates:
        return None

    candidates.sort(key=lambda x: -x[1])
    best_class = candidates[0][0]
    return reconstruct_test_name(prefix, best_class, method_name, has_wildcard,
                                 param_suffix)


def _clean_filter_lines(lines):
    """Clean up filter file content after modifications.

    - Deduplicates identical filter entries
    - Removes entries covered by wildcard patterns (e.g. -Foo.Bar when
      -Foo.* exists)
    - Collapses consecutive blank lines
    - Removes orphaned comment blocks (comments not followed by any
      filter entries)
    - Preserves header comments (## lines) and license blocks
    """
    # Deduplicate filter lines.
    seen = set()
    deduped = []
    for line in lines:
        stripped = line.strip()
        if stripped.startswith('-'):
            if stripped in seen:
                continue
            seen.add(stripped)
        deduped.append(line)

    # Collect wildcard patterns.
    wildcard_patterns = set()
    for line in deduped:
        stripped = line.strip()
        if stripped.startswith('-') and stripped.endswith('.*'):
            wildcard_patterns.add(stripped[1:-1])  # "ClassName."

    # Remove entries covered by wildcards.
    filtered = []
    for line in deduped:
        stripped = line.strip()
        if stripped.startswith('-') and not stripped.endswith('.*'):
            test_name = stripped[1:]
            if any(test_name.startswith(wp) for wp in wildcard_patterns):
                continue
        filtered.append(line)

    # Collapse consecutive blank lines.
    collapsed = []
    for line in filtered:
        if line.strip() == '' and collapsed and collapsed[-1].strip() == '':
            continue
        collapsed.append(line)

    # Remove orphaned comment blocks.
    result = []
    i = 0
    while i < len(collapsed):
        stripped = collapsed[i].strip()

        # Non-comment lines pass through.
        if not stripped.startswith('#') or stripped.startswith('##'):
            result.append(collapsed[i])
            i += 1
            continue

        # Collect the comment block.
        block = []
        while i < len(collapsed) and collapsed[i].strip().startswith('#'):
            block.append(collapsed[i])
            i += 1

        # Always keep header/license comments.
        is_preserved = any(c.strip().startswith('##') or any(
            k in c for k in ('License', 'license', 'Copyright', 'mozilla.org'))
                           for c in block)

        # Check if filter lines follow this comment block.
        has_filter_after = False
        j = i
        while j < len(collapsed):
            s = collapsed[j].strip()
            if s == '' or s.startswith('#'):
                break
            if s.startswith('-'):
                has_filter_after = True
                break
            j += 1

        if has_filter_after or is_preserved:
            result.extend(block)

    # Final cleanup.
    final = []
    for line in result:
        if line.strip() == '' and final and final[-1].strip() == '':
            continue
        final.append(line)

    while final and final[-1].strip() == '':
        final.pop()
    if final and not final[-1].endswith('\n'):
        final[-1] += '\n'

    return final


def process_filter_file(filepath, test_index, apply_fix=False):
    """Process a single filter file.

    Args:
        filepath: Path to the .filter file.
        test_index: dict mapping class_name -> set of method_names.
        apply_fix: If True, modify the file in place.

    Returns:
        (renames, removals) where:
        - renames: list of (line_num, old_name, new_name)
        - removals: list of (line_num, test_name)
    """
    with open(filepath, 'r') as f:
        lines = f.readlines()

    renames = []
    removals = []
    rename_map = {}
    remove_lines = set()

    for idx, line in enumerate(lines):
        stripped = line.strip()
        if not stripped or stripped.startswith('#'):
            continue

        test_name = stripped[1:] if stripped.startswith('-') else stripped
        if ' #' in test_name:
            test_name = test_name[:test_name.index(' #')].strip()

        prefix, class_name, method_name, has_wildcard, param_suffix = \
            parse_test_name(test_name)

        if test_exists(test_index, class_name, method_name, has_wildcard):
            continue

        new_name = find_class_rename(test_index, prefix, class_name,
                                     method_name, has_wildcard, param_suffix)

        if new_name:
            renames.append((idx + 1, test_name, new_name))
            rename_map[idx] = (test_name, new_name)
        else:
            removals.append((idx + 1, test_name))
            remove_lines.add(idx)

    if not apply_fix:
        return renames, removals

    # Apply changes.
    new_lines = []
    for i, line in enumerate(lines):
        if i in remove_lines:
            continue
        if i in rename_map:
            old_name, new_name = rename_map[i]
            line = line.replace(f'-{old_name}', f'-{new_name}')
        new_lines.append(line)

    cleaned = _clean_filter_lines(new_lines)

    with open(filepath, 'w') as f:
        f.writelines(cleaned)

    return renames, removals


def main():
    parser = argparse.ArgumentParser(
        description='Check Chromium test filter files against the source tree')
    parser.add_argument(
        '--fix',
        action='store_true',
        help='Apply renames and remove stale entries (default: dry run)')
    parser.add_argument('--src-dir',
                        type=Path,
                        default=_SRC_DIR,
                        help='Path to the Chromium src/ directory')
    parser.add_argument('--filters-dir',
                        type=Path,
                        default=_FILTERS_DIR,
                        help='Path to the test/filters/ directory')
    args = parser.parse_args()

    test_index, _ = build_test_index(args.src_dir)

    filter_files = sorted(args.filters_dir.glob('*.filter'))
    if not filter_files:
        print(f'No .filter files found in {args.filters_dir}', file=sys.stderr)
        return 1

    total_renames = 0
    total_removals = 0

    for filepath in filter_files:
        renames, removals = process_filter_file(filepath,
                                                test_index,
                                                apply_fix=args.fix)

        if not renames and not removals:
            continue

        print(f'\n{filepath.name}:')
        for line_num, old_name, new_name in renames:
            total_renames += 1
            print(f'  RENAME L{line_num}: {old_name}')
            print(f'           -> {new_name}')
        for line_num, name in removals:
            total_removals += 1
            if not args.fix:
                print(f'  REMOVE L{line_num}: {name}')

        if args.fix:
            print(f'  Applied: {len(renames)} renames, '
                  f'{len(removals)} removals')

    action = 'Applied' if args.fix else 'Found'
    print(f'\n{action}: {total_renames} renames, {total_removals} removals')

    return 0 if (total_renames + total_removals) == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
