# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
This script bumps Nala to a new version (or the latest version on the main
branch, if none is provided).

It commits the bumped version and lists all the changes in the commit message,
and links to the commits in Nala, to make it obvious what's new.
"""

import argparse
import sys
import os
import subprocess
import tempfile
import json
import re

REPO_ID = 'brave/leo'
NALA_REPO = f'https://github.com/{REPO_ID}'
nala_clone_dir = tempfile.mkdtemp()


class Commit:
    """
    Represents a commit in the Nala repository. This also handles updating
    links based on Github PR numbers to a full link so they work from the
    brave-core repo.
    """

    def __init__(self, log_line):
        parts = log_line.split(' ', 1)
        self.sha = parts[0]
        self.message = parts[1]

    def __str__(self):
        # Update the PR links to point to the repo
        return re.sub(r'\((#(\d+))\)$', rf'([\1]({NALA_REPO}/pull/\2))',
                      self.message)

    def __repr__(self):
        return self.__str__()


def parse_args():
    parser = argparse.ArgumentParser(description='Bumps Nala to a new version')
    parser.add_argument(
        '--target',
        help='The version to bump to. Can be a commit, tag or branch.',
        default="main")
    parser.add_argument('--push',
                        help='Push the changes to the remote repository',
                        action='store_true',
                        default=False)
    return parser.parse_args()


def clone_nala():
    """Clones the Nala repository to a temporary directory."""
    subprocess.run([
        'git', 'clone', NALA_REPO + '.git', nala_clone_dir, '--filter',
        'blob:none'
    ],
                   check=True)


def get_full_sha(target):
    """Gets the full sha of a commit, tag or branch."""
    cwd = os.getcwd()
    try:
        os.chdir(nala_clone_dir)
        return subprocess.run(
            ['git', 'rev-parse', target], check=True,
            capture_output=True).stdout.decode('utf-8').strip()
    finally:
        os.chdir(cwd)


def get_nala_current_sha():
    """Gets the current sha of Nala from the package.json file."""
    with open('package.json', 'r') as f:
        package_json = json.load(f)
    return package_json['dependencies']['@brave/leo'].split('#')[1]


def get_changes(old_sha, new_sha):
    """
    Gets the commits between two shas in the Nala repository.
    Returns a list of Commit objects.
    """
    cwd = os.getcwd()
    try:
        os.chdir(nala_clone_dir)
        result = subprocess.run([
            'git', 'log', '--oneline', '--no-decorate',
            f'{old_sha}...{new_sha}'
        ],
                                check=True,
                                capture_output=True)
        commits = result.stdout.decode('utf-8').split('\n')
        return [Commit(commit) for commit in commits if commit]
    finally:
        os.chdir(cwd)


def bump_nala(target):
    """
    Bumps Nala to a new version and commits the changes (with a commit message
    listing all the changes that landed in Nala since the last bump).
    """
    old_sha = get_nala_current_sha()

    clone_nala()

    new_sha = get_full_sha(target)

    if old_sha == new_sha:
        print(f"Nala is already at {target}. Nothing to do.")
        return

    changes = get_changes(old_sha, new_sha)
    changes_text = "\n".join(map(lambda x: f'- {x}', changes))
    commit_message = f"""[Nala]: Bump version to {target}

Changes {NALA_REPO}/compare/{old_sha}...{new_sha}
{changes_text}"""

    subprocess.run(['pnpm', 'add', f'github:{REPO_ID}#{new_sha}'], check=True)
    subprocess.run([
        'git', 'commit', 'package.json', 'pnpm-lock.yaml', '-m', commit_message
    ],
                   check=True)


def main():
    args = parse_args()
    bump_nala(args.target)

    if args.push:
        subprocess.run(['git', 'push'], check=True)


if __name__ == '__main__':
    main()
