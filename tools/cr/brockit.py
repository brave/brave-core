#!/usr/bin/env vpython3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""# 🚀Brockit! Guide

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

### `brockit.py lift`
This is *🚀Brockit!* (Brave Rocket? Brave Rock it? Broke it?): a tool to help
upgrade Brave to a newer Chromium base version. The main goal is to produce
use it to commit the following changes:

 * Update from Chromium [from] to [to].
 * Conflict-resolved patches from Chromium [from] to [to].
 * Update patches from Chromium [from] to [to].
 * Updated strings for Chromium [to].

To start it off, provide a `--to` target version, and a base branch to be used,
either by providing a `--from-ref` argument, or by having an upstream branch to
to your current branch.

```sh
tools/cr/brockit.py lift --to=135.0.7037.1 --from-ref=origin/master
```

When using `--from-ref`, any valid git reference can be used, such as a branch,
or even hashes. Additionally there are special tags that can be passed to this
flag.

 * `--from-ref=@upstream`: This will use the upstream branch as the base for
    the lift. This requires the user to set an upstream branch.
 * `--from-ref=@previous`: This tag means the previous version since the last
    upgrade in the current branch. This is useful when telling brockit that you
    are doing a minor version bump.
 * `--from-ref=@previous-major`: This tag means the reference the parent commit
    for the current major. This is useful when doing rebases, and wanting to
    select from the version change.

Using upstream:

```sh
git branch --set-upstream-to=origin/master
tools/cr/brockit.py lift --to=135.0.7037.1
```

The `--to` flag also provides special flags:
 * `--to=@latest-canary`: This will use the latest canary version as per
    Chromium Dash. For this particular flag, both the *Canary*, and the
    *Canary (DCHECK)* channels will be queried for the latest.
 * `--to=@latest-dev`, and `--to=@latest-beta`: Same as the falg above, but
    to these respective channels.

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
6. Having resolved all conflicts. Restart *🚀Brockit!* with `--continue` and
   other similar arguments you may want to keep (e.g. `--vscode`).
7. *🚀Brockit!* will pick up from where it stopped, possibly running
  `npm run update_patches`, staging all patches, and committing the under
  *Conflict-resolved patches from Chromium [from] to [to].*
8. *Update patches from Chromium [from] to [to]* will be committed.
9. *Updated strings for Chromium [to]* will be committed.

Steps 3-7 may end up being skipped altogether if no failures take place, or in
part if resolution is possible without manual intervention.

The `--restart` flag can be used to start the process from scratch, discarding
everything committed in the last run.

```sh
tools/cr/brockit.py lift --to=135.0.7037.1 --from-ref=origin/master --restart
```

### `brockit.py regen`
Additionally, *🚀Brockit!* can be run with `regen`. This is useful to generate
the "Update patches" and "Updated strings" commits on their own when rebasing
branches, regenerating these files as desired.

```sh
tools/cr/brockit.py update-version-issue
````

### `brockit.py update-version-issue`
The `lift` command supports the use of `--with-github`, which either creates a
new GitHub issue or updates an existing one with the details of the run.
However it is also possible to run this task on standalone as well.

```sh
tools/cr/brockit.py update-version-issue
````

Different from a lift using `--with-github`, the command above will
also attempt to create a PR, if none exists, however it is necessary that the
current branch has an upstream branch set, as creating a PR involves having an
upstream branch.

### Infra mode
When running on infra, the `--infra-mode` flag should be provided. This will
suppress all status updates, and rather provide a keep-alive type of feedback
to make sure that the CI doesn't time out.

### `brockit.py rebase`
This command is provide for two purposes: to generally rebase the current
branch before doing a small lift, and to provide a standard way to recommit
all changes in the branch, which is useful when it is necessary to separate the
committing stage from the signing stage.

A simple rebase should look like this:
```sh
script/version_up.py rebase
```

To rebase a lift of a major bump, you can either set the upstream for your
local branch and rely on the defaults, or pass `--from-ref`/`--to-ref`
yourself.

```sh
script/version_up.py rebase --from-ref=@previous-major --to-ref=origin/master
```

To recommit everything while rebase you can do:
```sh
script/version_up.py rebase --from-ref=origin/master --recommit
```

For convenience, `--discard-regen-change` is provided to discard the types of
changes that are supposed to be regenerated during the next lift (e.g.
"Update patches").

This command uses `--interactive` under the hood, so it may require extra steps
from the user to complete.

Furthermore, there is also `--squash-minor-bumps`, which can be used to squash
away all minor bumps in a branch down to a single one. This is useful when
when running a cr branch, as it is common to have multiple minor daily bumps
in a branch.
"""

import argparse
from dataclasses import dataclass, field, replace
from datetime import datetime
import json
import logging
import os
from pathlib import Path, PurePath
import pickle
import platform
import re
import requests
from rich.markdown import Markdown
from rich.padding import Padding
import subprocess
import sys
from typing import Optional, List, Dict

from incendiary_error_handler import IncendiaryErrorHandler
from git_status import GitStatus
from patchfile import Patchfile
import repository
from repository import Repository, CHROMIUM_SRC_PATH
from terminal import console, terminal
import versioning
from versioning import Version

# This file is updated whenever the version number is updated in package.json
PINSLIST_TIMESTAMP_FILE = (
    'chromium_src/net/tools/transport_security_state_generator/'
    'input_file_parsers.cc')
VERSION_UPGRADE_FILE = Path('.version_upgrade')

# The link to a specific commit in the Chromium source code.
GOOGLESOURCE_COMMIT_LINK = f'{versioning.GOOGLESOURCE_LINK}' '/+/{commit}'

# A basic url to the rust toolchain that can be used to check for the toolchain
# availability for a given version.
RUST_TOOLCHAIN_URL = 'https://brave-build-deps-public.s3.brave.com/rust-toolchain-aux/linux-x64-rust-toolchain-{revision}.tar.xz'

# Google dash link used to check the latest version for a given channel
CHROMIUMDASH_LATEST_RELEASE = 'https://chromiumdash.appspot.com/fetch_releases?channel={channel}&platform=Windows&num=1'

# A decorator to be shown for messages that the user should address before
# continuing.
ACTION_NEEDED_DECORATOR = '[bold yellow](action needed)[/]'

# The text for the body used on a GitHub issue for a version bump.
MINOR_VERSION_BUMP_ISSUE_TEMPLATE = """### Minor Chromium bump

{googlesource_log_link}

### QA tests

- Check branding items
- Check for version bump

### Minor Chromium bump

