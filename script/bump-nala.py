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
import shutil
import subprocess
import tempfile
import json
import re

REPO_ID = 'brave/leo'
NALA_REPO = f'https://github.com/{REPO_ID}'
nala_clone_dir = tempfile.mkdtemp()

SF_SYMBOLS_REPO_ID = 'brave/leo-sf-symbols'
SF_SYMBOLS_REPO = f'https://github.com/{SF_SYMBOLS_REPO_ID}'
sf_symbols_clone_dir = tempfile.mkdtemp()

PNPM_WORKSPACE = 'pnpm-workspace.yaml'


class Commit:
    """
    Represents a commit in a repository. This also handles updating links based
    on Github PR numbers to a full link so they work from the brave-core repo.
    """

    def __init__(self, log_line, repo_url=NALA_REPO):
        parts = log_line.split(' ', 1)
        self.sha = parts[0]
        self.message = parts[1]
        self.repo_url = repo_url

    def __str__(self):
        # Update the PR links to point to the repo
        return re.sub(r'\((#(\d+))\)$', rf'([\1]({self.repo_url}/pull/\2))',
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


def clone_repo(url, clone_dir):
    """Clones a repository to the given directory."""
    subprocess.run(
        ['git', 'clone', url + '.git', clone_dir, '--filter', 'blob:none'],
        check=True)


def get_full_sha(target, clone_dir):
    """Gets the full sha of a commit, tag or branch in the given clone."""
    cwd = os.getcwd()
    try:
        os.chdir(clone_dir)
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


def get_sf_symbols_current_sha():
    """Gets the current sha of leo-sf-symbols from the package.json file."""
    with open('package.json', 'r') as f:
        package_json = json.load(f)
    return package_json['dependencies']['@brave/leo-sf-symbols'].split('#')[1]


def detect_package_manager():
    """Detects whether npm or pnpm is used based on the lockfile present."""
    has_npm_lock = os.path.isfile('package-lock.json')
    has_pnpm_lock = os.path.isfile('pnpm-lock.yaml')
    if has_npm_lock and has_pnpm_lock:
        sys.exit('Both package-lock.json and pnpm-lock.yaml exist.')
    if has_pnpm_lock:
        return 'pnpm'
    if has_npm_lock:
        return 'npm'
    sys.exit('No package-lock.json or pnpm-lock.yaml found.')


def get_package_manager_cmd(package_manager):
    """Returns the path to the package manager executable."""
    cmd = shutil.which(package_manager)
    if not cmd:
        sys.exit(f'{package_manager} not found in PATH.')
    return cmd


def to_npm_packages(*package_refs):
    """Returns a list of npm package strings for the given packages."""
    return [f'github:{repo_id}#{sha}' for repo_id, sha in package_refs]


def update_pnpm_allow_builds(*package_refs):
    """Rewrites allowBuilds SHAs for known packages in pnpm-workspace.yaml.

    pnpm's --allow-build flag replaces the entire allowBuilds list, so we update
    the two known entries in place before running pnpm add.
    """
    with open(PNPM_WORKSPACE, 'r', encoding='utf-8') as f:
        content = f.read()

    for repo_id, sha in package_refs:
        pattern = (rf"('@{re.escape(repo_id)}"
                   rf"@https://codeload\.github\.com/{re.escape(repo_id)}"
                   rf"/tar\.gz/)[0-9a-f]+(')")
        content, count = re.subn(pattern, rf'\g<1>{sha}\g<2>', content)
        if count != 1:
            sys.exit(f'Expected exactly one allowBuilds entry for {repo_id} '
                     f'in {PNPM_WORKSPACE}, found {count}.')

    with open(PNPM_WORKSPACE, 'w', encoding='utf-8', newline='\n') as f:
        f.write(content)


def install_dependencies(package_manager, new_sha, new_sf_symbols_sha):
    """Installs bumped Nala dependencies with npm or pnpm."""
    cmd = get_package_manager_cmd(package_manager)
    package_refs = [(REPO_ID, new_sha),
                    (SF_SYMBOLS_REPO_ID, new_sf_symbols_sha)]
    packages = to_npm_packages(*package_refs)
    if package_manager == 'pnpm':
        update_pnpm_allow_builds(*package_refs)
        subprocess.run([cmd, 'add', *packages], check=True)
    else:
        subprocess.run([cmd, 'install', *packages], check=True)


def get_commit_files(package_manager):
    """Returns the files that should be committed for the package manager."""
    files = ['package.json']
    if package_manager == 'pnpm':
        files.extend(['pnpm-lock.yaml', PNPM_WORKSPACE])
    else:
        files.append('package-lock.json')
    return files


def get_changes(old_sha, new_sha, clone_dir, repo_url=NALA_REPO):
    """
    Gets the commits between two shas in a repository.
    Returns a list of Commit objects.
    """
    cwd = os.getcwd()
    try:
        os.chdir(clone_dir)
        result = subprocess.run([
            'git', 'log', '--oneline', '--no-decorate',
            f'{old_sha}...{new_sha}'
        ],
                                check=True,
                                capture_output=True)
        commits = result.stdout.decode('utf-8').split('\n')
        return [Commit(commit, repo_url) for commit in commits if commit]
    finally:
        os.chdir(cwd)


def bump_nala(target):
    """
    Bumps Nala to a new version and commits the changes (with a commit message
    listing all the changes that landed in Nala since the last bump). Also
    installs the latest leo-sf-symbols commit.
    """
    old_sha = get_nala_current_sha()

    clone_repo(NALA_REPO, nala_clone_dir)
    clone_repo(SF_SYMBOLS_REPO, sf_symbols_clone_dir)

    new_sha = get_full_sha(target, nala_clone_dir)
    new_sf_symbols_sha = get_full_sha('main', sf_symbols_clone_dir)

    if old_sha == new_sha:
        print(f"Nala is already at {target}. Nothing to do.")
        return

    changes = get_changes(old_sha, new_sha, nala_clone_dir)
    changes_text = "\n".join(map(lambda x: f'- {x}', changes))
    commit_message = f"""[Nala]: Bump version to {target}

Changes {NALA_REPO}/compare/{old_sha}...{new_sha}
{changes_text}"""

    package_manager = detect_package_manager()
    install_dependencies(package_manager, new_sha, new_sf_symbols_sha)
    subprocess.run([
        'git', 'commit', *get_commit_files(package_manager), '-m',
        commit_message
    ],
                   check=True)


def main():
    args = parse_args()
    bump_nala(args.target)

    if args.push:
        subprocess.run(['git', 'push'], check=True)


if __name__ == '__main__':
    main()
