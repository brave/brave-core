#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Discover best-practice documents and determine which ones
apply to a set of changed files.

Scans the best-practices directory and uses filename conventions
to match documents to file-type categories. Documents that don't
match any specific category are treated as "always applicable".

Usage:
    python3 discover-bp-docs.py <bp_dir> [--changed-files FILE...]
    python3 discover-bp-docs.py <bp_dir> --changed-files-stdin

Output (JSON):
    [
      {"doc": "coding-standards.md",
       "path": "/full/path/coding-standards.md",
       "category": "cpp"},
      {"doc": "architecture.md",
       "path": "/full/path/architecture.md",
       "category": "always"},
      ...
    ]

Category mapping (based on filename prefixes/keywords):
    cpp:          coding-standards*, style-guide*
    test:         testing-*
    chromium_src: chromium-src*
    build:        build-*
    frontend:     frontend*
    android:      android*
    ios:          ios*
    patch:        patch*
    nala:         nala*
    always:       everything else (architecture, documentation, etc.)
"""

import argparse
import json
import os
import sys

# Maps filename prefix/keyword → category
CATEGORY_RULES = [
    ("coding-standards", "cpp"),
    ("style-guide", "cpp"),
    ("testing-", "test"),
    ("chromium-src", "chromium_src"),
    ("build-", "build"),
    ("frontend", "frontend"),
    ("android", "android"),
    ("ios", "ios"),
    ("patch", "patch"),
    ("nala", "nala"),
    ("localization", "frontend"),
    ("plaster", "cpp"),
    ("ui-views", "cpp"),
]

# Maps category → file extension/path patterns that trigger it
CATEGORY_FILE_PATTERNS = {
    "cpp": {
        "extensions": {".cc", ".h", ".mm", ".cpp"},
        "path_contains": [],
    },
    "test": {
        "extensions": set(),
        "path_contains": [],
        "name_patterns": [
            "_test.cc",
            "_browsertest.cc",
            "_unittest.cc",
            ".test.ts",
            ".test.tsx",
        ],
    },
    "chromium_src": {
        "extensions": set(),
        "path_contains": ["chromium_src/"],
    },
    "build": {
        "extensions": {".gn", ".gni"},
        "path_contains": [],
        "name_patterns": ["BUILD.gn", "DEPS"],
    },
    "frontend": {
        "extensions": {".ts", ".tsx", ".html", ".css"},
        "path_contains": [],
    },
    "android": {
        "extensions": {".java", ".kt"},
        "path_contains": ["android/"],
    },
    "ios": {
        "extensions": {".swift"},
        "path_contains": ["ios/"],
    },
    "patch": {
        "extensions": {".patch"},
        "path_contains": ["patches/"],
    },
    "nala": {
        "extensions": {".icon", ".svg"},
        "path_contains": [
            "res/drawable",
            "res/values",
            "res/values-night",
            "components/vector_icons/",
        ],
    },
}


def categorize_doc(filename):
    """Determine the category of a best-practice document by its filename."""
    name = filename.lower()
    for prefix, category in CATEGORY_RULES:
        if name.startswith(prefix):
            return category
    return "always"


def get_active_categories(changed_files):
    """Determine which categories are active based on changed files."""
    categories = {"always"}  # always-applicable docs are always included

    for filepath in changed_files:
        basename = os.path.basename(filepath)
        _, ext = os.path.splitext(filepath)
        ext = ext.lower()

        for cat, patterns in CATEGORY_FILE_PATTERNS.items():
            # Check extensions
            if ext in patterns.get("extensions", set()):
                categories.add(cat)
                continue

            # Check path contains
            for substr in patterns.get("path_contains", []):
                if substr in filepath:
                    categories.add(cat)
                    break

            # Check name patterns
            for pattern in patterns.get("name_patterns", []):
                if basename.endswith(pattern) or basename == pattern:
                    categories.add(cat)
                    break

    return categories


def discover_docs(bp_dir, changed_files=None):
    """Discover and filter best-practice documents."""
    if not os.path.isdir(bp_dir):
        print(f"Error: {bp_dir} is not a directory", file=sys.stderr)
        sys.exit(1)

    all_docs = []
    for f in sorted(os.listdir(bp_dir)):
        if not f.endswith(".md"):
            continue
        category = categorize_doc(f)
        all_docs.append({
            "doc": f,
            "path": os.path.abspath(os.path.join(bp_dir, f)),
            "category": category,
        })

    if changed_files is None:
        # No filtering — return all docs
        return all_docs

    active_categories = get_active_categories(changed_files)

    return [d for d in all_docs if d["category"] in active_categories]


def list_categories(bp_dir):
    """List all documents with their categories for use by other skills."""
    docs = discover_docs(bp_dir, changed_files=None)
    return [{"doc": d["doc"], "category": d["category"]} for d in docs]


def main():
    parser = argparse.ArgumentParser(
        description="Discover applicable best-practice documents")
    parser.add_argument("bp_dir", help="Path to the best-practices directory")
    parser.add_argument(
        "--changed-files",
        nargs="*",
        help="List of changed file paths to filter by",
    )
    parser.add_argument(
        "--changed-files-stdin",
        action="store_true",
        help="Read changed file paths from stdin (one per line)",
    )
    parser.add_argument(
        "--list-categories",
        action="store_true",
        help="List all documents with their categories (no filtering)",
    )
    args = parser.parse_args()

    if args.list_categories:
        result = list_categories(args.bp_dir)
        json.dump(result, sys.stdout, indent=2)
        print()
        return

    changed_files = args.changed_files
    if args.changed_files_stdin:
        changed_files = [line.strip() for line in sys.stdin if line.strip()]

    docs = discover_docs(args.bp_dir, changed_files)
    json.dump(docs, sys.stdout, indent=2)
    print()


if __name__ == "__main__":
    main()
