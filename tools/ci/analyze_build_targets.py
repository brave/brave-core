#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Analyzes whether a build is needed based on PR changes.

Exit codes:
    0 - No build needed (no dependencies found)
    1 - Build needed (dependencies found)
    2 - Error

Usage:
    python analyze_build_targets.py out/Component_arm64
    python analyze_build_targets.py out/Android --base origin/main
    python analyze_build_targets.py out/BraveOrigin --files browser/foo.cc
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

SRC_DIR = Path(__file__).resolve().parent.parent.parent.parent

# Source file extensions that gn analyze can evaluate.
# Any other file type change will trigger a build.
SOURCE_EXTENSIONS = frozenset([
    '.c',
    '.cc',
    '.cpp',
    '.h',
    '.hpp',  # C/C++
    '.m',
    '.mm',  # Objective-C/C++
    '.rs',  # Rust
    '.java',  # Java
    '.swift',  # Swift
    '.gn',
    '.gni',  # gn
])

# File extensions that never affect builds and should be ignored.
IGNORED_EXTENSIONS = frozenset([
    '.md',  # Markdown
])
sys.path.insert(0, str(SRC_DIR / 'third_party' / 'depot_tools'))

from scm import GIT


def get_changed_files(base_branch: str, repo_path: Path) -> list[str]:
    """Get list of changed files using depot_tools scm module."""
    status = GIT.CaptureStatus(str(repo_path), base_branch)
    return [f"//brave/{f}" for _, f in status]


def run_gn_analyze(build_dir: str, files: list[str], src_root: Path) -> dict:
    """Run gn analyze and return result."""
    input_data = {
        "files": files,
        "test_targets": [],
        "additional_compile_targets": ["all"],
    }

    with tempfile.NamedTemporaryFile(mode="w", suffix=".json",
                                     delete=False) as f:
        json.dump(input_data, f)
        input_path = f.name

    with tempfile.NamedTemporaryFile(mode="w", suffix=".json",
                                     delete=False) as f:
        output_path = f.name

    try:
        subprocess.run(
            ["gn", "analyze", build_dir, input_path, output_path],
            cwd=src_root,
            capture_output=True,
            text=True,
            check=True,
        )
        with open(output_path) as f:
            return json.load(f)
    finally:
        os.unlink(input_path)
        os.unlink(output_path)


def main():
    parser = argparse.ArgumentParser(
        description="Check if a build is needed based on PR changes.")
    parser.add_argument(
        "build_dir",
        help="Path to build directory (e.g., out/Component_arm64)")
    parser.add_argument("--base",
                        default="origin/master",
                        help="Base branch (default: origin/master)")
    parser.add_argument("--files",
                        nargs="+",
                        help="Specific files to analyze (instead of git diff)")

    args = parser.parse_args()

    # Resolve build directory (can be absolute or relative to cwd)
    build_path = Path(args.build_dir).resolve()
    if not (build_path / "build.ninja").is_file():
        print(f"Error: {args.build_dir} is not a valid build directory",
              file=sys.stderr)
        sys.exit(2)

    # Get path relative to SRC_DIR for gn analyze
    try:
        build_dir = str(build_path.relative_to(SRC_DIR))
    except ValueError:
        print(f"Error: {args.build_dir} must be under {SRC_DIR}",
              file=sys.stderr)
        sys.exit(2)

    # Get changed files
    if args.files:
        files = []
        for f in args.files:
            if f.startswith("//"):
                files.append(f)
            elif f.startswith("brave/"):
                files.append(f"//{f}")
            else:
                files.append(f"//brave/{f}")
    else:
        try:
            files = get_changed_files(args.base, SRC_DIR / "brave")
        except Exception as e:
            print(f"Error getting changed files: {e}", file=sys.stderr)
            sys.exit(2)

    if not files:
        print("No build needed (no changed files)")
        sys.exit(0)

    # Separate source files from non-source files, ignoring doc files
    source_files = []
    non_source_files = []
    for f in files:
        ext = Path(f).suffix.lower()
        if ext in IGNORED_EXTENSIONS:
            continue  # Skip files that never affect builds
        if ext in SOURCE_EXTENSIONS:
            source_files.append(f)
        else:
            non_source_files.append(f)

    # Non-source file changes always trigger a build
    if non_source_files:
        print(
            f"Build needed (non-source files changed: {len(non_source_files)})"
        )
        sys.exit(1)

    # No source files to analyze
    if not source_files:
        print("No build needed (no source files changed)")
        sys.exit(0)

    # Run analysis on source files
    try:
        result = run_gn_analyze(build_dir, source_files, SRC_DIR)
    except subprocess.CalledProcessError as e:
        error_msg = e.stderr or e.stdout or str(e)
        print(f"Error running gn analyze: {error_msg}", file=sys.stderr)
        sys.exit(2)

    if "error" in result:
        print(f"Error: {result['error']}", file=sys.stderr)
        sys.exit(2)

    needs_build = result.get("status") != "No dependency"

    if needs_build:
        target_count = len(result.get("compile_targets", []))
        print(f"Build needed ({target_count} affected targets)")
        sys.exit(1)
    else:
        print("No build needed")
        sys.exit(0)


if __name__ == "__main__":
    main()
