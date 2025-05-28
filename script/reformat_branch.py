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


@atexit.register
def cleanup() -> None:
    global TEMP_DIR
    if TEMP_DIR:
        shutil.rmtree(TEMP_DIR, ignore_errors=True)


def run_git_command(cmd: List[str]) -> str:
    result = subprocess.run(['git'] + cmd,
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
    result = subprocess.run(['git', 'cherry-pick', '--allow-empty', commit],
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
        'npx', f'prettier@{get_prettier_version()}', '--config', config,
        '--ignore-unknown', '--ignore-path', ignore, '--write', file
    ]
    prettier_result = subprocess.run(args,
                                     capture_output=True,
                                     text=True,
                                     check=False)
    if prettier_result.returncode != 0:
        raise Exception(
            f'Failed to format file {file}: {prettier_result.stderr}')


def format_files(files: List[str], stage=False, commit=False) -> None:
    for file in files:
        if not os.path.isfile(file) or not is_supported_file(file):
            continue
        format_file(file)
        if stage:
            run_git_command(['add', file])
    if commit and is_dirty():
        run_git_command(
            ['commit', '-m', '[autoformat] Pre-format changed files'])


def reformat_commits(start: str, end: str) -> None:
    '''Reformat all commits in the given range.'''
    all_touched_files = get_changed_files(f'{start}..{end}')

    format_files(all_touched_files, stage=True, commit=True)

    commits = run_git_command(['rev-list', '--reverse',
                               f'{start}..{end}']).splitlines()
    for commit in commits:
        print(f'Reformatting commit: {commit}')
        changed_files = get_changed_files(f'{commit}^..{commit}')

        if git_cherry_pick(commit):
            format_files(changed_files, stage=True)
            run_git_command(['commit', '--amend', '--no-edit'])
        else:
            for file in changed_files:
                # git checkout <commit> -- <file> will:
                # - Update file to version in commit (if file exists in commit)
                # - Delete file  (if file doesn't exist in commit)
                # Both cases are correct.
                run_git_command(['checkout', commit, '--', file])

            format_files(changed_files, stage=True)
            run_git_command(['cherry-pick', '--continue', '--no-edit'])


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

    merge_base = run_git_command(['merge-base', base, target_ref])
    print(f'Merge base is: {merge_base}')

    run_git_command(['checkout', merge_base])

    temp_branch = f'{target_branch}-reformat'
    run_git_command(['checkout', '-B', temp_branch])

    try:
        reformat_commits(merge_base, target_ref)
        print(f'Formatting complete. You are now on branch {temp_branch}.')
        print(f'You may want to rebase this branch onto {base}')
    except Exception as e:
        print(f'Error during reformatting: {e}', file=sys.stderr)
        print(f'Restoring original branch: {original_branch}', file=sys.stderr)
        if original_branch:
            run_git_command(['checkout', original_branch])
        raise


if __name__ == '__main__':
    main()
