#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Reformat Branch Script

The script reformats the target branch using prettier.
It preserves the original commit structure and adds an extra commit
to format the changed files in the branch if it's necessary.

The scheme:
- Checkout the merge base.
- Format all the files changed in <merge_base>..<target_branch> and commit.
- Cherry-pick each commit and format the changed files in the commit.
- The conflicts are resolved by checking out the target file and
  applying formatting.

Usage: python3 reformat_branch.py [--target BRANCH] [--base BRANCH]
"""

import argparse
from functools import lru_cache
import shutil
import subprocess
import os
import sys
import json
import atexit
import tempfile
from typing import List, Optional

DEFAULT_BRANCH = 'origin/master'
TEMP_DIR: Optional[str] = None

GIT = 'git.bat' if sys.platform == 'win32' else 'git'
NPX = 'npx.cmd' if sys.platform == 'win32' else 'npx'


@atexit.register
def cleanup() -> None:
    global TEMP_DIR
    if TEMP_DIR:
        shutil.rmtree(TEMP_DIR, ignore_errors=True)


def run_git_command(cmd: List[str]) -> str:
    result = subprocess.run([GIT] + cmd,
                            capture_output=True,
                            text=True,
                            check=True)
    return result.stdout.strip()


@lru_cache
def load_file_from_git(file: str, ref=DEFAULT_BRANCH) -> str:
    global TEMP_DIR
    if TEMP_DIR is None:
        TEMP_DIR = tempfile.mkdtemp()
    filepath = os.path.join(TEMP_DIR, file)
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with open(filepath, 'w') as f:
        f.write(run_git_command(['show', f'{ref}:{file}']))
    return filepath


def git_cherry_pick(commit: str) -> bool:
    result = subprocess.run([GIT, 'cherry-pick', '--allow-empty', commit],
                            capture_output=True,
                            text=True,
                            check=False)
    if result.returncode > 1:
        # returncode 1 is conflict, >1 is actual error
        raise Exception(
            f'Failed to cherry-pick commit {commit}: {result.stderr}')
    return result.returncode == 0


def get_changed_files(commit_range: str) -> List[str]:
    '''Get list of files changed in the given commit range.'''
    num_commits = len(run_git_command(['rev-list', commit_range]).splitlines())
    if num_commits > 1000:
        raise Exception(f'Bad commit range: {commit_range} (too many commits)')

    return run_git_command(['diff', '--name-only', commit_range]).splitlines()


@lru_cache
def get_prettier_version() -> str:
    package_json = load_file_from_git('package.json')
    with open(package_json, 'r') as f:
        package_json_content = json.load(f)
    return package_json_content['devDependencies']['prettier']


def is_dirty() -> bool:
    return run_git_command(['status', '--porcelain',
                            '--untracked-files=no']).strip() != ''


def is_supported_file(file: str) -> bool:
    # Files that should NOT be formatted by prettier
    excluded_exts = ['.cc', '.h', '.java', '.py', '.gn', '.gni', '.mm', '.rs']
    return not any(file.endswith(ext) for ext in excluded_exts)


def format_file(file: str) -> None:
    config = load_file_from_git('.prettierrc.js')
    ignore = load_file_from_git('.prettierignore')

    args = [
        NPX, '-y', f'prettier@{get_prettier_version()}', '--config', config,
        '--ignore-unknown', '--ignore-path', ignore, '--write', file
    ]
    prettier_result = subprocess.run(args,
                                     capture_output=True,
                                     text=True,
                                     check=False)
    if prettier_result.returncode != 0:
        raise Exception(
            f'Failed to format file {file}: {prettier_result.stderr}')


def format_files(files: List[str], stage=False, commit=False) -> Optional[str]:
    for file in files:
        if not os.path.isfile(file) or not is_supported_file(file):
            continue
        format_file(file)
        if stage:
            run_git_command(['add', file])
    if commit and is_dirty():
        run_git_command(
            ['commit', '-m', '[autoformat] Pre-format changed files'])
        return run_git_command(['rev-parse', 'HEAD']).strip()
    return None


def format_all_touched_files(start_ref: str, end_ref: str) -> Optional[str]:
    all_touched_files = get_changed_files(f'{start_ref}..{end_ref}')
    print(f'Preformatting {len(all_touched_files)} files...')
    return format_files(all_touched_files, stage=True, commit=True)


def reformat_and_cherry_pick_commits(start: str, end: str) -> None:
    '''Reformat all commits in the given range.'''

    commits = run_git_command(['rev-list', '--reverse',
                               f'{start}..{end}']).splitlines()
    for commit in commits:
        changed_files = get_changed_files(f'{commit}~1..{commit}')
        print(f'Reformatting commit: {commit} ({len(changed_files)} files)')

        if git_cherry_pick(commit):
            format_files(changed_files, stage=True)
            run_git_command(['commit', '--amend', '--no-edit'])
        else:
            for file in changed_files:
                try:
                    run_git_command(['checkout', commit, '--', file])
                except:
                    # the file was deleted in the commit, remove it
                    run_git_command(['rm', file])

            format_files(changed_files, stage=True)
            run_git_command(['cherry-pick', '--continue', '--no-edit'])


def reformat_branch(target_ref: str, base=DEFAULT_BRANCH) -> Optional[str]:
    merge_base = run_git_command(['merge-base', base, target_ref])
    print(f'Merge base is: {merge_base}')
    run_git_command(['checkout', merge_base])
    pre_format_commit = format_all_touched_files(merge_base, target_ref)
    reformat_and_cherry_pick_commits('HEAD', target_ref)
    return pre_format_commit


def main() -> None:
    parser = argparse.ArgumentParser(
        description='Reformat files in the current branch.')
    parser.add_argument(
        '--target',
        default=None,
        help='Target branch to reformat (default: current branch)')
    parser.add_argument(
        '--base',
        default=DEFAULT_BRANCH,
        help=f'Base branch to compare against (default: {DEFAULT_BRANCH})')
    args = parser.parse_args()

    if is_dirty():
        raise Exception(
            'Current branch is dirty, please commit or stash your changes')

    # Store original branch for cleanup
    original_branch = run_git_command(['rev-parse', '--abbrev-ref', 'HEAD'])

    target_branch = args.target or original_branch
    target_ref = run_git_command(['rev-parse', target_branch])
    base = args.base

    try:
        reformat_branch(target_ref, base)
        temp_branch = f'{target_branch}-reformatted'
        run_git_command(['checkout', '-B', temp_branch])
        print(f'Formatting complete. You are now on branch {temp_branch}.')
        print(f'You may want to rebase this branch onto {base}')
    except Exception as e:
        print(f'Error during reformatting: {e}', file=sys.stderr)
        if original_branch:
            print(f'Restoring the original branch: {original_branch}..')
            if input(
                    f'Call git checkout -f {original_branch}? (y/n): ') == 'y':
                run_git_command(['checkout', '-f', original_branch])
        raise


if __name__ == '__main__':
    main()
