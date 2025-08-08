# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import sys
import os
import subprocess
import tempfile
import json
import re

NALA_REPO = 'https://github.com/brave/leo.git'
nala_clone_dir = tempfile.mkdtemp()

class Commit:
  def __init__(self, log_line):
    parts = log_line.split(' ', 1)
    self.sha = parts[0]
    self.message = parts[1]

  def __str__(self):
    # Update the PR links to point to brave/leo
    return re.sub(r'\((#(\d+))\)$', r'([\1](https://github.com/brave/leo/pull/\2))', self.message)

  def __repr__(self):
    return self.__str__()

def parse_args():
    parser = argparse.ArgumentParser(
        description='Bumps Nala to a new version')
    parser.add_argument('--sha',
                        help='The version to bump to',
                        default="main")
    parser.add_argument('--push',
                        help='Push the changes to the remote repository',
                        action='store_true',
                        default=False)
    return parser.parse_args()

def clone_nala():
  subprocess.run(['git', 'clone', NALA_REPO, nala_clone_dir], check=True)

def get_full_sha(sha):
  cwd = os.getcwd()
  try:
    os.chdir(nala_clone_dir)
    return subprocess.run(['git', 'rev-parse', sha], check=True, capture_output=True).stdout.decode('utf-8').strip()
  finally:
    os.chdir(cwd)

def get_nala_current_sha():
  with open('package.json', 'r') as f:
    package_json = json.load(f)
  return package_json['dependencies']['@brave/leo'].split('#')[1]

def get_changes(old_sha, new_sha):
  cwd = os.getcwd()
  try:
    os.chdir(nala_clone_dir)
    result = subprocess.run(['git', 'log', '--oneline', '--no-decorate', f'{old_sha}...{new_sha}'], check=True, capture_output=True)
    commits = result.stdout.decode('utf-8').split('\n')
    return [Commit(commit) for commit in commits if commit]
  finally:
    os.chdir(cwd)

def bump_nala(sha):
  old_sha = get_nala_current_sha()

  clone_nala()

  new_sha = get_full_sha(sha)

  if old_sha == new_sha:
    print(f"Nala is already at {sha}. Nothing to do.")
    return

  changes = get_changes(old_sha, new_sha)
  commit_message = f"""[Nala]: Bump version to {sha}
  
Changes https://github.com/brave/leo/compare/{old_sha}...{new_sha}
{"\n".join(map(lambda x: f'- {x}', changes))}"""

  subprocess.run(['npm', 'install', f'github:brave/leo#{new_sha}'], check=True)
  subprocess.run(['git', 'commit', 'package.json', 'package-lock.json', '-m', commit_message], check=True)

def main():
  args = parse_args()
  bump_nala(args.sha)

  if args.push:
    subprocess.run(['git', 'push'], check=True)

if __name__ == '__main__':
  main()
