#!/usr/bin/env python3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Version upgrade command line tool.

This tool is used to upgrade Brave to a desired version of Chromium. The goal
is to produce a set of patches for the new chromium base version, that looks
something like:

1. Update from Chromium [from] to [to].
2. Conflict-resolved patches from Chromium [from] to [to].
3. Update patches from Chromium [from] to [to].
4. Updated strings for Chromium [to].

The process is started by providing a target version to upgrade to. The tool
uses the current upstream branch for the current branch as the base version,
although that can be overriden with `--previous`.

  git checkout -b cr135 origin/master
  tools/cr/upgrade.py --to=135.0.7037.1

The workflow with this script:

1. The first commit will be created where package.json is updated, and so is
   the pinslist timestamp source.
2. The repository will be initialised with the newer version of Chromium,
   followed where all patches will be applied on the newer version.
3. Any patches that fail to apply will be reattempted with the --3way option.
4. If any patches still fail to apply, the process will stop, and the user will
   be asked to resolve the conflicts manually for the files that cannot be
   resolved. In such a case, the run will stop.
5. If any patches are deleted, the process will be stopped and the user will be
    asked to commit the deleted patches manually, preserving the history of why
    they are deleted.
5. Having resolved all conflicts, the tool will be run again with the same
   arguments, and with the added --continue flag.
6. This tool will then continue from the point where it stopped, staging all
   patches that were applied, and committing them as conflict-resolved patches.
7. This tool will then continue to update the patches, and rebase the strings
   for the new version of Chromium.

Steps 3-7 may end up being skipped altogether if no failures take place, or in
part if resolution is possible without manual intervention.

