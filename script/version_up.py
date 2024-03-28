#!/usr/bin/env python
# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import json
import re
import subprocess
import sys


def _run_git(*cmd):
  """Runs a git command on the current repository.

  This function returs a proper utf8 string in success, otherwise it allows the
  exception thrown by subprocess through.

  e.g:
    _run_git('add', '-u', '*.patch')
  """
    cmd = ['git'] + list(cmd)
    return subprocess.check_output(cmd).strip().decode('utf-8')


def _load_package_file(branch):
  """Retrieves the json content of package.json for a given revision

  Args:
    branch:
      A branch or hash to load the file from.
  """
    package = _run_git('show', f'{branch}:package.json')
    return json.loads(package)


def _run_npm_command(*cmd):
  """Runs an npm build command.

  This function will run 'npm run' commands appended by any extra arguments are
  passed into it.

  e.g:
    _run_npm_command('init')
  """
    cmd = ['npm', 'run'] + list(cmd)
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, bufsize=-1)
    while True:
        line = process.stdout.readline()
        if not line: break
        print(line.decode('utf-8').rstrip())

    process.wait()
    return process.returncode == 0


def _get_chromium_version_from_git(branch):
  """Retrieves the chromium tag in package.json

  This function will load the package.json file from a given branch, and
  retrieve the tag chromium version value in it.

  Args:
    branch:
      A branch or hash to load the file from.

  e.g:
    _get_chromium_version_from_git('HEAD')
  """
    return _load_package_file(branch).get('config').get('projects').get(
        'chrome').get('tag')


def _is_upgrading(target_version, working_version):
  """Checks whether a target version is higher than the working version

  This function is used to prevent a downgrade attempt, or re-applying an
  upgrade, by informing whether or not the target version is higher.

  Args:
    target_version:
      The version being upgraded to.
    working_version:
      The current version of the current branch.
  """
    to_version = [eval(i) for i in target_version.split('.')]
    in_version = [eval(i) for i in working_version.split('.')]

    if len(in_version) != 4 or len(to_version) != 4:
        return False

    return (to_version[0] > in_version[0] or to_version[1] > in_version[1]
            or to_version[2] > in_version[2] or to_version[3] > in_version[3])


class Upgrade:
    """The upgrade process, holding the data related to the upgrade.

    This class produces an object that is reponsible for keeping track of the
    upgrade process step-by-step. It acquires all the common data naecessary for
    its completion.
    """
    def __init__(self, base_branch, target_version):
        self.base_branch = base_branch
        self.target_version = target_version
        self.base_version = _get_chromium_version_from_git(base_branch)
        self.working_version = _get_chromium_version_from_git('HEAD')

    def _update_package_version(self):
        """Creates the change upgrading the chromium version

        This is for the creation of the first commit, which means updating
        package.json to the target version provided, and commiting the change to
        the repo
        """
        package = _load_package_file('HEAD')
        package['config']['projects']['chrome']['tag'] = self.target_version
        with open('package.json', "w") as package_file:
            json.dump(package, package_file, indent=2)

        _run_git('add', 'package.json')
        _run_git(
            'commit', '-m',
            f'"Update from Chromium {self.base_version} to Chromium {self.target_version}."'
        )

        commit = _run_git('log', '-1', '--pretty=oneline', '--abbrev-commit')
        print(f'Done: {commit}')

    def _save_updated_patches(self):
        """Creates the updated patches change

        This function creates the third commit in the order of the update, saving
        all patches that might have been changed or deleted. Untracked patches are
        excluded from addition at this stage.
        """
        _run_git('add', '-u', '*.patch')

        if _run_git('diff', '--cached', '--stat') == '':
            # Nothing to commit
            return
        _run_git(
            'commit', '-m',
            f'"Update patches from Chromium {self.base_version} to Chromium {self.target_version}."'
        )

        commit = _run_git('log', '-1', '--pretty=oneline', '--abbrev-commit')
        print(f'Done: {commit}')

    def _save_rebased_l10n(self):
        """Creates string rebase change

        This function stages, and commits, all changed, updated, or deleted files
        resulting from running npm run chromium_rebase_l10n.
        """
        _run_git('add', '*.grd', '*.grdp', '*.xtb')
        if _run_git('diff', '--cached', '--stat') == '':
            # Nothing to commit
            return

        _run_git('commit', '-m',
                 f'"Updated strings for Chromium {self.target_version}."')

        commit = _run_git('log', '-1', '--pretty=oneline', '--abbrev-commit')
        print(f'Done: {commit}')

    def run(self):
        """Run the upgrade process

        This is the main method used to carry out the whole update process.

        Return:
        Return True if completed entirely, and False otherwise.
        """
        if _is_upgrading(self.target_version, self.working_version) is not True:
            print(
                f'Cannot upgrade version from {self.target_version} to {self.working_version}',
                file=sys.stderr)
            return False

        self._update_package_version()
        if _run_npm_command('init') is False:
            print('Failures found when running npm run init', file=sys.stderr)
            return False

        if _run_npm_command('update_patches') is False:
            print('Failures found when running npm run update_patches',
                  file=sys.stderr)
            return False
        self._save_updated_patches()

        if _run_npm_command('chromium_rebase_l10n') is False:
            print('Failures found when running npm run chromium_rebase_l10n',
                  file=sys.stderr)
            return False
        self._save_rebased_l10n()

        return True


def main():
    parser = argparse.ArgumentParser(
        description='Convert HEX HRESULT to signed integer.')

    parser.add_argument('-p',
                        '--previous',
                        help='The previous version to be shown as base.',
                        required=True)
    parser.add_argument('-t',
                        '--to',
                        help='The version being upgraded to.',
                        required=True)

    args = parser.parse_args()

    if Upgrade(args.previous, args.to).run() is False:
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
