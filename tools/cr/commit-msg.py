#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# This script has several convenience features when committing changes to
# brave-core.
#
# 1. Chromium upgrade branches automatically added as tags
#
#    Any branch that starts with "cr" and is followed by a number will be added
#    to the commit message as a tag (e.g. [cr100] Some change).
#
# 2. Tags can be passed in via the environment variable tags
#
#    For instance `tags=CodeHealth,canary git commit -m "Some change"` will
#    add [CodeHealth] and [canary] to the commit message.
#
# 3. Labels in the branch name behind '+' will be added as tags
#
#    This means that a branch `canary+fix-failure` will have [canary] added to
#    the commit message.
#
# 4. `culprit` environment variable for google source links
#
#    If the culprit environment variable is set, it will be used to add links to
#    the commit message. The culprit variable should be a comma-separated list
#    of commit hashes. The script will look up the commit messages for each hash
#    and add them to the commit message. The links will be in the format
#    https://chromium.googlesource.com/chromium/src/+/{hash}.
#
# 5. `issue` environment variable for "Resolves" links
#
#    The `issue` environment variable can be set to a comma-separated list of
#    issue numbers. The script will add "Resolves" links at the end of the
#    commit message.
#
# 6. `fixup!` commits are ignored
#
#    If the commit message starts with "fixup!", the script will not add any
#    tags or links to the commit message.
#
# 7. Issue numbers in the branch name
#
#    If the branch name contains "issue-" followed by a number, the script will
#    add a "Resolves" link to the commit message. For example, if the branch
#    name is "codehealth+fix-issue-123", the commit message will include a link
#    to "Resolves https://github.com/brave/brave-browser/issues/123".
#
# This script can be set up either as a commit-msg hook, or as a
# prepare-commit-msg hook. The difference is that the commit-msg hook will run
# after the commit message is created, while the prepare-commit-msg hook will
# run before the commit message is created, which allows you to inspect the
# insertions.
#
# To install this script as a commit-msg hook, run the following command:
#   cp tools/cr/commit-msg.py .git/hooks/commit-msg
#

import os
import sys
import subprocess
import re

PREFIXES_FOR_UPGRADE_COMMITS = [
    'Update from Chromium ',
    'Conflict-resolved patches from Chromium ',
    'Update patches from Chromium ',
    'Updated strings for Chromium ',
]


def main():
    if (os.path.exists('.git/rebase-merge')
            or os.path.exists('.git/rebase-apply')
            or os.path.exists('.git/CHERRY_PICK_HEAD')):
        # There should be no changes to commit messages during a rebase or
        # cherry-pick.
        return 0

    commit_msg_file = sys.argv[1]

    # Read the message provided so we can check for duplicate insertions
    # beforehand.
    with open(commit_msg_file, 'r') as f:
        commit_message = f.read()

    if not commit_message or not commit_message.split('\n', 1)[0].strip():
        # We should not commmit if the commit message first line is empty as
        # this will cause any tags to become the effective message.
        return 1

    # Skip fixup! changes.
    if commit_message.startswith('fixup!') or commit_message.startswith(
            'amend!'):
        return 0

    first_line = commit_message.split('\n', 1)[0]

    # Getting the branch name to see if there's any tags that can be deduced
    # from it.
    branch_name = subprocess.check_output(
        ['git', 'rev-parse', '--abbrev-ref', 'HEAD']).strip().decode()

    tags = set()
    # Checking for chromium upgrade branches
    cr_branch_pattern = r"^cr\d+$"
    if re.fullmatch(cr_branch_pattern, branch_name):
        # The branch tag is excluded from the upgrade commit messages.
        if not any(suffix in first_line
                   for suffix in PREFIXES_FOR_UPGRADE_COMMITS):
            tags.add(branch_name)
    elif '+' in branch_name:
        # This is the simple case where `canary+fix-failure` produces a
        # [canary] tag.
        tags.update(branch_name.split('+')[:-1])

    # Checking for tags passed in via the environment variable tags
    tags_env = os.getenv("tags")
    if tags_env:
        tags.update(tags_env.split(','))

    # Making sure we are not re-inserting tags that are already in the commit
    # message
    tags = [f'[{tag}]' for tag in tags if f'[{tag}]' not in first_line]

    # These are the report links that are added to the commit message when
    # there's an upstream culprit.
    culprit_hashes = []
    if os.getenv("culprit"):
        culprit_hashes = os.getenv("culprit").split(',')
    culprit_hashes = [
        culprit_hash for culprit_hash in culprit_hashes
        if culprit_hash not in commit_message
    ]

    culprit_output = []
    for culprit_hash in culprit_hashes:
        culprit_output.append(
            subprocess.check_output(
                ['git', '-C', '../', 'log', '-1',
                 culprit_hash]).decode().strip())

    if tags:
        if commit_message[0] == '[':
            # If the commit message already starts with a tag, do not add a
            # space in between them.
            commit_message = f'{"".join(tags)}{commit_message}'
        else:
            commit_message = f'{"".join(tags)} {commit_message}'

    issues = os.getenv("issue")
    if issues:
        issues = issues.split(',')
    else:
        issues = []

    # Extract issue numbers from the branch name
    issue_pattern = r"issue-(\d+)"
    branch_issues = re.findall(issue_pattern, branch_name)
    issues.extend(branch_issues)

    # Remove duplicates
    issues = list(set(issues))

    if issues:
        commit_message += '\n\n' + '\n'.join([
            f"Resolves https://github.com/brave/brave-browser/issues/{issue}"
            for issue in issues
            if f'brave-browser/issues/{issue}' not in commit_message
        ])

    if culprit_output:
        chromium_links = [
            f"https://chromium.googlesource.com/chromium/src/+/{culprit_hash}"
            for culprit_hash in culprit_hashes
        ]
        commit_message += ('\n\nChromium changes:'
                           '\n{chromium_links}\n\n{culprits}'.format(
                               chromium_links="\n".join(chromium_links),
                               culprits="\n\n".join(culprit_output)))

    with open(commit_msg_file, 'w') as f:
        f.write(commit_message)

    return 0


if __name__ == "__main__":
    sys.exit(main())