Additionally, this tool can be run with the --update-patches-only flag. This is
useful to generate the "Update patches" and "Updated strings" commits on their
own when rebasing branches, regenerating these files as desired.
"""

import argparse
from datetime import datetime
import json
from pathlib import Path
import re
import subprocess
import sys

# This file is updated whenever the version number is updated in package.json
PINSLIST_TIMESTAMP_FILE = 'chromium_src/net/tools/transport_security_state_generator/input_file_parsers.cc'
VERSION_UPGRADE_FILE = '.version_upgrade'
PACKAGE_FILE = 'package.json'


def _run_git(*cmd):
    """Runs a git command on the current repository.

  This function returns a proper utf8 string in success, otherwise it allows
  the exception thrown by subprocess through.

  e.g:
    _run_git('add', '-u', '*.patch')
  """
    cmd = ['git'] + list(cmd)
    return subprocess.check_output(cmd).strip().decode('utf-8')


def _git_commit(message):
    """Commits the current staged changes.

    This function also prints the commit hash/message to the user.
    Args:
      message:
        The message to be used for the commit.
    """
    _run_git('commit', '-m', message)
    commit = _run_git('log', '-1', '--pretty=oneline', '--abbrev-commit')
    print(f'Done: {commit}')


def _get_current_branch_upstream_name():
    """Retrieves the name of the current branch's upstream.
    """
    return _run_git('rev-parse', '--abbrev-ref', '--symbolic-full-name',
                    '@{upstream}')

def _load_package_file(branch):
    """Retrieves the json content of package.json for a given revision

  Args:
    branch:
      A branch or hash to load the file from.
  """
    package = _run_git('show', f'{branch}:{PACKAGE_FILE}')
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
        if not line:
            break
        print(line.decode('utf-8').rstrip())

    return process.wait() == 0


def _update_pinslist_timestamp():
    """Updates the pinslist timestamp in the input_file_parsers.cc file for the
    version commit.
    """
    try:
        with open(PINSLIST_TIMESTAMP_FILE, "r", encoding="utf-8") as file:
            content = file.read()
    except FileNotFoundError:
        print(f"ERROR: File '{PINSLIST_TIMESTAMP_FILE}' not found. Aborting.")
        sys.exit(1)

    pattern = r"# Last updated:.*\nPinsListTimestamp\n[0-9]{10}\n"
    match = re.search(pattern, content, flags=re.DOTALL)
    if not match:
        print('ERROR: Expected pattern for PinsListTimestamp block not found. '
              'Aborting.')
        sys.exit(1)

    # Update the timestamp
    timestamp = int(datetime.now().timestamp())
    readable_timestamp = datetime.fromtimestamp(timestamp).strftime(
        '%a %b %d %H:%M:%S %Y')
    updated_content = re.sub(
        pattern,
        f"# Last updated: {readable_timestamp}\nPinsListTimestamp\n{timestamp}\n",
        content,
        flags=re.DOTALL,
    )

    # Write back to the file
    with open(PINSLIST_TIMESTAMP_FILE, "w", encoding="utf-8") as file:
        file.write(updated_content)

    updated = _run_git('diff', PINSLIST_TIMESTAMP_FILE)
    if updated == '':
        raise Exception('Pinslist timestamp failed to update.')


def _get_apply_patches_list():
    """Retrieves the list of patches to be applied by running 
    `npm run apply_patches`
    """

    process = subprocess.run([
        'npm', 'run', 'apply_patches', '--', '--print-patch-failures-in-json'
    ],
                             capture_output=True,
                             text=True,
                             check=False)

    # This is a regex to match the json output of the patches that failed to apply.
    match = re.search(r'\[\s*{.*?}\s*\]', process.stdout, re.DOTALL)
    if match is None:
        print('No patches to apply')
        return None

    return json.loads(match.group(0))


def get_subdirs_after_patches(path: str) -> str:
    """Extracts the subdirectories after 'patches', which relates to where a
    patch file should be applied.
    """
    parts = Path(path).parts
    if "patches" not in parts:
        raise Exception('Apply patches failed to provide a list of patches.')

    idx = parts.index("patches") + 1
    result = '/'.join(parts[idx:-1])  # Exclude the filename
    return result


class GitStatus:
    """Runs `git status` and provides a summary.
    """

    def __init__(self):
        self.status = _run_git('status', '--short')

        # a list of all deleted files, regardless of their staged status.
        self.deleted = []

        # a list of all modified files, regardless of their staged status.
        self.modified = []

        for line in self.status.splitlines():
            [status, path] = line.lstrip().split(' ', 1)
            if status == 'D':
                self.deleted.append(path)
            elif status == 'M':
                self.modified.append(path)

    def has_deleted_files(self):
        return len(self.deleted) > 0

    def has_deleted_patch_files(self):
        return any(path.endswith('.patch') for path in self.deleted)


class PatchFailureResolver:
    """Assist patch-failure resolutions, applying patches, reseting patches.

  This class provides the set of data and methods to assist patch failure
  resolution, persisting data across calls, allowing the resolution to occur in
  two steps.

  The operations are:
    * get a list of failed patches
    * try to apply the patches with --3way option using `apply_patches_3way()`
    * check if there are files with conflicts to resolve manually
    * `git reset` all places where patch files were applied
    * Save a file with the list of patches that were applied if manual resolution is required.
  """

    def __init__(self, target_version):
        # A dictionary that holds a list for all patch files affected, by
        # repository.
        self.patch_files = {}

        # A list of files that require manual conflict resolution before continuing.
        self.files_with_conflicts = []

        # The target version aimed by the patches. This is used to validate the
        # continuation mode.
        self.target_version = target_version

    def apply_patches_3way(self):
        """Applies patches that have failed using the --3way option to allow for
        manual conflict resolution.

        This method will apply the patches and reset the state of applied patches.
        Additionally, it will also produce a list of the files that are waiting for
        conflict resolution.

        A list of the patches applied will be produced as well.
        """
        print('Applying conflicting patches:')

        # the raw list of patches that failed to apply.
        patch_failures = _get_apply_patches_list()
        if patch_failures is None:
            raise Exception(
                'Apply patches failed to provide a list of patches.')

        for patch in patch_failures:
            patch_name = patch['patchPath']
            print(patch_name)

            if patch.get('reason', '') == 'SRC_REMOVED':
                # Skip patches that can't apply as the source is gone.
                print(
                    f'Skipping patch deleted due to repo source deletion: {patch_name}'
                )
                continue

            # Grouping patch files by their repositories, so to allow us to iterate
            # through them, applying them in their repo paths.
            subdir = get_subdirs_after_patches(patch_name)
            self.patch_files.setdefault(subdir, []).append(patch_name)
        print('...')

        for subdir, patches in self.patch_files.items():
            repo_path = f'../{subdir}'
            patch_relative_path = "../" * repo_path.count(
                '/') if repo_path != '../' else ''

            cmd = [
                'git', '-C', f'../{subdir}', 'apply', '--3way',
                '--ignore-space-change', '--ignore-whitespace'
            ]
            cmd += [f'{patch_relative_path}brave/{patch}' for patch in patches]

            process = subprocess.run(cmd,
                                     capture_output=True,
                                     text=True,
                                     check=False)
            self.files_with_conflicts += [
                line.lstrip("U ") for line in process.stderr.splitlines()
                if line.startswith('U ')
            ]

        for subdir, patches in self.patch_files.items():
            # Resetting any staged states from apply patches as that can cause
            # issues when generating patches.
            _run_git('-C', f'../{subdir}', 'reset', 'HEAD')

        if len(self.files_with_conflicts) > 0:
            print('Fix conflicts for the following files:')
            print(' '.join(self.files_with_conflicts))

        # A continuation file is created in case `--continue` is still required
        # for some reason (e.g. manual conflict resolution, patches being
        # deleted).
        continuation_file = {}
        continuation_file['version'] = self.target_version
        continuation_file['patches'] = self.patch_files
        continuation_file['conflicts'] = self.files_with_conflicts

        # Saving the patch file list to be able to revisit it later in JSON format.
        with open(VERSION_UPGRADE_FILE, 'w') as file:
            json.dump(continuation_file, file)

    def requires_conflict_resolution(self):
        return len(self.files_with_conflicts) > 0

    def stage_all_patches(self, ignore_deleted_files=False):
        """Stages all patches that were applied, so they can be committed as
        conflict-resolved patches.

        Args:
            ignore_deleted_files:
                If set to True, deleted files will be ignored, and not staged.
        """
        for _, patches in self.patch_files.items():
            for patch in patches:
                if ignore_deleted_files is True and Path(
                        patch).exists() is False:
                    # Skip deleted files.
                    continue

                # TODO: `_run_git` should be able to take an array.
                _run_git('add', patch)

    def load_continuation_file(self):
        continuation_file = {}
        with open(VERSION_UPGRADE_FILE, 'r') as file:
            continuation_file = json.load(file)

        if continuation_file['version'] != self.target_version:
            # This validation is in place for something that shouldn't happen.
            # If this is being hit, it means some wrong continuation file is in
            # the tree, and the process should be started all over.
            raise Exception(
                F'Target version in {VERSION_UPGRADE_FILE} does not match the target version.'
            )

        self.patch_files = continuation_file['patches']
        self.files_with_conflicts = continuation_file['conflicts']


def _get_chromium_version_from_git(branch):
    """Retrieves the Chromium tag in package.json

  This function will load the package.json file from a given branch, and
  retrieve the tag Chromium version value in it.

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
    to_version = [int(i) for i in target_version.split('.')]
    in_version = [int(i) for i in working_version.split('.')]

    if len(in_version) != 4 or len(to_version) != 4:
        return False

    for origin, target in zip(in_version, to_version):
        if target > origin:
            return True
        if target < origin:
            return False

    return False