- No specific code changes in Brave (only line number changes in patches)
"""


def _get_current_branch_upstream_name() -> Optional[str]:
    """Retrieves the name of the current branch's upstream.
    """
    try:
        return repository.brave.run_git('rev-parse', '--abbrev-ref',
                                        '--symbolic-full-name', '@{upstream}')
    except subprocess.CalledProcessError:
        return None


def _update_pinslist_timestamp() -> str:
    """Updates the pinslist timestamp in the input_file_parsers.cc file for the
    version commit.

    Returns:
        The readable timestamp of the update into the file.
    """
    content = repository.brave.read_file(PINSLIST_TIMESTAMP_FILE)

    pattern = r"# Last updated:.*\nPinsListTimestamp\n[0-9]{10}\n"
    match = re.search(pattern, content, flags=re.DOTALL)
    if not match:
        logging.error(
            'Expected pattern for PinsListTimestamp block not found. '
            'Aborting.')
        sys.exit(1)

    # Update the timestamp
    timestamp = int(datetime.now().timestamp())
    readable_timestamp = datetime.fromtimestamp(timestamp).strftime(
        '%a %b %d %H:%M:%S %Y')
    updated_content = re.sub(
        pattern,
        (f'# Last updated: {readable_timestamp}\nPinsListTimestamp\n'
         f'{timestamp}\n'),
        content,
        flags=re.DOTALL,
    )

    # Write back to the file
    with open(PINSLIST_TIMESTAMP_FILE, "w", encoding="utf-8") as file:
        file.write(updated_content)

    updated = repository.brave.run_git('diff', PINSLIST_TIMESTAMP_FILE)
    if updated == '':
        raise ValueError('Pinslist timestamp failed to update.')

    return readable_timestamp


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


@dataclass(frozen=True)
class ApplyPatchesRecord:
    """A class to hold the continuation data for patches.
    """

    # A dictionary of all patches with attempted `--3way`, grouped by
    # repository.
    patch_files: dict[Repository, Patchfile] = field(default_factory=dict)

    # A list of patches that cannot be applied due to their source file being
    # deleted.
    patches_to_deleted_files: list[Patchfile] = field(default_factory=list)

    # A list of files that require manual conflict resolution before continuing.
    files_with_conflicts: list[str] = field(default_factory=list)

    # A list of patches that fail entirely when running apply with `--3way`.
    broken_patches: list[Patchfile] = field(default_factory=list)

    def requires_conflict_resolution(self):
        """Checks if there are any patches that require manual conflict
        resolution.

        This function is used to determine it is necessary to stop the process
        to address any potential patch changes.
        """
        return (self.files_with_conflicts or self.patches_to_deleted_files
                or self.broken_patches)

    def stage_all_patches(self, ignore_deleted_files=False):
        """Stages all patches that were applied, so they can be committed as
        conflict-resolved patches.

        Args:
            ignore_deleted_files:
                If set to True, deleted files will be ignored, and not staged.
        """
        for _, patches in self.patch_files.items():
            for patch in patches:
                if (ignore_deleted_files and not Path(patch.path).exists()):
                    # Skip deleted files.
                    continue

                repository.brave.run_git('add', patch.path)


@dataclass(frozen=True)
class ContinuationFile:
    """A class to hold the continuation data for the upgrade process.
    """

    # The target version that brockit is aiming to upgrade brave to.
    target_version: Version

    # The version that was in the branch when the upgrade started (which can be
    # different from the base version).
    working_version: Version

    # The base version for the upgrade process. This is saved as a reference
    # like @previous cannot be relied on once we committed a change, moving the
    # previous branch ahead.
    base_version: Version

    # This flag indicates that the prerun adivisories have been shown to the
    # user.
    has_shown_advisory: bool = False

    # The continuation data for the patches.
    apply_record: Optional[ApplyPatchesRecord] = field(default=None)

    @staticmethod
    def load(target_version: Version,
             working_version: Optional[Version] = None,
             check: bool = True) -> Optional["ContinuationFile"]:
        """Loads the continuation file.

        This function loads the continuation file, and returns the instance of
        the continuation file, or None if the file does not exist.

        Args:
            target_version:
                The target version for the upgrade process. Used to validate
                the continuation file being loaded.
            working_version:
                The working version in the branch, used to validate the
                continuation file being loaded.
            check:
                If the function should raise an error if the file does not
                exist.
        """
        if not VERSION_UPGRADE_FILE.exists():
            if check:
                raise FileNotFoundError(
                    f'File {VERSION_UPGRADE_FILE} does not exist')
            return None

        with open(VERSION_UPGRADE_FILE, 'rb') as file:
            continuation = pickle.load(file)

        if (continuation.target_version != target_version
                or (working_version is not None
                    and continuation.working_version != working_version)):
            if not check:
                return None

            # This validation is in place for something that shouldn't happen.
            # If this is being hit, it means some wrong continuation file is in
            # the tree, and the process should be started all over.
            raise TypeError(
                'Trying to load a continuation file from another run. Target '
                f'verison:{continuation.target_version}, Working '
                f'version:{continuation.working_version}')

        return continuation

    def save(self):
        """Saves the continuation file.
        """
        with open(VERSION_UPGRADE_FILE, 'wb') as file:
            pickle.dump(self, file)

    @staticmethod
    def clear():
        logging.debug('Clearing the continuation file.')
        try:
            Path(VERSION_UPGRADE_FILE).unlink()
        except FileNotFoundError:
            pass

class Task:
    """ Base class for all tasks in brockit.

    This class provides a common interface for other tasks to build upon. It
    provides a run method that will execute the task, and a status_message
    method that will return a string to be displayed while the task is running.
    """

    def run(self, *cmd) -> bool:
        """Runs the task with a status message.

        This function will run the task inside the scope of a status message.

        Args:
            an open set of argument to be passed along to the derived class's
            execute method.

        Returns:
            Return 1 if the task failed, 0 if the task succeeded. This is used
            as the process' exit code.
        """
        console.log('[italic]🚀 Brockit!')
        with terminal.with_status(self.status_message()):
            result = self.execute(*cmd)

            if result:
                console.log('[bold]💥 Done!')

        return result == False

    def status_message(self) -> str:
        """Returns a status message for the task.

        This function has to be implemented by the derived class.
        """
        raise NotImplementedError

    def execute(self) -> bool:
        """Executes the task.

        This function has to be implemented by the derived class.
        """
        raise NotImplementedError


class Versioned(Task):
    """ Base class for all versioned tasks.

    Versioned tasks are tasks that have the concept of a base version and a
    target version.
    """

    def __init__(self,
                 base_version: Version,
                 target_version: Optional[Version] = None):
        # The version in `package.json` found in that upstream branch.
        self.base_version = base_version

        # The target of a given upgrade.
        self.target_version = target_version
        if self.target_version is None:
            # When not provided we default for whatever is in the current
            # branch because that's is possibly what the target version is for
            # maintainance tasks.
            self.target_version = Version.from_git('HEAD')

        if self.target_version <= self.base_version:
            logging.error(
                'Target version is not higher than base version: '
                'target=%s, base=%s', self.target_version, self.base_version)
            sys.exit(1)

    def execute(self) -> bool:
        raise NotImplementedError

    def _save_updated_patches(self):
        """Creates the updated patches change

    This function creates the third commit in the order of the update, saving
    all patches that might have been changed or deleted. Untracked patches are
    excluded from addition at this stage.
    """
        repository.brave.run_git('add', '-u', '*.patch')

        repository.brave.git_commit(
            f'Update patches from Chromium {self.base_version} '
            f'to Chromium {self.target_version}.')

    def _save_rebased_l10n(self):
        """Creates string rebase change

    This function stages, and commits, all changed, updated, or deleted files
    resulting from running npm run chromium_rebase_l10n.
    """
        repository.brave.run_git('add', '*.grd', '*.grdp', '*.xtb')
        repository.brave.git_commit(
            f'Updated strings for Chromium {self.target_version}.')


class Regen(Versioned):
    """Regenerates patches and strings for the current branch.

    This task is used for cases where the user wants to regenerate patches and
    strings. The purpose is to produce `Update patches` and `Updated strings`
    where approrpriate.
    """

    def status_message(self):
        return "Updating patches and strings..."

    def execute(self) -> bool:
        terminal.log_task(
            f'Processing changes for Chromium {self.base_version} '
            f'to Chromium {self.target_version}.')
        terminal.run_npm_command('init')
        terminal.run_npm_command('update_patches')
        self._save_updated_patches()
        terminal.run_npm_command('chromium_rebase_l10n')
        self._save_rebased_l10n()
        return True


class GitHubIssue(Versioned):
    """Creates a GitHub issue for the upgrade.

    This class offers ways to create or update the github issue for the
    upgrade. Also, as this is its own task, it can be called on its own for
    maintainance purposes.
    """

    def status_message(self):
        return "Creating/Updating GitHub issue for upgrade..."

    def compose_issue_title(self):
        """Generates the title for the upgrade issue.

        This function generates the title for the upgrade issue, based on the
        base and target version. The title is generated in a way that it can be
        used for both major and minor upgrades.

        This title is the same used for the push request.
        """
        title = 'Upgrade from Chromium {previous} to Chromium {to}'
        if self.target_version.major > self.base_version.major:
            # For major updates, the issue description doesn't have a precise
            # version number.
            title = title.format(previous=str(self.base_version.major),
                                 to=str(self.target_version.major))
        else:
            title = title.format(previous=str(self.base_version),
                                 to=str(self.target_version))
        return title

    def lookup_issue(self, title: str) -> Optional[List]:
        """Looks up the issue for the upgrade.

        This function checks if there's already an issue with the title
        provided, and returns its details.
        """
        results = json.loads(
            terminal.run([
                'gh', 'issue', 'list', '--repo', 'brave/brave-browser',
                '--search', title, '--state', 'open', '--json',
                'number,title,url,body'
            ]).stdout.strip())
        return next((entry for entry in results if entry['title'] == title),
                    None)

    def create_push_request(self, issue_url: str) -> bool:
        """Creates a push request for the current branch.

        This function creates a push request for the upgrade.
        """
        current_branch = repository.brave.current_branch()
        if current_branch == 'HEAD':
            logging.error('Cannot create a push request: Not in a branch')
            return False

        # It is assumed that the upstream branch is the base branch for the PR.
        upstream_branch = _get_current_branch_upstream_name()
        if upstream_branch is None:
            logging.error(
                'Cannot create a push request: Not in a branch with an upstream'
            )
            return False
        if '/' in upstream_branch:
            # removing the remote portion.
            upstream_branch = upstream_branch.split("/", 1)[-1]

        results = json.loads(
            terminal.run([
                'gh', 'pr', 'list', '--head', current_branch,
                '--json=number,url'
            ]).stdout.strip())
        if results:
            console.log(
                f'The push request for {current_branch} is: {results[0]["url"]}'
            )
            return True

        # That's a naive way to think about this, but in general the uplift
        # branches are upstreamed to 1.77.x, 1.78.x, etc.
        is_uplift = (versioning.get_uplift_branch_name_from_package() ==
                     upstream_branch)
        tag = f'[{upstream_branch}] ' if is_uplift else ''
        cmd = [
            'gh', 'pr', 'create', '--base', upstream_branch, '--head',
            current_branch, '--title', f'{tag}{self.compose_issue_title()}',
            '--body', f'Resolves {issue_url}', '--label',
            '"CI/run-audit-deps"', '--label', '"CI/run-network-audit"',
            '--assignee', 'cdesouza-chromium'
        ]
        # Emerick and Alexey take care of even-numbered major versions, while
        # Max and Sam do the odd ones.
        if self.target_version.major % 2 == 0:
            cmd += ['--assignee', 'emerick', '--assignee', 'AlexeyBarabash']
        else:
            cmd += ['--assignee', 'mkarolin', '--assignee', 'samartnik']

        is_major = self.target_version.major > self.base_version.major

        if is_major or upstream_branch == 'master':
            # It is not common for upstream test to be run on uplifts of minor
            # upgrades.
            cmd += ['--label', '"CI/run-upstream-tests"']

        if is_major:
            cmd += ['--label', '"CI/storybook-url"']

            # Always create PR for major version upgrades as draft
            cmd.append('--draft')

        try:
            pr_url = terminal.run(cmd).stdout.strip()
            terminal.log_task(f'GitHub PR created for this bump: {pr_url}')
        except subprocess.CalledProcessError as e:
            console.log(
                f'Failed to create PR for {current_branch}: {e.stderr.strip()}'
            )
            return False

        if is_uplift:
            # Only uplift branches set milestones.
            results = json.loads(
                terminal.run([
                    'gh', 'api', 'repos/brave/brave-core/milestones', '--jq',
                    '[.[] | {number, title}]'
                ]).stdout)
            if not results:
                logging.warning('No milestones returned for brave-core')
                return False

            milestone = next(
                (entry["number"] for entry in results
                 if entry["title"].startswith(f'{upstream_branch} - ')), None)
            if milestone is None:
                logging.warning('Failed to find milestone for branch %s',
                                upstream_branch)
                return False

            pr_number = pr_url.split("/", 1)[-1]
            terminal.run([
                'gh', 'api', '-X', 'PATCH',
                f'repos/brave/brave-core/issues/{pr_number}', '-F',
                f'milestone={milestone}'
            ])

        return True

    def create_or_update_version_issue(self, with_pr: bool) -> bool:
        """Creates a github issue for the upgrade.

        This function creates/updates the upgrade github issue.
        """
        link = self.target_version.get_googlesource_diff_link(
            from_version=str(self.base_version))

        title = self.compose_issue_title()
        issue = self.lookup_issue(title)
        if issue is not None:
            pattern = r"https://chromium\.googlesource\.com/chromium/src/\+log/[^\s]+"
            body = re.sub(pattern, link, issue['body'])
            if body == issue['body']:
                console.log(
                    f'A Github issue with the title "{title}" is already '
                    f'created and up-to-date. {str(issue["url"])}')
            else:
                terminal.run([
                    'gh', 'issue', 'edit',
                    str(issue['number']), '--repo', 'brave/brave-browser',
                    '--body', f'{body}'
                ])
                terminal.log_task(f'GitHub issue updated {str(issue["url"])}.')
            if with_pr:
                return self.create_push_request(issue["url"])
            return True

        body = MINOR_VERSION_BUMP_ISSUE_TEMPLATE.format(
            googlesource_log_link=link)
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

        if with_pr:
            return self.create_push_request(issue_url)
        return True

    def execute(self) -> bool:
        if not _is_gh_cli_logged_in():
            logging.error('GitHub CLI is not logged in.')
            return False

        return self.create_or_update_version_issue(with_pr=True)


class ReUpgrade(Task):
    """Restarts the upgrade process.

    This class is called when `--restart` is provided and it is useful when one
    wants to restart fresh. It will reset the repository to where it was before
    the update started.
    """

    def __init__(self, target_version: Version):
        # The target version passed to --to, which is used to make sure the
        # restart is being done in the right place.
        self.target_version = target_version

    def status_message(self):
        return "Restarting the upgrade process..."

    def execute(self) -> bool:
        """Restarts the upgrade process.

        This function will reset the repository to the state it was before the
        upgrade started. It will also clear the continuation file.
        """
        working_version = Version.from_git('HEAD')
        if self.target_version != working_version:
            logging.error(
                'Running with `--restart` but the target version does not '
                'match the current version. %s vs %s', self.target_version,
                working_version)
            return False

        starting_change = repository.brave.last_changed(
            PINSLIST_TIMESTAMP_FILE)
        commit_message = repository.brave.get_commit_short_description(
            starting_change)
        if not commit_message.startswith(
                'Update from Chromium ') or not commit_message.endswith(
                    f' to Chromium {self.target_version}.'):
            logging.error(
                'Running with `--restart` but the last change does match the '
                'arguments provide. %s %s', starting_change, commit_message)
            return False

        console.log('Discarding the following changes:')
        console.log(
            Padding(
                '[dim]%s' % repository.brave.run_git(
                    'log', '--pretty=%h %s', f'HEAD...{starting_change}~1'),
                (0, 4)))

        ContinuationFile.clear()
        repository.brave.run_git('reset', '--hard', f'{starting_change}~1')
        return True


class Upgrade(Versioned):
    """The upgrade process, holding the data related to the upgrade.

  This class produces an object that is reponsible for keeping track of the
  upgrade process step-by-step. It acquires all the common data necessary for
  its completion.
  """

    def __init__(self,
                 target_version: Version,
                 is_continuation: bool,
                 base_version: Optional[Version] = None):
        if ((base_version is None and not is_continuation)
                or (base_version is not None and is_continuation)):
            # either it is a new upgrade, and a base version is provided, or it
            # is a continuation and no base version is provided as it gets read
            # from disk.
            raise NotImplementedError()

        # Indicates that the upgrade is a continuation from a previous run.
        self.is_continuation = is_continuation

        # The last version the branch was in, that the update is being started
        # from
        self.working_version = None
        if self.is_continuation:
            version_on_head = Version.from_git('HEAD')
            if target_version != version_on_head:
                logging.error(
                    'Running with `--continue` on a branch with a different '
                    'version what the target should be. %s vs %s',
                    target_version, version_on_head)
                sys.exit(1)

            # Loads the working version from the continuation file, because the
            # current branch has already updated the working version to the
            # target version.
            try:
                continuation = ContinuationFile.load(
                    target_version=target_version)
            except FileNotFoundError:
                logging.error(
                    '%s continuation file does not exist. (Are you sure you '
                    'meant to pass [bold cyan]--continue[/]?)',
                    VERSION_UPGRADE_FILE)
                sys.exit(1)
            self.working_version = continuation.working_version
            base_version = continuation.base_version
        else:
            self.working_version = Version.from_git('HEAD')

        # The version currently set in the VERSION file.
        self.chromium_src_version = versioning.read_chromium_version_file()

        super().__init__(base_version, target_version)

    def status_message(self):
        return "Upgrading Chromium base version"

    def apply_patches_3way(self,
                           launch_vscode: bool = False) -> ApplyPatchesRecord:
        """Applies patches that have failed using the --3way option to allow for
        manual conflict resolution.

        This method will apply the patches and reset the state of applied
        patches. Additionally, it will also produce a list of the files that
        are waiting for conflict resolution.

        A list of the patches applied will be produced as well.
        """
        # A dictionary that holds a list for all patch files affected, by
        # repository.
        patch_files = {}

        # This is a flat of list of patches that cannot be applied due to their
        # source file being deleted.
        patches_to_deleted_files = []

        # A list of files that require manual conflict resolution before
        # continuing.
        files_with_conflicts = []

        # These are patches that fail entirely when running apply with `--3way`.
        broken_patches = []

        if patch_files:
            raise NotImplementedError(
                'unreachable: 3way apply should happen only once.')

        # the raw list of patches that failed to apply.
        patch_failures = _get_apply_patches_list()
        if patch_failures is None:
            raise ValueError(
                'Apply patches failed to provide a list of patches.')

        for entry in patch_failures:
            patch = Patchfile(path=PurePath(entry['patchPath']),
                              provided_source=entry.get('path'))

            if entry['reason'] == 'SRC_REMOVED':
                # Skip patches that can't apply as the source is gone.
                patches_to_deleted_files.append(patch)
                continue

            # Grouping patch files by their repositories, so to allow us to
            # iterate through them, applying them in their repo paths.
            patch_files.setdefault(patch.repository, []).append(patch)

        if patch_files:
            terminal.log_task(
                '[bold]Reapplying patch files with --3way:\n[/]%s' %
                '\n'.join(f'    * {file}' for file in [
                    patch.path for patch_list in patch_files.values()
                    for patch in patch_list
                ]))

        vscode_args = ['code']
        for repo, patches in patch_files.items():
            for patch in patches:
                apply_result = patch.apply()
                if apply_result.status == Patchfile.ApplyStatus.CONFLICT:
                    source = patch.source_from_brave()
                    files_with_conflicts.append(source)
                elif apply_result.status == Patchfile.ApplyStatus.BROKEN:
                    broken_patches.append(patch)
                elif apply_result.status == Patchfile.ApplyStatus.DELETED:
                    patches_to_deleted_files.append(patch)

        for repo, patches in patch_files.items():
            # Resetting any staged states from apply patches as that can cause
            # issues when generating patches.
            repo.unstage_all_changes()

        if patches_to_deleted_files:
            # The goal in this this section is print a report for listing every
            # patch that cannot apply anymore because the source file is gone,
            # fetching from git the commit and the reason why exactly the file
            # is not there anymore (e.g. renamed, deleted).

            terminal.log_task('[bold]Files that cannot be patched anymore '
                              f'{ACTION_NEEDED_DECORATOR}:[/]')

            # This set will hold the information about the deleted patches
            # in a way that they can be grouped around the chang that caused
            # their removal.
            deletion_report = {}

            for patch in patches_to_deleted_files:
                # Make sure we have an correct file source for the patch.
                patch = patch.fetch_source_from_git()
                # Finding the culptrit commit hash.
                commit = patch.get_last_commit_for_source()
                deletion_report.setdefault(
                    commit,
                    {})[patch] = patch.get_source_removal_status(commit)

            for commit, patches in deletion_report.items():
                for patch, status in patches.items():
                    if status.status == 'D':
                        console.log(
                            Padding(f'✘ {patch.source()} [red bold](deleted)',
                                    (0, 4)))
                        vscode_args.append(patch.path)
                    elif status.status == 'R':
                        renamed_to = patch.repository.from_brave(
                        ) / status.renamed_to
                        console.log(
                            Padding(
                                f'✘ {patch.source_from_brave()}\n    '
                                f'([yellow bold]renamed to[/] {renamed_to})',
                                (0, 4)))
                        vscode_args += [patch.path, renamed_to]

            # Printing the commmit message for the grouped changes.
            console.log(
                Padding(f'{next(iter(patches.items()))[1].commit_details}\n',
                        (0, 8),
                        style="dim"))

        if broken_patches:
            terminal.log_task(
                '[bold]Broken patches that fail to apply entirely '
                f'{ACTION_NEEDED_DECORATOR}:[/]')

            for patch in broken_patches:
                source = patch.source_from_brave()
                console.log(Padding(f'✘ {patch.path} ➜ {source}', (0, 4)))
                vscode_args += [patch.path, source]

        if files_with_conflicts:
            vscode_args += files_with_conflicts
            file_list = '\n'.join(f'    ✘ {file}'
                                  for file in files_with_conflicts)
            terminal.log_task(f'[bold]Manually resolve conflicts for '
                              f'{ACTION_NEEDED_DECORATOR}:[/]\n{file_list}')

        if launch_vscode and len(vscode_args) > 1:
            try:
                terminal.run(vscode_args)
            except subprocess.CalledProcessError as e:
                logging.error(
                    'Failed to launch VSCode with the following args: %s\n%s',
                    ' '.join(vscode_args), e.stderr)

        # The continuation file is updated at the end of the process, in case
        # the process has to be continued later.
        apply_record = ApplyPatchesRecord(
            patch_files=patch_files,
            patches_to_deleted_files=patches_to_deleted_files,
            files_with_conflicts=files_with_conflicts,
            broken_patches=broken_patches)

        replace(ContinuationFile.load(target_version=self.target_version),
                apply_record=apply_record).save()

        return apply_record

    def _update_package_version(self):
        """Creates the change upgrading the Chromium version

    This is for the creation of the first commit, which means updating
    package.json to the target version provided, and commiting the change to
    the repo
    """
        package = versioning.load_package_file('HEAD')
        package['config']['projects']['chrome']['tag'] = str(
            self.target_version)
        with open(versioning.PACKAGE_FILE, "w") as package_file:
            json.dump(package, package_file, indent=2)

        repository.brave.run_git('add', versioning.PACKAGE_FILE)

        # Pinlist timestamp update occurs with the package version update.
        _update_pinslist_timestamp()
        repository.brave.run_git('add', PINSLIST_TIMESTAMP_FILE)
        repository.brave.git_commit(
            f'Update from Chromium {self.base_version} '
            f'to Chromium {self.target_version}.')

    def _save_conflict_resolved_patches(self):
        repository.brave.git_commit(
            f'Conflict-resolved patches from Chromium {self.base_version} to '
            f'Chromium {self.target_version}.')

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
        if status.has_deleted_patch_files():
            logging.error(
                'Deleted patches detected. These should be committed as their '
                'own changes:\n%s' % '\n'.join(status.deleted))
            return False
        if status.has_untracked_patch_files():
            logging.error(
                'Untracked patch files detected. These should be committed as '
                'their own changes:\n%s' % '\n'.join(status.untracked))
            return False

        return True

    def look_for_diffs(self, *files) -> str:
        """Return the diffs for the files provided for this upgrade.

    Checks for diffs of the files provided in range of the changes for the
    current upgrade. Returns an empty string if no diffs are found.
        """
        return repository.chromium.run_git('diff', self.working_version,
                                           self.target_version, '--', *files)

    def get_assigned_value(self,
                           contents: str,
                           lookup: str,
                           added: bool = False,
                           removed: bool = False) -> Optional[str]:
        """Uses a basic regex to extract the value being assinged into a key.

    This function is useful to extract the value of a key from a file contents
    that is being diffed or read in general.

    Args:
        contents:
            The contents of the file to look for the key.
        lookup:
            The key to look for in the contents.
        added:
            If set to True, looks for the key starting with +.
        removed:
            If set to True, looks for the key starting with -.
    Returns:
        The value assigned to the key, or None if not found
        """
        for line in contents.splitlines():
            # Discad any comment that there may be in the line
            line = line.split('#', 1)[0].strip()

            if lookup not in line:
                continue
            if added and not line.startswith('+'):
                continue
            if removed and not line.startswith('-'):
                continue

            # remove the sign at the beggining of the line, if that's the case
            # we are looking for.
            if added or removed:
                line = line[1:]

            if '=' not in line:
                continue

            [key, value] = line.split('=', 1)
            if key.strip().lstrip() == lookup:
                return value.strip().lstrip().replace('"', '').replace("'", "")
        return None

    def _check_toolchain(self, file_path: str, key: str) -> Optional[Dict]:
        """ Helper function to check for toolchain updates.

    This helper is used to check for the MacOS SDK, Windows SDK to see if the
    toolchain for their respective OSes need updateing. It looks for diff of a
    given file to find if a certain key has been updade in-between the working
    and target version.

    Args:
        file_path:
            The file to look for the key in.
        key:
            The key to look for in the file.
    Returns:
        A dictionary with a summary of the SDK upgrade.
        """
        toolchain_diff = self.look_for_diffs(file_path)
        if key not in toolchain_diff:
            return None

        updated_value = self.get_assigned_value(toolchain_diff,
                                                key,
                                                added=True)
        current_value = self.get_assigned_value(toolchain_diff,
                                                key,
                                                removed=True)
        commit_log = repository.chromium.run_git(
            'log', f'{self.working_version}..{self.target_version}', '-S',
            updated_value, '--pretty=oneline', '-1', file_path)
        commit_hash, commit_message = commit_log[:40], commit_log[41:]

        return {
            'current': current_value,
            'target': updated_value,
            'commit': {
                'hash': commit_hash,
                'message': commit_message
            }
        }

    def _check_win_toolchain(self) -> Optional[Dict]:
        """Check for Windows toolchain updates.

    This function returns an advisory record if the Windows SDK has been
    updated, indicating a new toolchain is required.
        """
        result = self._check_toolchain('build/vs_toolchain.py', 'SDK_VERSION')
        if result:
            result['description'] = (
                'Windows SDK has been updated. '
                f'{result["current"]} ➜ {result["target"]}')
            result['advice'] = (
                'Contact DevOps regarding the new WinSDK for the hermetic '
                'toolchain. Update `env.GYP_MSVS_HASH_*` in '
                'build/commands/lib/config.js with correct hashes.')
        return result

    def _check_mac_toolchain(self) -> Optional[Dict]:
        """Check for MacOS toolchain updates.

    This function returns an advisory record if the MacoOS SDK has been updated,
    indicating a new toolchain is required.
        """
        result = self._check_toolchain('build/config/mac/mac_sdk.gni',
                                       'mac_sdk_official_version')
        if result:
            result['description'] = (
                'MacOS SDK has been updated. '
                f'{result["current"]} ➜ {result["target"]}')
            result['advice'] = (
                'Contact DevOps regarding the new macOS SDK for the hermetic '
                'toolchain. Update `XCODE_VERSION` and `XCODE_VERSION` values '
                'in build/mac/download_hermetic_xcode.py for the new download '
                'URL.')
        return result

    def _check_rust_toolchain(self) -> Optional[Dict]:
        """Check for Rust toolchain updates.

    This function checks for any updates to the Rust toolchain, including the
    validity of the rust toolchain URL for syncing.

    Returns:
        An advisory record if any updates occurred, and there's no toolchain in
        our infra.
        """
        toolchain_sources = [
            'tools/rust/update_rust.py', 'tools/clang/scripts/update.py'
        ]
        rust_diff = self.look_for_diffs(*toolchain_sources)
        if not any(
                key in rust_diff for key in
            ['-RUST_REVISION =', '-RUST_SUB_REVISION =', '-CLANG_REVISION =']):
            return None

        def get_rust_clang_revision(version: str) -> str:
            rust_sources = repository.chromium.read_file(*toolchain_sources,
                                                         commit=version)
            return '-'.join([
                self.get_assigned_value(rust_sources, "RUST_REVISION"),
                self.get_assigned_value(rust_sources, "RUST_SUB_REVISION"),
                self.get_assigned_value(rust_sources, "CLANG_REVISION")
            ])

        updated_version = get_rust_clang_revision(self.target_version)
        rust_toolchain_url = RUST_TOOLCHAIN_URL.format(
            revision=updated_version)

        try:
            response = requests.head(rust_toolchain_url,
                                     allow_redirects=True,
                                     timeout=5)
            if response.status_code == 200:
                return None
        except requests.RequestException:
            pass  # Assume toolchain is not available if request fails

        commit_log = repository.chromium.run_git(
            'log', f'{self.working_version}..{self.target_version}',
            '--pretty=oneline', '-1', 'tools/rust/update_rust.py',
            'tools/clang/scripts/update.py')
        commit_hash, commit_message = commit_log[:40], commit_log[41:]

        return {
            'current': get_rust_clang_revision(self.working_version),
            'target': updated_version,
            'description': 'The rust toolchain has been updated.',
            'advice': 'Run the jobs in https://ci.brave.com/view/rust to generate a new Rust toolchain.',
            'commit': {
                'hash': commit_hash,
                'message': commit_message
            }
        }

    def _prerun_checks(self) -> bool:
        """Runs pre-run checks for the upgrade.

    This function runs a series of checks to make sure the upgrade can proceed
    without any issues. If any advisories have been found, this function will
    print a summary that looks something like:

    * Pre-run advisory (attention needed)
        * The rust toolchain has been updated.
            CL: Roll clang+rust llvmorg-21-init-1655-g7b473dfe-1 : llvmorg-2...
                https://chromium.googlesource.com/chromium/src/+/f9fada98083846
            Run the jobs in https://ci.brave.com/view/rust to generate a new...

    Returns:
        True if all checks pass, and False otherwise.
        """
        # Fetching the tags between the current version and the target to check
        # for certain things that may have changed that require attention
        if (not repository.chromium.is_valid_git_reference(
                self.working_version)
                or not repository.chromium.is_valid_git_reference(
                    self.target_version)):
            repository.chromium.run_git('fetch', versioning.GOOGLESOURCE_LINK,
                                        'tag', self.working_version, 'tag',
                                        self.target_version)

        advisories = [
            check for check in [
                self._check_win_toolchain(),
                self._check_mac_toolchain(),
                self._check_rust_toolchain()
            ] if check is not None
        ]

        if not advisories:
            return True

        terminal.log_task(
            '[bold]Pre-run advisory ([bold yellow]attention needed[/])')
        for advisory in advisories:
            console.print(Padding(f'* {advisory["description"]}', (0, 15)))
            console.print(
                Padding(f'CL: [dim]{advisory["commit"]["message"]}', (0, 19)))
            console.print(
                Padding(
                    GOOGLESOURCE_COMMIT_LINK.format(
                        commit=advisory["commit"]["hash"]), (0, 23)))
            console.print(Padding(f'{advisory["advice"]}', (0, 19)))

        replace(ContinuationFile.load(target_version=self.target_version),
                has_shown_advisory=True).save()

        return False

    def _continue(self,
                  no_conflict_continuation: bool = False,
                  apply_record: Optional[ApplyPatchesRecord] = None) -> bool:
        """Continues the upgrade process.

    This function is responsible for continuing the upgrade process. It will
    pick up from where the process left the last time.

    This function handles resumption in a way that the user may have to call
    brockit with `--continue` multiple times, which will result in this
    function being called every time.

    Files that are staged are considered as being meant for the
    `conflict-resolved` change. Deleted files will cause this function to bail
    out, so the user provide a commit message for the deletion.

    Args:
        no_conflict_continuation:
            Indicates that a continuation does not produce a conflict-resolved
            change.
        """
        if not apply_record:
            apply_record = (ContinuationFile.load(
                self.target_version).apply_record)

        if not self._run_update_patches_with_no_deletions():
            return False

        apply_record.stage_all_patches(ignore_deleted_files=True)
        has_changes = repository.brave.has_staged_changed()

        if not has_changes and not no_conflict_continuation:
            logging.error(
                'Nothing has been staged to commit conflict-resolved '
                'patches. (Did you mean to pass '
                '[bold cyan]--no-conflict-change[/]?)')
            return False

        if has_changes:
            self._save_conflict_resolved_patches()

        self._save_updated_patches()
        # Run init again to make sure nothing is missing after updating
        # patches.
        terminal.run_npm_command('init')

        terminal.run_npm_command('chromium_rebase_l10n')
        self._save_rebased_l10n()

        # With the continuation finished there's no need to keep the
        # continuation file around.
        ContinuationFile.clear()

        return True

    def _start(self, launch_vscode: bool, ack_advisory: bool) -> bool:
        """Starts the upgrade process.

    This function is responsible for starting the upgrade process. It will
    update the package version, run `npm run init`, and then run
    `npm run update_patches`. If any patches fail to apply, it will run
    `npm run apply_patches_3way` to allow for manual conflict resolution.

    For cases where no conflict resolution is required, the process will
    will continue, concluding the whole four steps of the upgrade process.

    Args:
        launch_vscode:
            Indicates if the user wants to launch vscode with the patches that
            require manual conflict resolution.

    Return:
        Returns True if the process was successful, and False otherwise.
        """
        if (self.working_version != self.chromium_src_version
                and self.target_version != self.chromium_src_version):
            logging.warning(
                'Chrommium seems to be synced to a version entirely '
                'unrelated. Brave %s ➜ Chromium %s', self.working_version,
                self.chromium_src_version)
        elif self.working_version != self.chromium_src_version:
            logging.warning(
                'Chromium is checked out with the target version. '
                'Brave %s ➜ Chromium %s', self.working_version,
                self.chromium_src_version)

        if self.working_version != self.base_version:
            terminal.log_task('Changes for this bump: %s' %
                              self.target_version.get_googlesource_diff_link(
                                  self.working_version))
        terminal.log_task(
            'Changes since base version: %s' %
            self.target_version.get_googlesource_diff_link(self.base_version))

        if not ack_advisory and not self._prerun_checks():
            console.log('👋 (Address advisories and then rerun with '
                        '[bold cyan]--ack-advisory[/])')
            return False

        self._update_package_version()

        try:
            terminal.run_npm_command('init')

            # When no conflicts come back, we can proceed with the
            # update_patches.
            if not self._run_update_patches_with_no_deletions():
                return False
        except subprocess.CalledProcessError as e:
            if ('There were some failures during git reset of specific '
                    'repo paths' in e.stderr):
                logging.warning(
                    '[bold cyan]npm run init[/] is failing to reset some'
                    ' paths. This could be happening because of a bad sync'
                    'state before starting the upgrade.')

            if (e.returncode != 0
                    and 'Exiting as not all patches were successful!'
                    in e.stderr.splitlines()[-1]):
                apply_record = self.apply_patches_3way(
                    launch_vscode=launch_vscode)
                if apply_record.requires_conflict_resolution():
                    # Manual resolution required.
                    console.log(
                        '👋 (Address all sections with '
                        f'{ACTION_NEEDED_DECORATOR} above, and then rerun '
                        '[italic]🚀Brockit![/] with [bold cyan]'
                        '--continue[/])')
                    return False

                # With all conflicts resolved, it is necessary to close the
                # upgrade with all the same steps produced when running an
                # upgrade continuation, as recovering from a conflict-
                # resolution failure.
                return self._continue(apply_record=apply_record)
            if e.returncode != 0:
                logging.error('Failures found when running npm run init\n%s',
                              e.stderr)
                return False

        self._save_updated_patches()

        terminal.run_npm_command('chromium_rebase_l10n')
        self._save_rebased_l10n()

        return True

    # The argument list differs here because that's the way we get args through
    # Task into the derived methods. Maybe something better can be done here.
    # pylint: disable=arguments-differ
    def execute(self, no_conflict_continuation: bool, launch_vscode: bool,
                with_github: bool, ack_advisory: bool) -> bool:
        """Executes the upgrade process.

    Keep in this function all code that is common to both start and continue.

    Args:
        no_conflict_continuation:
            Indicates that a continuation does not produce a conflict-resolved
            change.
        launch_vscode:
            Indicates the user wants to launch vscode with the patches that
            require manual conflict resolution.
        with_github:
            Indicates the user wants to create or update the github issue for
            the upgrade.
        """
        if self.target_version == self.working_version:
            logging.error(
                'This branch is already in %s. (Maybe you meant to pass [bold '
                'cyan]--continue[/]?)', self.target_version)
            return False

        if self.target_version < self.working_version:
            logging.error('Cannot upgrade version from %s to %s',
                          self.target_version, self.working_version)
            return False

        if not self.is_continuation:
            if ack_advisory:
                # This check for the use of `--ack-advisory` for the actual
                # case where advisory has been shown is to avoid accidental
                # suppressions of advisories, and to prevent `--ack-advisory`
                # becoming something similar to some `--force` flag.
                continuation = ContinuationFile.load(
                    target_version=self.target_version,
                    working_version=self.working_version,
                    check=False)
                if (continuation is not None
                        and not continuation.has_shown_advisory):
                    logging.error(
                        'Use [bold cyna]--ack-advisory[/] just after being '
                        'shown advisories.')
                    return False
            # We initialise the continuation file here rather than in the
            # constructor to avoid overwritting the file if the user made the
            # mistake of calling brockit again without `--continue`.
            ContinuationFile(target_version=self.target_version,
                             working_version=self.working_version,
                             base_version=self.base_version).save()

        if with_github and not _is_gh_cli_logged_in():
            # Fail early if gh cli is not logged in.
            logging.error('GitHub CLI is not logged in.')
            return False

        if self.is_continuation:
            if self.target_version != self.chromium_src_version:
                logging.error(
                    'To run with [bold cyan]--continue[/] the Chromium '
                    'version has to be in Sync with Brave.'
                    ' Brave %s ➜ Chromium %s', self.target_version,
                    self.chromium_src_version)
                return False

            result = self._continue(
                no_conflict_continuation=no_conflict_continuation)
        else:
            result = self._start(launch_vscode=launch_vscode,
                                 ack_advisory=ack_advisory)

        if result and with_github:
            GitHubIssue(base_version=self.base_version,
                        target_version=self.target_version
                        ).create_or_update_version_issue(with_pr=False)

        return result


def solve_git_ref(from_ref: str) -> str:
    """Solves the git reference.

    This function is used to resolve the git reference provided by the user.
    This reference can be either a branch name, a commit hash, or a special
    tag used by brockit to find specific changes.

    Args:
        from_ref:
            The git reference to resolve.

    Returns:
        The resolved git reference, if the reference is valid, or the
        reference for the solved special tag:
        @upstream: The upstream branch of the current branch.
        @previous: The commit just before the last version bump.
        @previous-major: The commit just before the last major version bump.
    """
    if from_ref and from_ref[0] != '@':
        # No special handling needed
        if not repository.brave.is_valid_git_reference(from_ref):
            logging.error(
                'Value provided to [bold cyan]--from-ref[/] is not a valid git '
                'ref: %s', from_ref)
            sys.exit(1)
        return from_ref

    if from_ref == "@upstream":
        upstream_branch = _get_current_branch_upstream_name()
        if upstream_branch is None:
            logging.error(
                'Could not determine the upstream branch. (Maybe set [bold '
                'cyan]--set-upstream-to[/] in your branch?)')
            sys.exit(1)
        return upstream_branch

    def find_previous_version_hash(is_major: bool = False) -> str:
        starting_version = Version.from_git('HEAD')
        base_version = starting_version
        last_changed = repository.brave.last_changed(versioning.PACKAGE_FILE)
        while True:
            base_version = Version.from_git(f'{last_changed}~1')
            if (is_major and base_version.major != starting_version.major):
                break
            if (not is_major and base_version != starting_version):
                break
            # Prefer to look for the PACKAGE_FILE here, because this has to
            # resolve even when the upgrade was done manually, so don't assume
            # the presence of pinslist timestamp changes.
            last_changed = repository.brave.last_changed(
                versioning.PACKAGE_FILE, f'{last_changed}~1')

        return f'{last_changed}~1'

    if from_ref == "@previous":
        return find_previous_version_hash()

    if from_ref == "@previous-major":
        return find_previous_version_hash(is_major=True)

    raise NotImplementedError(
        f'Unknown value for [bold cyan]--from-ref[/]: {from_ref}. '
        'Valid values are: @upstream, @previous, @previous-major')


class Rebase(Task):
    """Regenerates patches and strings for the current branch.

    This task is used for cases where the user wants to regenerate patches and
    strings. The purpose is to produce `Update patches` and `Updated strings`
    where approrpriate.
    """

    def status_message(self):
        return "Rebasing current branch..."

    @staticmethod
    def discard_regen_changes_from_rebase_plan(todo_file: PurePath):
        """Removes regen changes from the rebase plan.

    This function removes all lines that contain the string `Update patches`
    or `Updated strings` from the rebase plan file, and saves the changes to
    the file.
        """
        with open(todo_file, 'r') as file:
            lines = file.readlines()

        with open(todo_file, 'w') as file:
            for line in lines:
                if ('Update patches from Chromium ' not in line
                        and 'Updated strings for Chromium ' not in line):
                    file.write(line)

    @staticmethod
    def recommit_in_rebase_plan(todo_file: PurePath):
        """Recommits the first commit in the rebase plan.

    This function replaces the first `pick` in the rebase plan with `edit`,
    which forces the first commit to be recommitted.
        """
        with open(todo_file, 'r') as file:
            contents = file.read()

        with open(todo_file, 'w') as file:
            contents = contents.replace('pick', 'edit', 1)
            file.write(contents)

    @staticmethod
    def squash_minor_bumps_from_rebase_plan(todo_file: PurePath):
        """Squashes minor bumps from the rebase plan.

    This function groups all commmit lines that start with "Update from
    Chromium" into a single commit on top, and does the same for commits
    that start with "Conflict-resolved patches from Chromium"
    """
        with open(todo_file, 'r') as file:
            lines = file.readlines()

        version = []
        conflict = []
        gnrt = []
        others = []
        for line in lines:
            if 'Update from Chromium ' in line:
                version.append(
                    line if not version else line.replace('pick', 'squash'))
            elif 'Conflict-resolved patches from Chromium ' in line:
                conflict.append(
                    line if not conflict else line.replace('pick', 'squash'))
            elif '`gnrt` run for Chromium ' in line:
                gnrt.append(
                    line if not gnrt else line.replace('pick', 'squash'))
            else:
                others.append(line)

        console.print('Version:', version)

        with open(todo_file, 'w') as file:
            for line in version:
                file.write(line)
            for line in conflict:
                file.write(line)
            for line in gnrt:
                file.write(line)
            for line in others:
                file.write(line)

    @staticmethod
    def squash_commit_message_to_last_line(todo_file: PurePath):
        """Squashes the commit message to the last line.

    This function removes all lines that are not the last line of the commit
    message, and saves the changes to the file. This is useful to discard all
    the minor bumps from history.
        """
        with open(todo_file, 'r') as file:
            lines = file.readlines()

        # finds the last line that is not empty and that doesn't start with a
        # hash comment.
        last_valid_line = next(
            (line for line in reversed(lines)
             if line.strip() and not line.lstrip().startswith("#")), None)

        if not last_valid_line:
            # This should never happen, as there should be one valid line
            sys.exit(1)

        with open(todo_file, 'w') as file:
            file.write(last_valid_line)

    # The argument list differs here because that's the way we get args through
    # Task into the derived methods. Maybe something better can be done here.
    # pylint: disable=arguments-differ
    def execute(self, from_ref: Optional[str], to_ref: Optional[str],
                recommit: bool, discard_regen_changes: bool,
                squash_minor_bumps: bool) -> bool:
        """Rebases the current branch onto the provided ref.

    This function rebases the current branch onto the provided branch. It is
    the same as calling `git rebase --i --autosquash`.

    Args:
        from_ref:
            The reference to start rebasing from. This refers to the first
            change in the branch we want to pick up when rebasing. When null,
            it will either default to `@previous-major` when `to_ref` is in a
            different major version, or to `@previous` when `to_ref` is in the
            same version.
        to_ref:
            This is the git reference that we are rebasing onto. This will
            default to `@upstream` if not provided.
        recommit:
            Indicates that the first commit should be recommitted to force all
            the other commits to be recommitted as well.
        discard_regen_changes:
            Indicates that the changes that are automatically regenerated
            should be discarded.

    Returns:
        True if the rebase was successful, and False otherwise.
        """
        if to_ref is None:
            to_ref = '@upstream'

        to_ref = solve_git_ref(to_ref)

        if from_ref is None:
            if Version.from_git(to_ref).major != Version.from_git(
                    'HEAD').major:
                # If the major version is different, we default to the
                # previous major version.
                from_ref = solve_git_ref(from_ref or '@previous-major')
            else:
                # If the major version is the same, we default to the
                # previous version.
                from_ref = solve_git_ref(from_ref or '@previous')

        from_ref = solve_git_ref(from_ref)

        current_branch = repository.brave.current_branch()
        terminal.log_task(
            f'Rebasing {current_branch} onto {to_ref} starting from {from_ref}'
        )

        # We have to receive the rebase plan from git, and then modify it
        # if that's desired. That's done by calling this script again with
        # special internal flags.
        env = os.environ.copy()
        editor = [sys.executable, __file__]
        if discard_regen_changes:
            editor.append('--internal-rebase-remove-regen-changes')
        if recommit:
            editor.append('--internal-rebase-recommit')
        if squash_minor_bumps:
            editor.append('--internal-rebase-squash-minor-bumps')

            # Squashes will cause `GIT_EDITOR` also to open for the commit
            # message, so we need to handle those too.
            env["GIT_EDITOR"] = (
                f'{sys.executable} '
                f'{__file__} --internal-rebase-squash-commit-message')

        if len(editor) > 2:
            env["GIT_SEQUENCE_EDITOR"] = " ".join(editor)
        else:
            # If there are no internal operation, we can just return always
            # true to whatever plan git gives us.
            env["GIT_SEQUENCE_EDITOR"] = 'cmd /c "exit 0"' if platform.system(
            ) == 'Windows' else 'true'

        try:
            terminal.run([
                'git', 'rebase', '--interactive', '--autosquash',
                '--empty=drop', '--onto', to_ref, from_ref, current_branch
            ],
                         env=env)
            if recommit:
                repository.brave.run_git('commit', '--amend', '--no-edit')
                repository.brave.run_git('rebase', '--continue')
        except subprocess.CalledProcessError as e:
            logging.error('Rebase failed. %s', e.stderr)
            return False

        return True


def fetch_chromium_dash_version(channel: str) -> Version:
    """Fetches the latest version from the Chromium Dash.
    """
    response = requests.get(
        CHROMIUMDASH_LATEST_RELEASE.format(channel=channel), timeout=10)
    return Version(response.json()[0].get('version'))


def fetch_lastest_canary_version(channel) -> Version:
    """Fetches the latest canary version from the Chromium Dash.
    """
    if channel == 'canary':
        # The canary branch has two versions, the regular and the ASAN version,
        # and we want the highest of the two.
        canary_version = fetch_chromium_dash_version('canary')
        canary_asan_version = fetch_chromium_dash_version('canary_asan')
        return max(canary_version, canary_asan_version)

    return fetch_chromium_dash_version(channel)


def show(args: argparse.Namespace):
    """Prints various insights about brave-core.

    This is a helper command line that allows us to inspect a few things about
    brave-core and how brockit process things.
    """
    if args.package_version:
        console.print(f'upstream version: {Version.from_git("HEAD")}')

    if args.from_ref_value is not None:
        from_ref_value = Version.from_git(solve_git_ref(args.from_ref_value))
        if from_ref_value is not None:
            console.print(f'base version: {from_ref_value}')

    if args.log_link:
        console.print('googlesource link: %s' %
                      Version.from_git('HEAD').get_googlesource_diff_link(
                          Version.from_git(solve_git_ref('@previous'))))

    if args.latest_chromiumdash_version is not None:
        console.print(
            'latest canary version: %s' %
            fetch_lastest_canary_version(args.latest_chromiumdash_version))

    return 0


def main():
    # This is a global parser with arguments that apply to every function.
    global_parser = argparse.ArgumentParser(add_help=False)
    global_parser.add_argument(
        '--verbose',
        action='store_true',
        help='Produces verbose logs (full command lines being executed, etc).')
    global_parser.add_argument(
        '--infra-mode',
        action='store_true',
        help=
        ('Indicates that the script is being run in the infra environment. '
         'This changes the script output, specially providing feedback for the '
         'CI to be kept alive.'),
        dest='infra_mode')

    # The `--from-ref` parse is used by multiple operations.
    base_version_parser = argparse.ArgumentParser(add_help=False)
    base_version_parser.add_argument(
        '--from-ref',
        help=
        'A reference to the version that the upgrade is coming from. This is '
        'a git branch, hash, tag, etc, or one of the special values: @upstream'
        ' (upstream branch), @previous (the previous version from HEAD). '
        'Defaults to @upstream.',
        default=None)

    parser = argparse.ArgumentParser()

    subparsers = parser.add_subparsers(dest='command', required=True)
    lift_parser = subparsers.add_parser(
        'lift',
        parents=[global_parser, base_version_parser],
        help='Upgrade the chromium base version. Special tags: '
        '@latest-[beta|dev|canary] pulls the version from chromium dash.')
    lift_parser.add_argument('--to',
                             required=True,
                             help='The branch used as the base version.')
    lift_parser.add_argument(
        '--continue',
        action='store_true',
        help='Resumes from manual patch conflict resolution.',
        dest='is_continuation')
    lift_parser.add_argument(
        '--ack-advisory',
        action='store_true',
        help=
        'Added to indicate that pre-run check advisory has been acknowledged.')
    lift_parser.add_argument(
        '--restart',
        action='store_true',
        help='Resumes from manual patch conflict resolution.')
    lift_parser.add_argument(
        '--with-github',
        action='store_true',
        help='Creates or updates the github for this branch.',
        dest='with_github')
    lift_parser.add_argument(
        '--vscode',
        action='store_true',
        help=
        'Launches vscode for manual conflict resolution and similar issues.')
    lift_parser.add_argument(
        '--no-conflict-change',
        action='store_true',
        help='Indicates that a continuation does not have conflict patches to '
        'commit any longer.',
        dest='no_conflict')

    subparsers.add_parser(
        'regen',
        parents=[global_parser, base_version_parser],
        help='Regenerates all patches and strings for the current branch.')

    rebase_parser = subparsers.add_parser(
        'rebase',
        parents=[global_parser, base_version_parser],
        help='Rebases the current branch.')
    rebase_parser.add_argument(
        '--to-ref',
        default=None,
        help='The branch you are rebasing to. Defaults to the upstream branch.'
    )
    rebase_parser.add_argument(
        '--recommit',
        action='store_true',
        help=
        'Even if there is nothing to rebase, do a rebase to recommit changes.')
    rebase_parser.add_argument(
        '--discard-regen-changes',
        action='store_true',
        help=
        'Discard patches like "Update patches" and "Updated strings" that can '
        'be regenerated.')
    rebase_parser.add_argument(
        '--squash-minor-bumps',
        action='store_true',
        help=
        'Squashes all the minor bumps in-between the the last version and the '
        'previous upstream ref.')

    subparsers.add_parser(
        'update-version-issue',
        parents=[global_parser, base_version_parser],
        help='Creates or updates the GitHub issue for the corrent branch.')

    show_parser = subparsers.add_parser(
        'show', help='Prints various insights about brave-core.')
    show_parser.add_argument(
        '--package-version',
        action='store_true',
        help='Shows the current Chromium version in package.')
    show_parser.add_argument(
        '--from-ref-value',
        help='Shows the Chromium version from a git reference.',
        default=None,
        dest='from_ref_value')
    show_parser.add_argument('--log-link',
                             action='store_true',
                             help='Prints the git log links to googlesource.')
    show_parser.add_argument(
        '--latest-chromiumdash-version',
        default=None,
        help='Prints the latest version available for the channel provided.')

    subparsers.add_parser('reference',
                          help='Detailed documentation for this tool.')
    args = parser.parse_args()

    def has_verbose():
        return hasattr(args, 'verbose') and args.verbose

    logging.basicConfig(
        level=logging.DEBUG if has_verbose() else logging.INFO,
        format='%(message)s',
        handlers=[IncendiaryErrorHandler(markup=True, rich_tracebacks=True)])

    if hasattr(args, 'infra_mode') and args.infra_mode:
        terminal.set_infra_mode()

    if hasattr(args, 'from_ref'):
        if args.command == 'lift' and args.is_continuation:
            if args.from_ref is not None:
                parser.error(
                    'Switch --from-ref not supported with --continue.')

    def resolve_from_ref_flag_version() -> Version:
        return Version.from_git(
            solve_git_ref(args.from_ref if args.from_ref else '@upstream'))

    if args.command == 'lift' and args.no_conflict and not args.is_continuation:
        parser.error('--no-conflict-change can only be used with --continue')
    if args.command == 'lift' and args.restart and args.is_continuation:
        parser.error('--restart does not support --continue')
    if args.command == 'lift' and args.ack_advisory and args.is_continuation:
        parser.error('--ack-advisory does not support --continue')
    if args.command == 'lift' and args.to.startswith('@latest-'):
        [_, channel] = args.to.split('-')
        if channel not in ['canary', 'beta', 'dev']:
            parser.error('Invalid channel for --to.')

    def resolve_to_flag() -> Version:
        if args.to.startswith('@latest-'):
            [_, channel] = args.to.split('-')
            if channel not in ['canary', 'beta', 'dev']:
                parser.error('Invalid channel for --to.')
            return fetch_lastest_canary_version(channel)
        return Version(args.to)

    if args.command == 'lift':
        target = resolve_to_flag()
        if args.restart:
            if ReUpgrade(target).run() != 0:
                return 1

        if not args.is_continuation:
            upgrade = Upgrade(target, args.is_continuation,
                              resolve_from_ref_flag_version())
        else:
            upgrade = Upgrade(resolve_to_flag(), args.is_continuation)

        return upgrade.run(args.no_conflict, args.vscode, args.with_github,
                           args.ack_advisory)
    if args.command == 'rebase':
        return Rebase().run(args.from_ref, args.to_ref, args.recommit,
                            args.discard_regen_changes,
                            args.squash_minor_bumps)
    if args.command == 'regen':
        return Regen(resolve_from_ref_flag_version()).run()
    if args.command == 'update-version-issue':
        return GitHubIssue(resolve_from_ref_flag_version()).run()
    if args.command == 'reference':
        return console.print(Markdown(__doc__))
    if args.command == 'show':
        return show(args)

    return 0


if __name__ == '__main__':
    if any(arg.startswith("--internal-rebase") for arg in sys.argv):
        # Special flags used to carry out some of the rebase tasks fed by git
        # during rebase --interactive mode.
        if '--internal-rebase-remove-regen-changes' in sys.argv:
            Rebase.discard_regen_changes_from_rebase_plan(
                PurePath(sys.argv[-1]))
        if '--internal-rebase-recommit' in sys.argv:
            Rebase.recommit_in_rebase_plan(PurePath(sys.argv[-1]))
        if '--internal-rebase-squash-minor-bumps' in sys.argv:
            Rebase.squash_minor_bumps_from_rebase_plan(PurePath(sys.argv[-1]))
        if '--internal-rebase-squash-commit-message' in sys.argv:
            Rebase.squash_commit_message_to_last_line(PurePath(sys.argv[-1]))
        sys.exit(0)

    sys.exit(main())
