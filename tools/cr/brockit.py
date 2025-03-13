#!/usr/bin/env python3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""**🚀Brockit! Guide**

```
⠀⠀⠀⠀⠀⢀⣴⣶⣶⣶⣶⣶⣶⣶⣶⣦⡀⠀⠀⠀⠀⠀
⠀⣀⣤⣤⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣤⣤⣀⠀                              🚀
⣾⣿⣿⣿⣿⡿⠛⠻⠿⠋⠁⠈⠙⠛⠛⠛⢿⣿⣿⣿⣿⣷                         .
⢈⣿⣿⣿⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣿⣿⣿⡁                     .
⣿⣿⣟⠁⠀⠴⣿⣿⣿⡄⠀⠀⢠⣿⣿⣿⠦⠀⠈⣻⣿⣿                .
⣿⣿⣿⣧⡀⠀⠀⠀⣽⠇⠀⠀⠸⣯⠀⠀⠀⢀⣴⣿⣿⣿             .
⠸⣿⣿⣿⣿⣄⠀⢼⣯⣀⠀⠀⣀⣽⡧⠀⢠⣾⣿⣿⣿⠇          .
⠀⣿⣿⣿⣿⡟⠀⠀⠉⢻⣿⣿⡟⠉⠀⠀⢹⣿⣿⣿⣿⠀        .
⠀⢹⣿⣿⣿⣧⣀⣀⣤⣾⣿⣿⣷⣤⣀⣀⣴⣿⣿⣿⡏⠀      .
⠀⠈⣿⣿⣿⣿⣿⣿⣿⠛⠋⠙⠛⣿⣿⣿⣿⣿⣿⣿⠁⠀     .
⠀⠀⠸⣿⣿⣿⣿⣿⣿⣷⣤⣠⣾⣿⣿⣿⣿⣿⣿⠇⠀⠀    .
⠀⠀⠀⠀⠙⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠋⠀⠀⠀⠀   .
⠀⠀⠀⠀⠀⠀⠀⠙⠻⣿⣿⣿⣿⠟⠋⠀⠀⠀⠀⠀⠀⠀💥
```

This is *🚀Brockit!* (Brave Rocket? Brave Rock it? Broke it?): a tool to help
upgrade Brave to a newer Chromium base version. The main goal is to produce
use it to commit the following changes:

 * Update from Chromium [from] to [to].
 * Conflict-resolved patches from Chromium [from] to [to].
 * Update patches from Chromium [from] to [to].
 * Updated strings for Chromium [to].

To start it off, provide a `--to` target version, and a base branch to be used,
either by providing a `--previous` argument, or by having an upstream branch to
to your current branch.

```sh
tools/cr/brockit.py --to=135.0.7037.1 --previous=origin/master
```

Using upstream:

```sh
git branch --set-upstream-to=origin/master
tools/cr/brockit.py --to=135.0.7037.1
```

The following steps will take place:

1. A *Update from Chromium [from] to [to]* commit will be created. This commit
   contains changes to `package.json` and to the pinlist timestamp file.
2. `npm run init` will be run with the newer version.
3. For any patches that fail to apply during `init`, there will be another
   attempt to apply them using `--3way`.
4. If any patches still fail to apply, the process will stop. A summary of
   files with conflicts will be provided for resolution.
5. Deleted patches will also cause the  *🚀Brockit!* to stop . The user is
   expected to provide separate commits for deleted patches, explaining the
   reason.
6. Having resolved all conflicts. Restart *🚀Brockit!* with the same arguments
   as before, but add `--continue` as well.
7. *🚀Brockit!* will pick up from where it stopped, possibly running
  `npm run update_patches`, staging all patches, and committing the under
  *Conflict-resolved patches from Chromium [from] to [to].*
8. *Update patches from Chromium [from] to [to]* will be committed.
9. *Updated strings for Chromium [to]* will be committed.

Steps 3-7 may end up being skipped altogether if no failures take place, or in
part if resolution is possible without manual intervention.

Additionally, *🚀Brockit!* can be run with the `--update-patches-only` flag. This
is useful to generate the "Update patches" and "Updated strings" commits on
their own when rebasing branches, regenerating these files as desired.

**Github support**

Upgrade runs can also be accompanied by github tasks, by passing
`--with-github` when running *🚀Brockit!*.

```sh
tools/cr/brockit.py --to=135.0.7037.1 --with-github
```