def has_staged_changed():
    return _run_git('diff', '--cached', '--stat') != ''


class Upgrade:
    """The upgrade process, holding the data related to the upgrade.

  This class produces an object that is reponsible for keeping track of the
  upgrade process step-by-step. It acquires all the common data necessary for
  its completion.
  """

    def __init__(self, base_branch, target_version):
        # The base branch used to fetch the base version from. Defaults to the
        # upstream branch of the current branch if none specified.
        self.base_branch = base_branch
        if self.base_branch is None or self.base_branch == '':
            self.base_branch = _get_current_branch_upstream_name()
        if self.base_branch is None or self.base_branch == '':
            raise Exception(
                'Cannot determine the upstream. Provide a --previous argument '
                'value or set an upstream branch.')

        # The target version that is being upgraded to.
        self.target_version = target_version

        # The base chromium version from the upstream/previous branch
        self.base_version = _get_chromium_version_from_git(self.base_branch)

        # The current chromium version in the current branch
        self.working_version = _get_chromium_version_from_git('HEAD')

    def _update_package_version(self):
        """Creates the change upgrading the Chromium version

    This is for the creation of the first commit, which means updating
    package.json to the target version provided, and commiting the change to
    the repo
    """
        package = _load_package_file('HEAD')
        package['config']['projects']['chrome']['tag'] = self.target_version
        with open(PACKAGE_FILE, "w") as package_file:
            json.dump(package, package_file, indent=2)

        _run_git('add', PACKAGE_FILE)

        # Pinlist timestamp update occurs with the package version update.
        _update_pinslist_timestamp()
        _run_git('add', PINSLIST_TIMESTAMP_FILE)
        _git_commit(
            f'Update from Chromium {self.base_version} to Chromium {self.target_version}.'
        )

    def _save_updated_patches(self):
        """Creates the updated patches change

    This function creates the third commit in the order of the update, saving
    all patches that might have been changed or deleted. Untracked patches are
    excluded from addition at this stage.
    """
        _run_git('add', '-u', '*.patch')

        if has_staged_changed() is False:
            # Nothing to commit
            return
        _git_commit(
            f'Update patches from Chromium {self.base_version} to Chromium {self.target_version}.'
        )

    def _save_rebased_l10n(self):
        """Creates string rebase change

    This function stages, and commits, all changed, updated, or deleted files
    resulting from running npm run chromium_rebase_l10n.
    """
        _run_git('add', '*.grd', '*.grdp', '*.xtb')
        if has_staged_changed() is False:
            # Nothing to commit
            return

        _git_commit(f'Updated strings for Chromium {self.target_version}.')

    def _save_conflict_resolved_patches(self):
        _git_commit(
            f'Conflict-resolved patches from Chromium {self.base_version} to Chromium {self.target_version}.'
        )

    def _run_update_patches_with_no_deletions(self):
        """Runs update_patches and returns if any deleted patches are found.

        This function is usually preferred, as it checks if any patches are
        deleted after running update_patches. Deleted patches should be
        committed manually with a history of why the patching is not required
        anymore.

        return:
          Returns True if no deleted patches are found, and False otherwise.
        """
        if _run_npm_command('update_patches') is not True:
            raise Exception(
                'Failures found when running npm run update_patches')

        status = GitStatus()
        if status.has_deleted_patch_files() is True:
            print(
                'Deleted patches detected. These should be committed as their own changes:\n'
                + '\n'.join(status.deleted),
                file=sys.stderr)
            return False

        return True

    def run(self, is_continuation, no_conflict_continuation):
        """Run the upgrade process

    This is the main method used to carry out the whole update process.

    Args:
      is_continuation:
        Indicates that we are picking up from a resolve-conflict failure.
      no_conflict_continuation:
        Indicates a continuation does not produces a conflict-resolved change.

    Return:
      Return True if completed entirely, and False otherwise.
    """
        override_continuation = is_continuation
        if is_continuation is not True:
            if self.target_version == self.working_version:
                print(
                    f"It looks like upgrading to {self.target_version} has already started.",
                    file=sys.stderr)
                return False

            if _is_upgrading(self.target_version,
                             self.working_version) is not True:
                print(
                    f'Cannot upgrade version from {self.target_version} to {self.working_version}',
                    file=sys.stderr)
                return False

            self._update_package_version()

            process = subprocess.run(['npm', 'run', 'init'],
                                     capture_output=True,
                                     text=True,
                                     check=False)
            if process.returncode != 0 and 'Exiting as not all patches were successful!' in process.stderr.splitlines(
            )[-1]:
                resolver = PatchFailureResolver(self.target_version)
                resolver.apply_patches_3way()
                if resolver.requires_conflict_resolution() is True:
                    # Manual resolution required.
                    return False

                if self._run_update_patches_with_no_deletions() is not True:
                    return False

                resolver.stage_all_patches()
                self._save_conflict_resolved_patches()

                # With all conflicts resolved, it is necessary to close the
                # upgrade with all the same steps produced when running an
                # upgrade continuation, as recovering from a conflict-
                # resolution failure.
                override_continuation = True
            elif process.returncode != 0:
                print(process.stderr, file=sys.stderr)
                print('Failures found when running npm run init',
                      file=sys.stderr)
                return False
            else:
                # When no conflicts come back, we can proceed with the
                # update_patches.
                if self._run_update_patches_with_no_deletions() is not True:
                    return False
        elif is_continuation is True and no_conflict_continuation is False:
            # This is when a continuation follows conflict resolution. The
            # tries to recover from where it stopped.
            resolver = PatchFailureResolver(self.target_version)
            resolver.load_continuation_file()

            if resolver.requires_conflict_resolution() is True:
                if self._run_update_patches_with_no_deletions() is not True:
                    return False

            resolver.stage_all_patches(ignore_deleted_files=True)

            if has_staged_changed() is False:
                print(
                    'Nothing has been staged to commit conflict-resolved patches.',
                    file=sys.stderr)
                return False

            self._save_conflict_resolved_patches()

        self._save_updated_patches()
        if is_continuation is True or override_continuation is True:
            # Run init again to make sure nothing is missing after updating patches.
            if _run_npm_command('init') is not True:
                print(
                    'Failures found when running npm run init after committing patches.',
                    file=sys.stderr)
                return False

        if _run_npm_command('chromium_rebase_l10n') is not True:
            print('Failures found when running npm run chromium_rebase_l10n',
                  file=sys.stderr)
            return False
        self._save_rebased_l10n()

        return True

    def generate_patches(self):
        if _run_npm_command('init') is not True:
            raise Exception(
                'Failures found when running npm run init after committing patches.'
            )

        if _run_npm_command('update_patches') is not True:
            raise Exception(
                'Failures found when running npm run update_patches')
        self._save_updated_patches()

        if _run_npm_command('chromium_rebase_l10n') is not True:
            raise Exception(
                'Failures found when running npm run chromium_rebase_l10n')
        self._save_rebased_l10n()


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        # Custom formatter to preserve line breaks in the docstring
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument('--previous',
                        help='The previous version to be shown as base.')
    parser.add_argument(
        '--to',
        help=
        'The branch used as the base version. In the absence of this argument,'
        ' defaults to the current upstream branch.',
        required=True)
    parser.add_argument(
        '--continue',
        action='store_true',
        help=
        'Picks up from manual patch conflict resolution, stages the patches '
        'previously flagged as with conflict, commits the staged patches as well '
        'anything that my have been staged manually, and carries on from that '
        'point onward generating the "Update patches" and "Updated strings" '
        'changes.',
        dest='is_continuation')
    parser.add_argument(
        '--no-conflict-change',
        action='store_true',
        help=
        'Indicates that a continuation does not have conflict patches to commit '
        '(when conflict resolution is committed as its own fix manually with '
        'specific messages).',
        dest='no_conflict')
    parser.add_argument(
        '--update-patches-only',
        action='store_true',
        help=
        'Pass this flag to have udpate patches/update string changes created at '
        'on their own.',
        dest='update_patches_only')

    args = parser.parse_args()
    upgrade = Upgrade(args.previous, args.to)

    if args.update_patches_only:
        upgrade.generate_patches()
    elif upgrade.run(args.is_continuation, args.no_conflict) is False:
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
