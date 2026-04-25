#!/usr/bin/env python3
#
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
find_rebase_commits.py

Purpose:
    Scan a git repository to determine a range of commits associated with a
    Chromium rebase number (e.g., cr148) in commit subjects. Outputs a commit
    range in `oldest..newest` format suitable for git operations.

Usage:
    python find_rebase_commits.py --tag cr148 [--start <commit>] [--verbose]
    python find_rebase_commits.py [--start <commit>] [--verbose]  # auto-detect rebase number

Arguments:
    --start     Starting commit (default: HEAD)
    --tag       Rebase number to look for (e.g., cr123). If omitted,
                automatically detects the most frequent rebase number in the
                first N commits.
    --verbose   Include commit hash, author, and truncated subject in output.
"""

import subprocess
import argparse
import re
import sys
from collections import Counter

MAX_LOOKAHEAD_NO_CR = 20  # commits to scan if rebase number not supplied
MAX_EMPTY_COMMITS = 20  # stop after this many commits without match
TRUNCATE_SUBJECT_LEN = 80  # max length of commit subject in verbose mode
CR_REGEX = r"cr\d{3,}"  # pattern to detect rebase numbers


def run_git_log(start_ref, max_count=None):
    # Fetch commits with UTF-8 decoding, replace invalid bytes
    cmd = ["git", "-c", "i18n.logOutputEncoding=utf-8", "log"]
    if max_count is not None:
        cmd.append(f"-n{max_count}")
    cmd.append("--pretty=format:%H%x01%an%x01%s")
    cmd.append(start_ref)

    try:
        output = subprocess.check_output(cmd,
                                         encoding="utf-8",
                                         errors="replace")
    except subprocess.CalledProcessError as e:
        print("Error running git log:", e, file=sys.stderr)
        sys.exit(1)

    commits = []
    for line in output.splitlines():
        parts = line.split("\x01", 2)
        if len(parts) == 3:
            commits.append((parts[0], parts[1], parts[2]))
    return commits


def find_most_frequent_cr(commits):
    # Pick the most frequently occurring rebase number in commit subjects
    cr_pattern = re.compile(CR_REGEX)
    cr_list = []
    for _, _, subject in commits:
        match = cr_pattern.search(subject)
        if match:
            cr_list.append(match.group(0))
    if not cr_list:
        return None
    counter = Counter(cr_list)
    most_common_cr, _ = counter.most_common(1)[0]
    return most_common_cr


def find_commit_range(commits, cr_id):
    # Walk commits until MAX_EMPTY_COMMITS consecutive commits do not match
    # rebase number
    cr_regex = re.compile(cr_id)
    last_match_index = None
    empty_count = 0

    for i, (_, _, subject) in enumerate(commits):
        if cr_regex.search(subject):
            last_match_index = i
            empty_count = 0
        else:
            empty_count += 1

        if last_match_index is not None and empty_count >= MAX_EMPTY_COMMITS:
            break

    if last_match_index is None:
        return []

    return commits[:last_match_index + 1]


def truncate(text, max_len=TRUNCATE_SUBJECT_LEN):
    # Truncate commit subjects for verbose output
    if len(text) <= max_len:
        return text
    return text[:max_len - 3] + "..."


def main():
    parser = argparse.ArgumentParser(
        description="Find commit range by rebase number.")
    parser.add_argument("--start",
                        help="Starting commit (default: HEAD)",
                        default="HEAD")
    parser.add_argument(
        "--tag", help="Rebase number (e.g., cr123). Auto-detect if omitted.")
    parser.add_argument("--verbose",
                        action="store_true",
                        help="Show commit details")
    args = parser.parse_args()

    auto_detected = False

    if args.tag is None:
        # auto-detect rebase number from first N commits
        first_commits = run_git_log(args.start, max_count=MAX_LOOKAHEAD_NO_CR)
        cr_id = find_most_frequent_cr(first_commits)
        if cr_id is None:
            print(
                f"No rebase number found in first {MAX_LOOKAHEAD_NO_CR} commits.",
                file=sys.stderr)
            sys.exit(1)
        auto_detected = True
    else:
        cr_id = args.tag

    commits = run_git_log(args.start)
    commit_range = find_commit_range(commits, cr_id)

    if not commit_range:
        print(f"No commits found matching rebase number {cr_id}.")
        sys.exit(0)

    # commits are newest -> oldest; git range is oldest..newest
    newest = commit_range[0][0]
    oldest = commit_range[-1][0]

    print(f"{oldest}..{newest}")

    if args.verbose:
        if auto_detected:
            print(f"\nAuto-detected rebase number: {cr_id}")
        else:
            print(f"\nUsing rebase number: {cr_id}")

        print("\nCommits:")
        for commit_hash, author, subject in commit_range:
            short_hash = commit_hash[:10]
            print(f"{short_hash}  {author:20}  {truncate(subject)}")


if __name__ == "__main__":
    main()
