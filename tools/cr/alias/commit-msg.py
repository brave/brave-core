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
# 4. `culprit` environment variable for upstream commit links
#
#    If the culprit environment variable is set, it will be used to add links to
#    the commit message. The culprit variable should be a comma-separated list
#    of commit hashes. The script will look up the commit messages for each hash
#    and add them to the commit message, with a link to each commit built from
#    its repository's `origin` push URL: on googlesource,
#    https://chromium.googlesource.com/chromium/src/+/{hash}, and on GitHub,
#    https://github.com/{owner}/{repo}/commit/{hash}.
#
#    A culprit may also target a subrepo (e.g. v8, that lives outside of the
#    Chromium checkout) using the `<subrepo-path>:<hash>` syntax. For instance
#    `culprit=v8/src:3ba47f111` looks up the commit in `../v8/src` and links to
#    it through that checkout's remote (e.g.
#    https://chromium.googlesource.com/v8/v8/+/{hash}). A bare hash resolves
#    against the Chromium checkout at `../`.
#
#    Each culprit is written as its full hash, however short the one given, and
#    one that isn't a commit stops the commit. On a `cr<N>` branch, a Chromium
#    culprit must also be in the branch's range: after the Chromium tag its
#    upstream's `package.json` pins, up to the one HEAD's pins.
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

import json
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

# A googlesource remote (`https://<host>.googlesource.com/[a/]<project>[.git]`),
# whose commits browse at `/<project>/+/<hash>`.
_GOOGLESOURCE_REMOTE = re.compile(
    r'https://([\w.-]+\.googlesource\.com)/(?:a/)?(.+?)(?:\.git)?/?'
)
# A GitHub remote, over https or ssh, whose commits browse at
# `/<owner>/<repo>/commit/<hash>`.
_GITHUB_REMOTE = re.compile(
    r'(?:https://|ssh://git@|git@)github\.com[:/](.+?)(?:\.git)?/?'
)


def parse_culprit(culprit: str) -> tuple[str, str]:
    """Splits a culprit value into its subrepo path and commit hash.

    A bare hash (e.g. `abc123`) targets the Chromium checkout, returning a
    subrepo of ''. A `<subrepo-path>:<hash>` value (e.g. `v8/src:abc123`)
    targets a repo that lives outside the Chromium checkout, at
    `../<subrepo-path>`.
    """
    repo, separator, commit_hash = culprit.partition(':')
    if separator:
        return repo, commit_hash
    return '', repo


def _git(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        ['git', *args], capture_output=True, text=True, check=False
    )


def commit_url_base(git_dir: str) -> str | None:
    """The URL a commit hash completes, from `git_dir`'s origin push URL.

    The push URL rather than the fetch one, as a checkout fetching from a git
    cache mirror still pushes to the real remote. None for a remote that is
    neither on googlesource nor on GitHub.
    """
    result = _git('-C', git_dir, 'remote', 'get-url', '--push', 'origin')
    url = result.stdout.strip()
    if match := _GOOGLESOURCE_REMOTE.fullmatch(url):
        host, project = match.groups()
        return f'https://{host}/{project}/+/'
    if match := _GITHUB_REMOTE.fullmatch(url):
        return f'https://github.com/{match.group(1)}/commit/'
    return None


def resolve_commit(git_dir: str, commit_hash: str) -> str | None:
    """The full hash of `commit_hash` in `git_dir`, or None if it isn't one."""
    result = _git(
        '-C',
        git_dir,
        'rev-parse',
        '--verify',
        '--quiet',
        f'{commit_hash}^{{commit}}',
    )
    return result.stdout.strip() if result.returncode == 0 else None


def _chromium_tag(ref: str) -> str | None:
    """The Chromium tag brave-core's `package.json` pins at `ref`."""
    result = _git('show', f'{ref}:package.json')
    if result.returncode:
        return None
    try:
        return json.loads(result.stdout)['config']['projects']['chrome']['tag']
    except (ValueError, KeyError, TypeError):
        return None


