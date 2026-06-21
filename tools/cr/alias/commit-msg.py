#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Notice: Keep this script as *standalone*, with no deps to `tools/cr` code, so
# users can install it with little effort and no deps surprises.

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
#    A culprit may also target a subrepo (e.g. v8, that lives outside of the
#    Chromium checkout) using the `<subrepo-path>:<hash>` syntax. For instance
#    `culprit=v8/src:3ba47f111` looks up the commit in `../v8/src` and links to
#    that subrepo's googlesource URL (e.g.
#    https://chromium.googlesource.com/v8/v8/+/{hash}). A bare hash resolves
#    against the Chromium checkout at `../`.
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
from pathlib import Path

PREFIXES_FOR_UPGRADE_COMMITS = [
    'Update from Chromium ',
    'Apply-fixed 🩹 patches from Chromium ',
    'Conflict-resolved patches from Chromium ',
    'Update patches from Chromium ',
    'Updated strings for Chromium ',
]

# Known googlesource browse URLs keyed by the culprit's subrepo path (relative
# to the Chromium checkout). A bare hash with no subrepo prefix maps to ''.
# Add new subrepos here as needed.
GOOGLESOURCE_URLS = {
    '': 'https://chromium.googlesource.com/chromium/src',
    'v8/src': 'https://chromium.googlesource.com/v8/v8',
}


def parse_culprit(culprit: str) -> tuple[str, str]:
    """Splits a culprit value into its subrepo path and commit hash.

    A bare hash (e.g. `abc123`) targets the Chromium checkout, returning a
    subrepo of ''. A `<subrepo-path>:<hash>` value (e.g. `v8/src:abc123`)
    targets a repo that lives outside the Chromium checkout, keyed in
    GOOGLESOURCE_URLS.
    """
    repo, separator, commit_hash = culprit.partition(':')
    if separator:
        return repo, commit_hash
    return '', repo


def main():
    if (os.path.exists('.git/rebase-merge')
            or os.path.exists('.git/rebase-apply')
            or os.path.exists('.git/CHERRY_PICK_HEAD')):
        # There should be no changes to commit messages during a rebase or
        # cherry-pick.
        return 0

    commit_msg_file = Path(sys.argv[1])

    # Read the message provided so we can check for duplicate insertions
    # beforehand.
    commit_message = commit_msg_file.read_bytes().decode('utf-8')

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
    culprits = []
    if os.getenv("culprit"):
        culprits = [
            parse_culprit(culprit)
            for culprit in os.getenv("culprit").split(',')
        ]
    # Skip culprits whose hash is already present in the commit message.
    culprits = [(repo, commit_hash) for repo, commit_hash in culprits
                if commit_hash not in commit_message]

    culprit_output = []
    culprit_links = []
    for repo, commit_hash in culprits:
        if repo not in GOOGLESOURCE_URLS:
            known = ', '.join(sorted(r for r in GOOGLESOURCE_URLS if r))
            print(
                f"Unknown culprit subrepo '{repo}'. Known subrepos: {known}.",
                file=sys.stderr)
            return 1
        git_dir = os.path.join('..', repo)
        culprit_output.append(
            subprocess.check_output(
                ['git', '-C', git_dir, 'log', '-1',
                 commit_hash]).decode().strip())
        culprit_links.append(f"{GOOGLESOURCE_URLS[repo]}/+/{commit_hash}")

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
        commit_message += ('\n\nChromium changes:'
                           '\n{chromium_links}\n\n{culprits}'.format(
                               chromium_links="\n".join(culprit_links),
                               culprits="\n\n".join(culprit_output)))

    with commit_msg_file.open('w', encoding='utf-8', newline='') as f:
        f.write(commit_message)

    return 0


if __name__ == "__main__":
    sys.exit(main())
