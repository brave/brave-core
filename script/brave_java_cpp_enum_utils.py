# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Brave-specific utilities for Java C++ enum parsing.

This module provides Brave-specific modifications to the enum parsing behavior,
particularly for handling comma-separated enum entries on a single line.
"""


def SplitBraveJavaEnumEntries(lines):
    """Split lines with BRAVE_JAVA_ENUM comment into separate enum entry lines.

    Args:
        lines: List of strings representing lines from the input file.

    Returns:
        List of strings with each enum entry on its own line.
    """
    processed_lines = []

    for line in lines:
        # Only process lines that contain the BRAVE_JAVA_ENUM comment
        if '/* BRAVE_JAVA_ENUM: */' in line and ',' in line:
            # Remove the comment and inline comments before splitting
            line_without_brave_comment = line.replace('/* BRAVE_JAVA_ENUM: */',
                                                      '').strip()
            # Split by comma and create separate lines for each entry
            entries = [
                entry.strip()
                for entry in line_without_brave_comment.rstrip(',').split(',')
            ]
            for entry in entries:
                processed_lines.append(f"{entry},\n")
        else:
            processed_lines.append(line)

    return processed_lines
