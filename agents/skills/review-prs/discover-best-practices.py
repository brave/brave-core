# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Discover all best-practice documents and their applicability conditions.

Scans the best-practices directory for .md files and determines which
file-type conditions trigger each document. Conditions are read from an
<!-- applicability: CONDITION --> HTML comment in each file's first 10 lines.
If no comment is found, the script falls back to naming conventions.

Usage:
    python3 discover-best-practices.py <best_practices_dir> [--flags ...]

    Optional flags (pass the ones that are true for the current PR):
      --has-cpp --has-test --has-chromium-src --has-build --has-frontend
      --has-android --has-ios --has-patch --has-nala --has-localization

    When flags are provided, only documents matching those conditions
    (plus "always" documents) are output. When no flags are provided,
    all documents are output with their conditions.

Output (JSON):
    [
      {"doc": "coding-standards.md",
       "path": "/abs/path/coding-standards.md",
       "condition": "has_cpp_files"},
      {"doc": "architecture.md",
       "path": "/abs/path/architecture.md",
       "condition": "always"},
      ...
    ]
"""

import argparse
import json
import os
import re
import sys

# Naming convention fallbacks: prefix/name -> condition
NAMING_CONVENTIONS = {
    "coding-standards": "has_cpp_files",
    "testing-": "has_test_files",
    "build-system": "has_build_files",
    "chromium-src": "has_chromium_src",
    "frontend": "has_frontend_files",
    "android": "has_android_files",
    "ios": "has_ios_files",
    "patches": "has_patch_files",
    "nala": "has_nala_files",
    "localization": "has_localization_files",
    "style-guide": "has_frontend_files",
    "architecture": "always",
    "documentation": "always",
}

# Map CLI flags to condition strings
FLAG_TO_CONDITION = {
    "has_cpp": "has_cpp_files",
    "has_test": "has_test_files",
    "has_chromium_src": "has_chromium_src",
    "has_build": "has_build_files",
    "has_frontend": "has_frontend_files",
    "has_android": "has_android_files",
    "has_ios": "has_ios_files",
    "has_patch": "has_patch_files",
    "has_nala": "has_nala_files",
    "has_localization": "has_localization_files",
}


def extract_applicability(filepath):
    """Read first 10 lines looking for <!-- applicability: CONDITION -->."""
    try:
        with open(filepath) as f:
            for i, line in enumerate(f):
                if i >= 10:
                    break
                m = re.search(r"<!--\s*applicability:\s*(\S+)\s*-->", line,
                              re.IGNORECASE)
                if m:
                    return m.group(1).lower()
    except OSError:
        pass
    return None


def infer_condition(filename):
    """Infer condition from filename using naming conventions."""
    name = filename.lower().removesuffix(".md")
    for prefix, condition in NAMING_CONVENTIONS.items():
        if name == prefix or name.startswith(prefix):
            return condition
    # Default: always include unknown docs
    return "always"


def discover(bp_dir):
    """Discover all .md files and their conditions."""
    results = []
    for fname in sorted(os.listdir(bp_dir)):
        if not fname.endswith(".md"):
            continue
        fpath = os.path.join(bp_dir, fname)
        if not os.path.isfile(fpath):
            continue

        condition = extract_applicability(fpath) or infer_condition(fname)
        results.append({
            "doc": fname,
            "path": os.path.abspath(fpath),
            "condition": condition,
        })
    return results


def main():
    parser = argparse.ArgumentParser(
        description="Discover best-practice documents and their applicability")
    parser.add_argument("bp_dir", help="Path to best-practices directory")
    parser.add_argument("--has-cpp", action="store_true")
    parser.add_argument("--has-test", action="store_true")
    parser.add_argument("--has-chromium-src", action="store_true")
    parser.add_argument("--has-build", action="store_true")
    parser.add_argument("--has-frontend", action="store_true")
    parser.add_argument("--has-android", action="store_true")
    parser.add_argument("--has-ios", action="store_true")
    parser.add_argument("--has-patch", action="store_true")
    parser.add_argument("--has-nala", action="store_true")
    parser.add_argument("--has-localization", action="store_true")
    args = parser.parse_args()

    all_docs = discover(args.bp_dir)

    # If any filter flags are set, filter to matching docs
    active_conditions = set()
    any_flag_set = False
    for flag_attr, condition in FLAG_TO_CONDITION.items():
        if getattr(args, flag_attr.replace("-", "_")):
            active_conditions.add(condition)
            any_flag_set = True

    if any_flag_set:
        filtered = [
            d for d in all_docs if d["condition"] == "always"
            or d["condition"] in active_conditions
        ]
    else:
        filtered = all_docs

    json.dump(filtered, sys.stdout, indent=2)
    print()


if __name__ == "__main__":
    main()