def culprit_range_problem(git_dir: str, full_hash: str) -> str | None:
    """Says why a Chromium culprit is outside the branch's range, if it is.

    On a `cr<N>` branch, an upstream culprit is a commit between the
    Chromium tag its upstream (the base) pins, and the one HEAD pins. The
    check is skipped, with a warning, when those tags can't be resolved.
    """
    upstream = _git('rev-parse', '--abbrev-ref', '@{upstream}')
    if upstream.returncode:
        return None
    old, new = _chromium_tag(upstream.stdout.strip()), _chromium_tag('HEAD')
    if not old or not new or old == new:
        return None
    for tag in (old, new):
        if resolve_commit(git_dir, f'refs/tags/{tag}') is None:
            print(
                f"Warning: tag {tag} isn't in {git_dir}, so the culprit's "
                'range is not checked.',
                file=sys.stderr,
            )
            return None

    def is_ancestor(ancestor: str, tag: str) -> bool:
        return not _git(
            '-C',
            git_dir,
            'merge-base',
            '--is-ancestor',
            ancestor,
            f'refs/tags/{tag}',
        ).returncode

    if is_ancestor(full_hash, new) and not is_ancestor(full_hash, old):
        return None
    return (
        f'Culprit {full_hash} is not in the branch\'s Chromium range, '
        f'{old}..{new}. Commit without --culprit if no upstream change is to '
        'blame.'
    )


def main():
    if (
        os.path.exists('.git/rebase-merge')
        or os.path.exists('.git/rebase-apply')
        or os.path.exists('.git/CHERRY_PICK_HEAD')
    ):
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
        'amend!'
    ):
        return 0

    first_line = commit_message.split('\n', 1)[0]

    # Getting the branch name to see if there's any tags that can be deduced
    # from it.
    branch_name = (
        subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])
        .strip()
        .decode()
    )

    tags = set()
    # Checking for chromium upgrade branches
    cr_branch_pattern = r"^cr\d+$"
    if re.fullmatch(cr_branch_pattern, branch_name):
        # The branch tag is excluded from the upgrade commit messages.
        if not any(
            suffix in first_line for suffix in PREFIXES_FOR_UPGRADE_COMMITS
        ):
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
    for culprit in filter(None, (os.getenv("culprit") or '').split(',')):
        repo, commit_hash = parse_culprit(culprit)
        git_dir = os.path.join('..', repo)
        if _git('-C', git_dir, 'rev-parse', '--git-dir').returncode:
            print(
                f"Culprit subrepo '{repo}' is not a git checkout at {git_dir}.",
                file=sys.stderr,
            )
            return 1
        url_base = commit_url_base(git_dir)
        if url_base is None:
            print(
                f"Can't link to commits in {git_dir}: its origin push URL is "
                'neither on googlesource nor on GitHub.',
                file=sys.stderr,
            )
            return 1
        # Links and later checks need the full hash, whatever was passed.
        full_hash = resolve_commit(git_dir, commit_hash)
        if full_hash is None:
            print(
                f"Culprit '{commit_hash}' is not a commit in {git_dir}.",
                file=sys.stderr,
            )
            return 1
        if not repo and re.fullmatch(cr_branch_pattern, branch_name):
            if problem := culprit_range_problem(git_dir, full_hash):
                print(problem, file=sys.stderr)
                return 1
        culprits.append((git_dir, full_hash, url_base))
    # Skip culprits already in the commit message, even as a short hash.
    culprits = [
        culprit for culprit in culprits if culprit[1][:7] not in commit_message
    ]

    culprit_output = []
    culprit_links = []
    for git_dir, commit_hash, url_base in culprits:
        culprit_output.append(
            subprocess.check_output(
                ['git', '-C', git_dir, 'log', '-1', commit_hash]
            )
            .decode()
            .strip()
        )
        culprit_links.append(f'{url_base}{commit_hash}')

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
        commit_message += '\n\n' + '\n'.join(
            [
                f"Resolves https://github.com/brave/brave-browser/issues/{issue}"
                for issue in issues
                if f'brave-browser/issues/{issue}' not in commit_message
            ]
        )

    if culprit_output:
        commit_message += (
            '\n\nChromium changes:\n{chromium_links}\n\n{culprits}'.format(
                chromium_links="\n".join(culprit_links),
                culprits="\n\n".join(culprit_output),
            )
        )

    with commit_msg_file.open('w', encoding='utf-8', newline='') as f:
        f.write(commit_message)

    return 0


if __name__ == "__main__":
    sys.exit(main())