This will attempt to create/update the github issue for the run as part of
the whole process. Another option is to use `--github-issue-only` to have these
GitHub run as a standalone.
"""

import argparse
from datetime import datetime
import json
from pathlib import Path
import platform
import logging
import re
from rich.console import Console
from rich.logging import RichHandler
from rich.markdown import Markdown
from rich.padding import Padding
import subprocess
import sys

console = Console()

class RichHelpFormatter(argparse.RawDescriptionHelpFormatter):
    """A custom formatter that allows for markdown in `--help`.
    """
    def __init__(self, prog):
        super().__init__(prog)

    def format_help(self):
        console.print(Markdown(__doc__))
        console.print('')
        print(super().format_help())
        return ""

# This file is updated whenever the version number is updated in package.json
PINSLIST_TIMESTAMP_FILE = 'chromium_src/net/tools/transport_security_state_generator/input_file_parsers.cc'
VERSION_UPGRADE_FILE = '.version_upgrade'
PACKAGE_FILE = 'package.json'
CHROMIUM_VERSION_FILE = 'chrome/VERSION'

# The text for the body used on a GitHub issue for a version bump.
MINOR_VERSION_BUMP_ISSUE_TEMPLATE = """### Minor Chromium bump

{googlesource_link}

### QA tests

- Check branding items
- Check for version bump

### Minor Chromium bump

