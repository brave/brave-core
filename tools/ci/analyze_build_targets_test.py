#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Tests for analyze_build_targets.py

Uses pre-generated test build directories in test_build_dirs/.
Run with: vpython3 analyze_build_targets_test.py
"""

import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
SRC_DIR = SCRIPT_DIR.parent.parent.parent  # brave/tools/ci -> src
SCRIPT = SCRIPT_DIR / 'analyze_build_targets.py'
TEST_OUT_DIR = SCRIPT_DIR / 'test_build_dirs'


def ensure_build_dirs():
    """Run gn gen for any test dirs missing build.ninja."""
    for config_dir in TEST_OUT_DIR.iterdir():
        if not config_dir.is_dir():
            continue
        if not (config_dir / 'build.ninja').exists():
            build_dir = str(config_dir.relative_to(SRC_DIR))
            subprocess.run(['gn', 'gen', build_dir],
                           cwd=SRC_DIR,
                           capture_output=True,
                           check=False)


# Test cases: (config, file, expected_needs_build, description)
TEST_CASES = [
    ('mac', 'browser/brave_stats/brave_stats_updater.cc', True,
     'C++ triggers mac'),
    ('mac', 'android/java/org/Example.java', False, 'Java skips mac'),
    ('ios', 'components/brave_wallet/browser/brave_wallet_service.cc', True,
     'C++ triggers ios'),
    ('ios', 'android/java/org/Example.java', False, 'Java skips ios'),
    ('win', 'browser/brave_stats/brave_stats_updater.cc', True,
     'C++ triggers win'),
    ('win', 'android/java/org/Example.java', False, 'Java skips win'),
    ('linux', 'browser/brave_stats/brave_stats_updater.cc', True,
     'C++ triggers linux'),
    ('linux', 'android/java/org/Example.java', False, 'Java skips linux'),
    ('android', 'android/java/org/Example.java', True,
     'Java triggers android'),
    ('android', 'browser/brave_stats/brave_stats_updater.cc', True,
     'C++ triggers android'),
    ('brave_origin', 'browser/brave_stats/brave_stats_updater.cc', True,
     'C++ triggers brave_origin'),
]


def run_test(config, file, expected):
    """Run a single test."""
    config_dir = TEST_OUT_DIR / config
    if not (config_dir / 'build.ninja').exists():
        return None, 'SKIP (gn gen failed - cross-compile not available)'

    result = subprocess.run(
        [sys.executable,
         str(SCRIPT),
         str(config_dir), '--files', file],
        cwd=SRC_DIR,
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode == 2:
        return False, f'ERROR: {result.stderr.strip()}'

    actual = result.returncode == 1
    if actual == expected:
        return True, 'PASS'
    exp = "build" if expected else "skip"
    got = "build" if actual else "skip"
    return False, f'FAIL (expected {exp}, got {got})'


def main():
    ensure_build_dirs()
    passed = failed = skipped = 0

    for config, file, expected, desc in TEST_CASES:
        ok, msg = run_test(config, file, expected)
        print(f'[{config}] {desc}: {msg}')
        if ok is None:
            skipped += 1
        elif ok:
            passed += 1
        else:
            failed += 1

    print(f'\nResults: {passed} passed, {failed} failed, {skipped} skipped')
    return 0 if failed == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