- No specific code changes in Brave (only line number changes in patches)
"""

# The with the log of changes between two versions.
GOOGLESOURCE_LINK = 'https://chromium.googlesource.com/chromium/src/+log/{from_version}..{to_version}?pretty=fuller&n=10000'

# A decorator to be shown for messages that the user should address before
# continuing.
ACTION_NEEDED_DECORATOR = '[bold yellow](action needed)[/]'

class Terminal:
    """A class that holds the application data and methods.
    """

    def __init__(self):
        self.status = None
        self.starting_status_message = ''

    def set_status_object(self, status):
        """Preserves the status object for updates.

    This function is used to preserve the status object for updates, so that
    the status can be updated with the initial status message.
    """
        self.status = status
        self.starting_status_message = status.status

    def run(self, cmd):
        """Runs a command on the terminal.
        """

        def truncate_on_max_length(message):
            # Truncate at the first newline if it exists
            if '\n' in message:
                message = message.split('\n')[0] + '...'
            # Truncate at max_length if the message is still too long
            max_length = console.size.width - 40
            if len(message) > max_length:
                message = message[:max_length - 3] + '...'
            return message

        if self.status is not None:
            self.status.update(
                f'[bold green]{self.starting_status_message}[/] [bold cyan]({truncate_on_max_length(" ".join(cmd))})[/]'
            )
        logging.debug('λ %s', ' '.join(cmd))

        try:
            # It is necessary to pass `shell=True` on Windows, otherwise the
            # process handle is entirely orphan and can't resolve things like
            # `npm`.
            return subprocess.run(cmd,
                                  capture_output=True,
                                  text=True,
                                  check=True,
                                  shell=platform.system() == 'Windows')
        except subprocess.CalledProcessError as e:
            logging.debug('❯ %s', e.stderr.strip())
            raise e

    def run_git(self, *cmd):
        """Runs a git command on the current repository.

    This function returns a proper utf8 string in success, otherwise it allows
    the exception thrown by subprocess through.

    e.g:
        self.run_git('add', '-u', '*.patch')
    """
        cmd = ['git'] + list(cmd)
        return self.run(cmd).stdout.strip()

    def log_task(self, message):
        """Logs a task to the console using common decorators
        """
        console.log(f'[bold red]*[/] {message}')

    def git_commit(self, message):
        """Commits the current staged changes.

        This function also prints the commit hash/message to the user.
        Args:
        message:
            The message to be used for the commit.
        """
        if _has_staged_changed() is False:
            # Nothing to commit
            return

        self.run_git('commit', '-m', message)
        commit = self.run_git('log', '-1', '--pretty=oneline',
                              '--abbrev-commit')
        self.log_task(f'[bold]✔️ [/] [italic]{commit}')

    def run_npm_command(self, *cmd):
        """Runs an npm build command.

      This function will run 'npm run' commands appended by any extra arguments
      are passed into it.

      e.g:
          self.run_npm_command('init')
      """
        cmd = ['npm', 'run'] + list(cmd)
        return self.run(cmd)

terminal = Terminal()


class IncendiaryErrorHandler(RichHandler):
    """ A custom handler that adds emojis to error messages.
    """

    def emit(self, record: logging.LogRecord) -> None:
        if record.levelno == logging.ERROR:
            record.msg = f"🔥🔥 {record.getMessage()}"
        elif record.levelno == logging.DEBUG:
            # Debug messages should be printed as normal logs, otherwise the
            # formatting goes all over the place with the status bar.
            console.log(f'[dim]{record.getMessage()}[/]', _stack_offset=8)
            return

        super().emit(record)


def _has_staged_changed():
    return terminal.run_git('diff', '--cached', '--stat') != ''


def _get_current_branch_upstream_name():
    """Retrieves the name of the current branch's upstream.
    """
    try:
        return terminal.run_git('rev-parse', '--abbrev-ref',
                                '--symbolic-full-name', '@{upstream}')
    except subprocess.CalledProcessError:
        logging.error(
            'Could not determine the upstream branch. Please provide a '
            '[bold cyan]--previous[/] argument or set an upstream branch to '
            'your current branch.')
        sys.exit(1)

def _load_package_file(branch):
    """Retrieves the json content of package.json for a given revision

  Args:
    branch:
      A branch or hash to load the file from.
  """
    package = terminal.run_git('show', f'{branch}:{PACKAGE_FILE}')
    return json.loads(package)

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

    updated = terminal.run_git('diff', PINSLIST_TIMESTAMP_FILE)
    if updated == '':
        raise Exception('Pinslist timestamp failed to update.')


def _is_gh_cli_logged_in():
    """Checks if the GitHub CLI is logged in.
    """
    try:
        result = terminal.run(['gh', 'auth', 'status']).stdout.strip()
        if 'Logged in to github.com account' in result:
            return True
    except subprocess.CalledProcessError:
        return False

    return False

def _get_apply_patches_list():
    """Retrieves the list of patches to be applied by running 
    `npm run apply_patches`
    """

    try:
        terminal.run_npm_command('apply_patches', '--',
                                 '--print-patch-failures-in-json')
    except subprocess.CalledProcessError as e:
        # This is a regex to match the json output of the patches that failed
        # to apply.
        match = re.search(r'\[\s*{.*?}\s*\]', e.stdout, re.DOTALL)
        if match is None:
            return None
        return json.loads(match.group(0))

    return None


class GitStatus:
    """Runs `git status` and provides a summary.
    """

    def __init__(self):
        self.git_status = terminal.run_git('status', '--short')

        # a list of all deleted files, regardless of their staged status.
        self.deleted = []

        # a list of all modified files, regardless of their staged status.
        self.modified = []

        # a list of all untracked files.
        self.untracked = []

        for line in self.git_status.splitlines():
            [status, path] = line.lstrip().split(' ', 1)
            if status == 'D':
                self.deleted.append(path)
            elif status == 'M':
                self.modified.append(path)
            elif status == '??':
                self.untracked.append(path)

    def has_deleted_files(self):
        return len(self.deleted) > 0

    def has_deleted_patch_files(self):
        return any(path.endswith('.patch') for path in self.deleted)

    def has_untracked_patch_files(self):
        return any(path.endswith('.patch') for path in self.untracked)


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

    def __init__(self, target_version, working_version=None):
        # A dictionary that holds a list for all patch files affected, by
        # repository.
        self.patch_files = {}

        # This is a flat of list of patches that cannot be applied due to their
        # source file being deleted.
        self.patches_to_deleted_files = []

        # A list of files that require manual conflict resolution before continuing.
        self.files_with_conflicts = []

        # These are patches that fail entirely when running apply with `--3way`.
        self.broken_patches = []

        # The target version aimed by the patches. This is used to validate the
        # continuation mode.
        self.target_version = target_version

        # The working version from where the upgrade actually started
        self.working_version = working_version

    def apply_patches_3way(self, launch_vscode=False):
        """Applies patches that have failed using the --3way option to allow for
        manual conflict resolution.

        This method will apply the patches and reset the state of applied patches.
        Additionally, it will also produce a list of the files that are waiting for
        conflict resolution.

        A list of the patches applied will be produced as well.
        """
        # the raw list of patches that failed to apply.
        patch_failures = _get_apply_patches_list()
        logging.debug(patch_failures)
        if patch_failures is None:
            raise Exception(
                'Apply patches failed to provide a list of patches.')

        def get_subdirs_after_patches(path: str) -> str:
            """Extracts the subdirectories after 'patches'.

            The result is something like
              'patches/build-android-gyp-dex.py.patch' -> ''
              'patches/third_party/test/foo.patch' -> 'third_party/test'
            """
            parts = Path(path).parts
            if "patches" not in parts:
                raise Exception(
                    'Apply patches failed to provide a list of patches.')

            idx = parts.index("patches") + 1
            result = '/'.join(parts[idx:-1])  # Exclude the filename
            return result

        for patch in patch_failures:
            patch_name = patch['patchPath']

            if patch.get('reason', '') == 'SRC_REMOVED':
                # Skip patches that can't apply as the source is gone.
                self.patches_to_deleted_files.append(patch_name)
                continue

            # Grouping patch files by their repositories, so to allow us to iterate
            # through them, applying them in their repo paths.
            subdir = get_subdirs_after_patches(patch_name)
            self.patch_files.setdefault(subdir, []).append(patch_name)

        patches_to_apply = [
            patch for patch_list in self.patch_files.values()
            for patch in patch_list
        ]
        if len(patches_to_apply) > 0:
            terminal.log_task(
                '[bold]Reapplying patch files with --3way:\n[/]' +
                '\n'.join(f'    * {file}' for file in patches_to_apply))

        def get_repo_path_from_subdir(subdir):
            """Returns relative repo path for a given patch subdir.

                The result is something like
                '' -> '../'
                'third_party/test' -> '../third_party/test'
            """
            return f'../{subdir}{"" if subdir == "" else "/"}'

        def get_patch_relative_path_from_repo(repo_path):
            """Returns the relative path to the patch file from the repo.
                '../' -> ''
                '../third_party/test' -> '../../'
            """
            return "../" * (repo_path.count('/') -
                            1) if repo_path != '../' else ''

        for subdir, patches in self.patch_files.items():
            repo_path = get_repo_path_from_subdir(subdir)
            patch_relative_path = get_patch_relative_path_from_repo(repo_path)

            cmd = [
                'git', '-C', f'../{subdir}', 'apply', '--3way',
                '--ignore-space-change', '--ignore-whitespace'
            ]

            all_output = []
            for patch in patches:
                # Each patch is run individually as a broken patch can stop a
                # git run for all subsequent patches.

                try:
                    terminal.run(cmd + [f'{patch_relative_path}brave/{patch}'])
                except subprocess.CalledProcessError as e:
                    # If the process fails, we need to collect the files that
                    # failed to apply for manual conflict resolution.
                    if 'does not exist in index' in e.stderr:
                        # This type of detection could occur in certain cases
                        # when `npm run init` or `sync` were not run for the
                        # working branch. It may be useful to warn.
                        self.patches_to_deleted_files.append(patch)
                        logging.warning(
                            'Patch with missing file detected during --3way '
                            'apply, which may indicate a bad sync state before'
                            ' starting to upgrade. %s', patch)
                    elif e.stderr.startswith('error:'):
                        self.broken_patches.append(patch)

                    all_output += e.stderr.strip().splitlines()

            # Checking for " U" which means conflicts were added.
            self.files_with_conflicts += [
                f'{repo_path}{line.lstrip("U ")}' for line in all_output
                if line.startswith('U ')
            ]

        for subdir, patches in self.patch_files.items():
            # Resetting any staged states from apply patches as that can cause
            # issues when generating patches.
            terminal.run_git('-C', f'../{subdir}', 'reset', 'HEAD')

        vscode_args = ['code']
        if len(self.patches_to_deleted_files) > 0:
            vscode_args += self.patches_to_deleted_files
            # The goal in this this section is print a report for listing every
            # patch that cannot apply anymore because the source file is gone,
            # fetching from git the commit and the reason why exactly the file
            # is not there anymore (e.g. renamed, deleted).

            terminal.log_task(
                f'[bold]Files that cannot be patched anymore {ACTION_NEEDED_DECORATOR}:[/]'
            )

            # This set will hold the information about the deleted patches
            # in a way that they can be grouped around the chang that caused
            # their removal.
            deletion_report = {}

            for patch_name in self.patches_to_deleted_files:
                subdir = get_subdirs_after_patches(patch_name)
                patch_relative_path = get_patch_relative_path_from_repo(
                    get_repo_path_from_subdir(subdir))

                try:
                    # Calling `git apply` with `--check` to have git tell us
                    # the exact name of the file that the patch is meant for.
                    #
                    # The failure output should looks something like:
                    #   error: base/some_file.cc: No such file or directory
                    terminal.run_git(
                        '-C', f'../{subdir}', 'apply', '--check',
                        f'{patch_relative_path}brave/{patch_name}')
                except subprocess.CalledProcessError as e:
                    [status, file, reason] = e.stderr.lstrip().split(':', 2)

                    # The command is expeted to fail, but the reason should be
                    # that the source for the patch is deleted. Any other
                    # types of failures should be reported.
                    if status != 'error' or 'No such file or directory' not in reason:
                        raise e

                    file = file.strip()
                    # This command fetches the last commit where the file path
                    # was mentioned, which is the culprit.
                    commit = terminal.run_git('-C', f'../{subdir}', 'log',
                                              '-1', '--pretty=%h', '--',
                                              file).strip()
                    if commit not in deletion_report:
                        deletion_report[commit] = {}
                        deletion_report[commit]['files'] = []

                        # Running `git show --name-status` to get more details
                        # about the culprit.
                        # Not very sure why, but by passing `--no-commit-id` the
                        # output looks something like this:
                        #
                        # M       chrome/VERSION
                        # commit 17bb6c858e818e81744de42ed292b7060bc341e5
                        # Author: Chrome Release Bot (LUCI)
                        # Date:   Wed Feb 26 10:17:33 2025 -0800
                        #
                        #     Incrementing VERSION to 134.0.6998.45
                        #
                        #     Change-Id: I6e995e1d7aed40e553d3f51e98fb773cd75
                        #
                        # So the output is read and split from the moment a
                        # a line starts with `commit`.
                        change = terminal.run_git('-C', f'../{subdir}', 'show',
                                                  '--name-status',
                                                  '--no-commit-id', commit)
                        sections = re.split(r'(?=^commit\s)',
                                            change,
                                            flags=re.MULTILINE)

                        deletion_report[commit]['summary'] = sections[1]
                        deletion_report[commit]['status'] = sections[0]

                    status = deletion_report[commit]['status']
                    status_datails = next(
                        (s for s in status.splitlines() if file in s),
                        None).split()

                    if status_datails[0] == 'D':
                        deletion_report[commit]['files'].append(
                            f'{file} [red bold](deleted)')
                    elif status_datails[0].startswith('R'):
                        file = status_datails[2]
                        deletion_report[commit]['files'].append(
                            f'{file}\n    ([yellow bold]renamed to[/] {status_datails[2]})'
                        )


            for commit, entry in deletion_report.items():
                for file in entry['files']:
                    console.log(Padding(f'✘ {file}', (0, 4)))

                summary = entry['summary']
                console.log(Padding(f'{summary}\n', (0, 8), style="dim"))

        if len(self.broken_patches) > 0:
            terminal.log_task(
                f'[bold]Broken patches that fail to apply entirely {ACTION_NEEDED_DECORATOR}:[/]'
            )
            for patch in self.broken_patches:
                file = next((entry.get('path') for entry in patch_failures
                             if entry.get('patchPath', '') in patch), None)
                if file is None:
                    console.log(Padding(f'✘ {patch}', (0, 4)))
                else:
                    console.log(Padding(f'✘ {patch} ➜ {file}', (0, 4)))
                    vscode_args += [patch, file]

        if len(self.files_with_conflicts) > 0:
            vscode_args += self.files_with_conflicts
            terminal.log_task(
                f'[bold]Manually resolve conflicts for {ACTION_NEEDED_DECORATOR}:[/]\n'
                + '\n'.join(f'    ✘ {file}'
                            for file in self.files_with_conflicts))


        # A continuation file is created in case `--continue` is still required
        # for some reason (e.g. manual conflict resolution, patches being
        # deleted).
        continuation_file = {}
        continuation_file['target_version'] = self.target_version
        continuation_file['working_version'] = self.working_version
        continuation_file['patches'] = self.patch_files
        continuation_file['conflicts'] = self.files_with_conflicts
        continuation_file['deleted'] = self.patches_to_deleted_files

        # Saving the patch file list to be able to revisit it later in JSON format.
        with open(VERSION_UPGRADE_FILE, 'w') as file:
            json.dump(continuation_file, file)

        if launch_vscode:
            terminal.run(vscode_args)

    def requires_conflict_resolution(self):
        return len(self.files_with_conflicts) > 0 or len(
            self.patches_to_deleted_files) > 0 or len(self.broken_patches) > 0

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

                # TODO: `terminal.run_git` should be able to take an array.
                terminal.run_git('add', patch)

    def load_continuation_file(self):
        continuation_file = {}
        with open(VERSION_UPGRADE_FILE, 'r') as file:
            continuation_file = json.load(file)

        if continuation_file['target_version'] != self.target_version:
            # This validation is in place for something that shouldn't happen.
            # If this is being hit, it means some wrong continuation file is in
            # the tree, and the process should be started all over.
            raise Exception(
                F'Target version in {VERSION_UPGRADE_FILE} does not match the target version.'
            )

        self.working_version = continuation_file['working_version']
        self.patch_files = continuation_file['patches']
        self.files_with_conflicts = continuation_file['conflicts']
        self.patches_to_deleted_files = continuation_file['deleted']


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


def _is_upgrading(target_version, from_version):
    """Checks whether a target version is higher than the working version

  This function is used to prevent a downgrade attempt, or re-applying an
  upgrade, by informing whether or not the target version is higher.

  Args:
    target_version:
      The version being upgraded to.
    from_version:
      The current version of the current branch.
  """
    to_version = [int(i) for i in target_version.split('.')]
    in_version = [int(i) for i in from_version.split('.')]

    if len(in_version) != 4 or len(to_version) != 4:
        return False

    for origin, target in zip(in_version, to_version):
        if target > origin:
            return True
        if target < origin:
            return False

    return False


def _is_upgrading_major(target_version, from_version):
    """Checks if this is a MAJOR upgrade.

    A major upgrade is different from a minor bump, and this function provides
    the test for that.
    """

    to_version = [int(i) for i in target_version.split('.')]
    in_version = [int(i) for i in from_version.split('.')]

    return to_version[0] > in_version[0]


def _read_chromium_version_file():
    """Retrieves the Chromium version from the VERSION file.

    This function reads directly from git, as VERSION gets patched during
    `apply_patches`.
    """
    version_parts = {}
    file = terminal.run_git('-C', '../', 'show',
                            f'HEAD:{CHROMIUM_VERSION_FILE}')
    for line in file.splitlines():
        key, value = line.strip().split('=')
        version_parts[key] = value
    return '{MAJOR}.{MINOR}.{BUILD}.{PATCH}'.format(**version_parts)


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

        # The target version that is being upgraded to.
        self.target_version = target_version

        # The base chromium version from the upstream/previous branch
        self.base_version = _get_chromium_version_from_git(self.base_branch)

        # The last version the branch was in, that the update is being started
        # from. This value is either retrieved from git, or from a continuation
        # file.
        self.working_version = None

        # The version currently set in the VERSION file.
        self.chromium_src_version = _read_chromium_version_file()

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

        terminal.run_git('add', PACKAGE_FILE)

        # Pinlist timestamp update occurs with the package version update.
        _update_pinslist_timestamp()
        terminal.run_git('add', PINSLIST_TIMESTAMP_FILE)
        terminal.git_commit(
            f'Update from Chromium {self.base_version} to Chromium {self.target_version}.'
        )

    def _save_updated_patches(self):
        """Creates the updated patches change

    This function creates the third commit in the order of the update, saving
    all patches that might have been changed or deleted. Untracked patches are
    excluded from addition at this stage.
    """
        terminal.run_git('add', '-u', '*.patch')

        terminal.git_commit(
            f'Update patches from Chromium {self.base_version} to Chromium {self.target_version}.'
        )

    def _save_rebased_l10n(self):
        """Creates string rebase change

    This function stages, and commits, all changed, updated, or deleted files
    resulting from running npm run chromium_rebase_l10n.
    """
        terminal.run_git('add', '*.grd', '*.grdp', '*.xtb')
        terminal.git_commit(
            f'Updated strings for Chromium {self.target_version}.')

    def _save_conflict_resolved_patches(self):
        terminal.git_commit(
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
        terminal.run_npm_command('update_patches')

        status = GitStatus()
        if status.has_deleted_patch_files() is True:
            logging.error(
                'Deleted patches detected. These should be committed as their '
                'own changes:\n' + '\n'.join(status.deleted))
            return False
        if status.has_untracked_patch_files() is True:
            logging.error(
                'Untracked patch files detected. These should be committed as '
                'their own changes:\n' + '\n'.join(status.untracked))
            return False

        return True

    def _get_googlesource_history_link(self, from_version):
        """Generates a link to the review history of the upgrade.

    This function generates a link to the review history of changes between the
    base version and the target version, for the current run.
        """

        return f'https://chromium.googlesource.com/chromium/src/+log/{from_version}..{self.target_version}?pretty=fuller&n=10000'

    def create_or_updade_issue_with_gh(self):
        """Creates a github issue for the upgrade.

        This function creates/updates the upgrade github issue.
        """

        title = 'Upgrade from Chromium {previous} to Chromium {to}'
        if _is_upgrading_major(self.target_version, self.base_version):
            # For major updates, the issue description doesn't have a precise
            # version number.
            title = title.format(previous=self.base_version.split('.')[0],
                                 to=self.target_version.split('.')[0])
        else:
            title = title.format(previous=self.base_version,
                                 to=self.target_version)

        link = GOOGLESOURCE_LINK.format(from_version=self.base_version,
                                        to_version=self.target_version)

        results = json.loads(
            terminal.run([
                'gh', 'issue', 'list', '--repo', 'brave/brave-browser',
                '--search', title, '--state', 'open', '--json',
                'number,title,url,body'
            ]).stdout.strip())
        issue = next((entry for entry in results if entry['title'] == title),
                     None)
        if issue is not None:
            pattern = r"https://chromium\.googlesource\.com/chromium/src/\+log/[^\s]+"
            body = re.sub(pattern, link, issue['body'])
            if body == issue['body']:
                terminal.log_task(
                    f'Github issue already up-to-date {issue["url"]}.')
            else:
                terminal.run([
                    'gh', 'issue', 'edit',
                    str(issue['number']), '--repo', 'brave/brave-browser',
                    '--body', f'{body}'
                ])
                terminal.log_task(f'GitHub issue udpated {issue["url"]}.')
            return

        body = MINOR_VERSION_BUMP_ISSUE_TEMPLATE.format(googlesource_link=link)
        issue_url = terminal.run([
            'gh', 'issue', 'create', '--repo', 'brave/brave-browser',
            '--title', title, '--body', f'{body}', '--label',
            '"Chromium/upgrade minor"', '--label', '"OS/Android"', '--label',
            '"OS/Desktop"', '--label', '"QA/Test-Plan-Specified"', '--label',
            '"QA/Yes"', '--label', '"release-notes/include"', '--assignee',
            'emerick', '--assignee', 'mkarolin', '--assignee',
            'cdesouza-chromium'
        ]).stdout.strip()
        terminal.log_task(f'GitHub Issue created for this bump: {issue_url}')

    def _run_internal(self, is_continuation, no_conflict_continuation,
                      launch_vscode, with_github):
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

        if with_github is True and _is_gh_cli_logged_in() is False:
            # Fail early if gh cli is not logged in.
            logging.error('GitHub CLI is not logged in.')
            return False

        override_continuation = is_continuation
        if is_continuation is not True:
            if self.target_version == self.working_version:
                logging.error(
                    "It looks like upgrading to %s has already started.",
                    self.target_version)
                return False

            if _is_upgrading(self.target_version,
                             self.working_version) is not True:
                logging.error('Cannot upgrade version from %s to %s',
                              self.target_version, self.working_version)
                return False

            self._update_package_version()

            try:
                terminal.run_npm_command('init')

                # When no conflicts come back, we can proceed with the
                # update_patches.
                if self._run_update_patches_with_no_deletions() is not True:
                    return False
            except subprocess.CalledProcessError as e:
                if 'There were some failures during git reset of specific repo paths' in e.stderr:
                    logging.warning(
                        '[bold cyan]npm run init[/] is failing to reset some'
                        ' paths. This could be happening because of a bad sync'
                        'state before starting the upgrade.')

                if e.returncode != 0 and 'Exiting as not all patches were successful!' in e.stderr.splitlines(
                )[-1]:
                    resolver = PatchFailureResolver(self.target_version,
                                                    self.working_version)
                    resolver.apply_patches_3way(launch_vscode)
                    if resolver.requires_conflict_resolution() is True:
                        # Manual resolution required.
                        console.log(
                            f'👋 (Address all sections with {ACTION_NEEDED_DECORATOR} above, and then rerun [italic]🚀Brockit![/] with [bold cyan]--continue[/])'
                        )
                        return False

                    if self._run_update_patches_with_no_deletions(
                    ) is not True:
                        return False

                    resolver.stage_all_patches()
                    self._save_conflict_resolved_patches()

                    # With all conflicts resolved, it is necessary to close the
                    # upgrade with all the same steps produced when running an
                    # upgrade continuation, as recovering from a conflict-
                    # resolution failure.
                    override_continuation = True
                elif e.returncode != 0:
                    logging.error(
                        'Failures found when running npm run init\n%s',
                        e.stderr)
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

            if _has_staged_changed() is False:
                logging.error(
                    'Nothing has been staged to commit conflict-resolved patches.'
                )
                return False

            self._save_conflict_resolved_patches()

        self._save_updated_patches()
        if is_continuation is True or override_continuation is True:
            # Run init again to make sure nothing is missing after updating patches.
            terminal.run_npm_command('init')

        terminal.run_npm_command('chromium_rebase_l10n')
        self._save_rebased_l10n()

        if with_github is True:
            self.create_or_updade_issue_with_gh()

        return True

    def run(self,
            is_continuation,
            no_conflict_continuation,
            launch_vscode=False,
            with_github=False):
        """Run the upgrade process

    Check `_run_internal` for details.

    Return:
      Return True if completed entirely, and False otherwise.
    """
        with console.status("Upgrading Chromium base version") as status:
            terminal.set_status_object(status)

            if is_continuation is True:
                # Update the working version from the continuation file.
                resolver = PatchFailureResolver(self.target_version)
                resolver.load_continuation_file()
                self.working_version = resolver.working_version

                if self.target_version != self.chromium_src_version:
                    logging.error(
                        'To run with [bold cyan]--continue[/] the Chromium version has to '
                        'be in Sync with Brave. Brave %s ➜ Chromium %s',
                        self.target_version, self.chromium_src_version)
                    return False
            else:
                self.working_version = _get_chromium_version_from_git('HEAD')

                if self.working_version != self.chromium_src_version and self.target_version != self.chromium_src_version:
                    logging.error(
                        'Chrommium seems to be synced to a version entirely '
                        'unrelated. Brave %s ➜ Chromium %s',
                        self.working_version, self.chromium_src_version)
                    return False
                if self.working_version != self.chromium_src_version:
                    logging.warning(
                        'Chromium is checked out with the target version. '
                        'Brave %s ➜ Chromium %s', self.working_version,
                        self.chromium_src_version)

            if self.working_version != self.base_version:
                terminal.log_task(
                    'Chromium changes since last branch upgrade: '
                    f'{GOOGLESOURCE_LINK.format(from_version=self.working_version, to_version=self.target_version)}'
                )
            terminal.log_task(
                'All Chromium changes: '
                f'{GOOGLESOURCE_LINK.format(from_version=self.base_version, to_version=self.target_version)}'
            )

            return self._run_internal(is_continuation,
                                      no_conflict_continuation, launch_vscode,
                                      with_github)

    def generate_patches(self):
        with console.status(
                "Working to update patches and strings...") as status:
            terminal.set_status_object(status)
            terminal.run_npm_command('init')
            terminal.run_npm_command('update_patches')
            self._save_updated_patches()
            terminal.run_npm_command('chromium_rebase_l10n')
            self._save_rebased_l10n()

    def run_github_issue_only(self):
        with console.status("Creating/Updating GitHub issue...") as status:
            terminal.set_status_object(status)
            if _is_gh_cli_logged_in() is False:
                logging.error('GitHub CLI is not logged in.')
                sys.exit(1)

            self.create_or_updade_issue_with_gh()

def main():
    parser = argparse.ArgumentParser(
        # Custom formatter to preserve line breaks in the docstring
        formatter_class=RichHelpFormatter)

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
        help='Regenerates the "Update patches" and "Updated strings" commits for'
        'current version.',
        dest='update_patches_only')
    parser.add_argument(
        '--with-github',
        action='store_true',
        help='Indicates that GitHub tasks should be carried out as part of the'
        'of the upgrade run (e.g create/update GitHub issues).',
        dest='with_github')
    parser.add_argument(
        '--github-issue-only',
        action='store_true',
        help=
        'Creates or updates the GitHub issue based on the version details provided.',
        dest='github_issue_only')
    parser.add_argument(
        '--vscode',
        action='store_true',
        help=
        'Launches vscode with the files that need manual conflict resolution.')
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Produces verbose logs (full command lines being executed, etc).')

    args = parser.parse_args()
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format='%(message)s',
        handlers=[IncendiaryErrorHandler(markup=True, rich_tracebacks=True)])

    upgrade = Upgrade(args.previous, args.to)

    console.log('[italic]🚀 Brockit!')
    if args.update_patches_only:
        upgrade.generate_patches()
    elif args.github_issue_only:
        upgrade.run_github_issue_only()
    elif upgrade.run(args.is_continuation, args.no_conflict, args.vscode,
                     args.with_github) is False:
        return 1

    console.log('[bold]💥 Done!')

    return 0


if __name__ == '__main__':
    sys.exit(main())
