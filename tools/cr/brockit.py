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
or even hashes. Additionally there are special labels that can be passed to
this flag.

 * `--from-ref=@upstream`: This will use the upstream branch as the base for
    the lift. This requires the user to set an upstream branch. This value is
    the default when none is provided.
 * `--from-ref=@previous`: This tag means the previous version since the last
    upgrade in the current branch. This is useful when telling brockit that you
    are doing a minor version bump.
 * `--from-ref=@previous-major`: This label means the reference the parent
    commit for the current major. This is useful when doing rebases, and
    wanting to select from the version change.

Using upstream:

```sh
git branch --set-upstream-to=origin/master
tools/cr/brockit.py lift --to=135.0.7037.1
```

The `--to` flag also accepts special labels:
 * `--to=@latest-tag`: Uses the latest tag from the Chromium repository.
 * `--to=@latest-m{MAJOR}`: Uses the latest tag for the given major version
    (e.g. `--to=@latest-m135`).
 * `--to=@latest-for-branch`: Infers the major version from the current branch
    name, which must follow the `cr{MAJOR}` convention (e.g. `cr135`), and
    uses the latest tag for that major.
 * `--to=@latest-canary`: Uses the latest canary version from Chromium Dash.
    Both the *Canary* and the *Canary (DCHECK)* channels are queried and the
    highest version is used.
 * `--to=@latest-dev`, `--to=@latest-beta`, `--to=@latest-stable`: Same as
    the flag above, but for the respective channel.

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
   other similar arguments you may want to keep.
7. *🚀Brockit!* will pick up from where it stopped, possibly running
  `npm run update_patches`, staging all patches, and committing the under
  *Conflict-resolved patches from Chromium [from] to [to].*
8. *Update patches from Chromium [from] to [to]* will be committed.
9. *Updated strings for Chromium [to]* will be committed.
10. `gnrt` will be rerun and the changes produced under `third_party/rust/` will
    be committed.

Steps 3-7 may end up being skipped altogether if no failures take place, or in
part if resolution is possible without manual intervention.

The `--restart` flag can be used to start the process from scratch, discarding
everything committed in the last run.

```sh
tools/cr/brockit.py lift --to=135.0.7037.1 --from-ref=origin/master --restart
```

### `brockit.py regen`
Additionally, *🚀Brockit!* can be run with `regen`. This is useful to generate
the "Update patches", "Updated strings", and `gnrt` run commits on their own
when rebasing branches, regenerating these files as desired. If you want to run
these operations and yet, not commit any changes, you can use `--dry-run`.

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

### `brockit.py update-xcode-toolchain`
The hermetic Xcode toolchain is generated in CI for the macOS SDK that Chromium
pins in `build/config/mac/mac_sdk.gni`, and published to Brave's download bucket
alongside a sibling YAML index. This command repins
`build/mac/download_hermetic_xcode.py` to that published toolchain.

Pass `--to` with the Chromium reference whose pinned macOS SDK to repin against.
It accepts a concrete version or the same `@latest-*` labels as `lift --to`:

```sh
tools/cr/brockit.py update-xcode-toolchain --to=150.0.7850.1
```

`brockit` reads the SDK version/build from `mac_sdk.gni` at that ref, downloads
the matching toolchain index, and rewrites the archive hash and the SDK
version/build constants (and the provenance comment) in
`build/mac/download_hermetic_xcode.py`. It also lifts the
`MAC_MINIMUM_OS_VERSION` block (the minimum-OS gate and its comment) verbatim
from Chromium's `build/mac_toolchain.py` at the same ref.

Pass `--culprit=<hash>` to provide a specific culprit for the toolchain update.
If none is provided, `brockit` determines the culprit by looking for the last
commit that pinned the macOS SDK in `mac_sdk.gni` up to the `--to` ref.

### `brockit.py gen-{rust,xcode,windows}-toolchain`
These commands trigger a toolchain's CI (Jenkins) pipeline(s) for a given
Chromium tag.

```sh
tools/cr/brockit.py gen-rust-toolchain 150.0.7850.1
tools/cr/brockit.py gen-xcode-toolchain 150.0.7850.1
tools/cr/brockit.py gen-windows-toolchain 150.0.7850.1
```

The `tag` argument also accepts the same `@latest-*` labels (e.g. `@latest-tag`,
etc).

Pass `--watch` to show a live-updating table of each pipeline's stage and
status until all finish. Pressing Ctrl+C stops watching, but the builds keep
running.

```sh
tools/cr/brockit.py gen-rust-toolchain @latest-canary --watch
```

### `brockit.py update-rust-wasm-toolchain`
This command repins the Rust/WASM toolchain objects in
`tools/cr/install_extra_deps.py` to the latest published archives for
a given Chromium tag's Rust+Clang revision, and commits the change. The tag's
`tools/rust/update_rust.py` and `tools/clang/scripts/update.py` are read to
identify the toolchain to pin.

```sh
tools/cr/brockit.py update-rust-wasm-toolchain --to=150.0.7850.1
```

The `--to` expects a Chromium referecence, and this includes reference labels
(e.g. `@latest-tag`, etc).

Pass `--culprit=<hash>` to reference a specific Chromium commit in the commit
body. If none is provided, `brockit` uses the last Chromium commit that has
changed the versioning of the rust toolchain.

### `brockit.py reassign`
This command is used to change the authorship of a given commit in the branch.
It generates an empty commit with the message `reassign! {original_subject}`
where the author is the person running the command.

```sh
tools/cr/brockit.py reassign <commit_hash>
```

As a shortcut, `git cr commit --fixup=reassign:<commit_hash>` runs this exact
command. This mirrors git's native `--fixup=amend:` / `--fixup=reword:` modes,
adding a brave-core `reassign:` mode to the same family.

This commit acts similarly to a `fixup!` commit but works in tandem with
`brockit.py rebase`. When `brockit.py rebase` is run, the rebase process detects
the `reassign! ` commit. It moves this commit directly above the original target
commit in the interactive rebase sequence, changing the target commit's command
from `pick` to `squash`. (This only works when using `rebase` with
`--squash-minor-bumps`.)

During the squash operation, `brockit.py rebase` also automatically edits the
squashed commit message by removing the `reassign! ` line and preserving the
original commit's subject and body. Because the original commit is squashed into
the reassignment commit, the resulting commit adopts the reassignment commit's
authorship.

### `brockit.py drop`
This is the counterpart to `reassign`. It generates an empty commit with the
message `drop! {original_subject}`, marking the referenced change to be dropped
by `brockit.py rebase`.

```sh
tools/cr/brockit.py drop <commit_hash>
```
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field, replace
from datetime import datetime
import json
import logging
import os
from pathlib import Path
import pickle
import platform
import re
import requests
from rich.markdown import Markdown
from rich.padding import Padding
import subprocess
import sys

from exceptions import (ActionNeededException, BadOutcomeException,
                        InvalidInputException)
from git_status import GitStatus
from patchfile import Patchfile
import plaster
from plaster import PlasterFile, PlasterFileNeedsRegen
import rebase
from rebase import DROP_COMMIT_MSG_PREFIX, REASSIGN_COMMIT_MSG_PREFIX
import repository
from repository import Repository
from terminal import (IncendiaryErrorHandler, Task as BaseTask, console,
                      is_verbose, terminal)
import toolchain
import versioning
from versioning import Version
from vpython_utils import VPYTHON3_PATH
from vscode import VsCodeIpcConnection

# This file is updated whenever the version number is updated in package.json
PINSLIST_TIMESTAMP_FILE = (
    'chromium_src/net/tools/transport_security_state_generator/'
    'input_file_parsers.cc')
VERSION_UPGRADE_FILE = Path('.version_upgrade')

# Commit subject prefixes that identify brockit-managed upgrade commits.
# Patches whose most recent branch commit carries one of these subjects are
# NOT dev-cycle changes and therefore must NOT be turned into fixups.
_UPGRADE_COMMIT_WITH_PATCHES_PREFIXES = (
    'Conflict-resolved patches from Chromium ',
    'Apply-fixed 🩹 patches from Chromium ',
    'Update patches from Chromium ',
)

# The link to a specific commit in the Chromium source code.
GOOGLESOURCE_COMMIT_LINK = f'{versioning.GOOGLESOURCE_LINK}' '/+/{commit}'

# Google dash link used to check the latest version for a given channel
CHROMIUMDASH_LATEST_RELEASE = 'https://chromiumdash.appspot.com/fetch_releases?channel={channel}&platform={platform}&num=1'

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

# Banners framing every Brockit task run.
_BROCKIT_START_BANNER = '[italic]🚀 Brockit!'
_BROCKIT_END_BANNER = '[bold]💥 Done!'


def _get_current_branch_upstream_name() -> str | None:
    """Retrieves the name of the current branch's upstream.
    """
    try:
        return repository.brave.run_git('rev-parse', '--abbrev-ref',
                                        '--symbolic-full-name', '@{upstream}')
    except subprocess.CalledProcessError:
        return None


def _ensure_chromium_tags(*refs: Version | str) -> None:
    """Ensures each Chromium reference resolves locally, fetching any that are
    missing as tags from Googlesource.

    Tags for freshly released Chromium versions are frequently absent from a
    local checkout, so any operation that resolves a version to a commit (e.g.
    the culprit pickaxes, `git show <tag>:<file>`) must fetch the tag first.
    """
    missing = [
        ref for ref in refs
        if not repository.chromium.is_valid_git_reference(ref)
    ]
    if not missing:
        return

    fetch_args = ['fetch', versioning.GOOGLESOURCE_LINK]
    for ref in missing:
        fetch_args += ['tag', ref]
    repository.chromium.run_git(*fetch_args)


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
        raise ValueError(
            'Expected pattern for PinsListTimestamp block not found. '
            'Aborting.')

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
    (repository.brave.root / PINSLIST_TIMESTAMP_FILE).write_text(
        updated_content, encoding='utf-8', newline='')

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
        pass

    return False


def _get_apply_patches_list() -> dict[Repository, list[Patchfile]] | None:
    """Retrieves the list of patches to be applied by running
    `npm run apply_patches`, grouped by repository.
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
        entries = json.loads(match.group(0))
        patch_paths = [Path(entry['patchPath']) for entry in entries]
        patch_files: dict[Repository, list[Patchfile]] = {}
        for patch_path in patch_paths:
            patchfile = Patchfile(path=patch_path)
            patch_files.setdefault(patchfile.repository, []).append(patchfile)
        return patch_files

    return None


@dataclass(frozen=True)
class ApplyPatchesRecord:
    """A class to hold the continuation data for patches.
    """

    # A dictionary of all patches with attempted `--3way`, grouped by
    # repository.
    patch_files: dict[Repository,
                      list[Patchfile]] = field(default_factory=dict)

    # A list of patches that cannot be applied due to their source file being
    # deleted.
    patches_to_deleted_files: list[Patchfile] = field(default_factory=list)

    # A list of files that require manual conflict resolution before continuing.
    files_with_conflicts: list[Path] = field(default_factory=list)

    # A list of patches that fail entirely when running apply with `--3way`.
    broken_patches: list[Patchfile] = field(default_factory=list)

    # A list of patches where the apply failure was resolved by plaster.
    plaster_fixed_patches: list[Path] = field(default_factory=list)

    # A list of patches where the apply failure could not be resolved by
    # plaster.
    plaster_broken_patches: list[Patchfile] = field(default_factory=list)

    def all_conflict_resolved_patches(self) -> list[Patchfile]:
        """Returns a flattened list of all conflict-resolved candidates."""
        return [p for patches in self.patch_files.values() for p in patches]

    def requires_conflict_resolution(self):
        """Checks if there are any patches that require manual conflict
        resolution.

        This function is used to determine it is necessary to stop the process
        to address any potential patch changes.
        """
        return (self.files_with_conflicts or self.patches_to_deleted_files
                or self.broken_patches or self.plaster_broken_patches)

    def check_broken_plasters_fixed(self) -> None:
        """Verifies all previously-broken plasters are now resolved.

        For each entry in plaster_broken_patches:
        - If the plaster file still exists, runs a dry-run check to confirm
          the plaster output is up to date.
        - If the plaster file is gone, verifies the corresponding .patch
          file has also been removed.

        Raises InvalidInputException if any broken plaster is not yet resolved.
        """
        for patchfile in self.plaster_broken_patches:
            if patchfile.plaster.exists():
                try:
                    PlasterFile(patchfile.plaster).apply(dry_run=True)
                except PlasterFileNeedsRegen as e:
                    raise InvalidInputException(
                        'Plaster file has not been fixed and re-applied: '
                        f'{patchfile.plaster}') from e
            else:
                if (repository.brave.root / patchfile.path).exists():
                    raise InvalidInputException(
                        'Plaster file was deleted but patch still exists: '
                        f'{patchfile.path}')

    def stage_all_patches(self):
        """Stages all patches that were applied, so they can be committed as
        conflict-resolved patches.

        Args:
            ignore_deleted_files:
                If set to True, deleted files will be ignored, and not staged.
        """
        for _, patches in self.patch_files.items():
            for patchfile in patches:
                if not (repository.brave.root / patchfile.path).exists():
                    # Skip deleted files.
                    continue

                repository.brave.run_git('add', patchfile.path)


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
    apply_record: ApplyPatchesRecord | None = field(default=None)

    @staticmethod
    def load(target_version: Version,
             working_version: Version | None = None,
             check: bool = True) -> ContinuationFile | None:
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

        continuation = pickle.loads(VERSION_UPGRADE_FILE.read_bytes())

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
        VERSION_UPGRADE_FILE.write_bytes(pickle.dumps(self))

    @staticmethod
    def clear():
        logging.debug('Clearing the continuation file.')
        try:
            Path(VERSION_UPGRADE_FILE).unlink()
        except FileNotFoundError:
            pass


class Task(BaseTask):
    """Base class for all Brockit tasks.

    Adds the Brockit banners around the generic `terminal.Task` run; the actual
    behaviour (status spinner, `execute`/`status_message` contract) lives in the
    base class. Subclasses provide `status_message`.
    """

    # Abstract base: concrete subclasses implement `status_message`.
    # pylint: disable=abstract-method

    start_banner = _BROCKIT_START_BANNER
    end_banner = _BROCKIT_END_BANNER


class Versioned(Task):
    """ Base class for all versioned tasks.

    Versioned tasks are tasks that have the concept of a base version and a
    target version.
    """

    def __init__(self,
                 base_version: Version,
                 target_version: Version | None = None):
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
            raise InvalidInputException(
                f'Target version {self.target_version} is not higher than base '
                f'version {self.base_version}.')

    def is_major(self) -> bool:
        """Returns True if this is a major version upgrade."""
        return self.target_version.major > self.base_version.major

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

    def _save_gnrt_rerun(self, dry_run=False):
        """Creates the updated patches change

    This function creates the third commit in the order of the update, saving
    all patches that might have been changed or deleted. Untracked patches are
    excluded from addition at this stage.
    """
        terminal.run([VPYTHON3_PATH, './tools/crates/run_gnrt.py', 'vendor'],
                     cwd=repository.chromium.root)
        terminal.run([VPYTHON3_PATH, './tools/crates/run_gnrt.py', 'gen'],
                     cwd=repository.chromium.root)

        if dry_run:
            return

        repository.brave.run_git('add', '-u', 'third_party/rust/')
        repository.brave.git_commit(
            f'`gnrt` run for Chromium {self.target_version}.')

    def status_message(self) -> str:
        raise NotImplementedError


class Regen(Versioned):
    """Regenerates patches and strings for the current branch.

    This task is used for cases where the user wants to regenerate patches and
    strings. The purpose is to produce `Update patches` and `Updated strings`
    where approrpriate.
    """

    def status_message(self):
        return "Updating patches and strings..."

    def execute(self, dry_run=False) -> bool:
        terminal.log_task(
            f'Processing changes for Chromium {self.base_version} '
            f'to Chromium {self.target_version}.')

        terminal.run_npm_command('init')

        terminal.run_npm_command('update_patches', '--', '--no-plaster-check')
        if not dry_run:
            self._save_updated_patches()

        terminal.run_npm_command('chromium_rebase_l10n')
        if not dry_run:
            self._save_rebased_l10n()

        self._save_gnrt_rerun(dry_run=dry_run)


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
        if self.is_major():
            # For major updates, the issue description doesn't have a precise
            # version number.
            title = title.format(previous=str(self.base_version.major),
                                 to=str(self.target_version.major))
        else:
            title = title.format(previous=str(self.base_version),
                                 to=str(self.target_version))
        return title

    def lookup_issue(self, title: str) -> list | None:
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

    def create_push_request(self, issue_url: str):
        """Creates a push request for the current branch.

        This function creates a push request for the upgrade.
        """
        current_branch = repository.brave.current_branch()
        if current_branch == 'HEAD':
            raise InvalidInputException(
                'Cannot create a push request: Not in a branch')

        # It is assumed that the upstream branch is the base branch for the PR.
        upstream_branch = _get_current_branch_upstream_name()
        if upstream_branch is None:
            raise InvalidInputException(
                'Cannot create a push request: Not in a branch with an upstream'
            )
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
            return

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
            '--label', '"CI/run-linux-arm64"', '--label', '"CI/run-macos-x64"',
            '--label', '"CI/run-windows-x86"', '--label',
            '"CI/run-windows-arm64"', '--assignee', 'cdesouza-chromium'
        ]
        # Emerick and Alexey take care of even-numbered major versions, while
        # Max and Sam do the odd ones.
        if self.target_version.major % 2 == 0:
            cmd += ['--assignee', 'emerick', '--assignee', 'AlexeyBarabash']
        else:
            cmd += ['--assignee', 'mkarolin', '--assignee', 'samartnik']

        if self.is_major() or upstream_branch == 'master':
            # It is not common for upstream test to be run on uplifts of minor
            # upgrades.
            cmd += ['--label', '"CI/run-upstream-tests"']

        if self.is_major():
            cmd += ['--label', '"CI/storybook-url"']

            # Always create PR for major version upgrades as draft
            cmd.append('--draft')

        try:
            pr_url = terminal.run(cmd).stdout.strip()
            terminal.log_task(f'GitHub PR created for this bump: {pr_url}')
        except subprocess.CalledProcessError as e:
            raise BadOutcomeException(
                f'Failed to create PR for {current_branch}: {e.stderr.strip()}'
            ) from e

        if is_uplift:
            # Only uplift branches set milestones.
            results = json.loads(
                terminal.run([
                    'gh', 'api', 'repos/brave/brave-core/milestones', '--jq',
                    '[.[] | {number, title}]'
                ]).stdout)
            if not results:
                raise BadOutcomeException(
                    'No milestones returned for brave-core')

            milestone = next(
                (entry["number"] for entry in results
                 if entry["title"].startswith(f'{upstream_branch} - ')), None)
            if milestone is None:
                raise BadOutcomeException(
                    f'Failed to find milestone for branch {upstream_branch}')

            pr_number = pr_url.rsplit('/', 1)[-1]
            terminal.run([
                'gh', 'api', '-X', 'PATCH',
                f'repos/brave/brave-core/issues/{pr_number}', '-F',
                f'milestone={milestone}'
            ])

    def create_or_update_version_issue(self, with_pr: bool):
        """Creates a github issue for the upgrade.

        This function creates/updates the upgrade github issue.
        """
        link = self.target_version.get_googlesource_diff_link(
            from_version=str(self.base_version))

        title = self.compose_issue_title()
        issue = self.lookup_issue(title)
        if issue is not None:
            pattern = (
                r"https://chromium\.googlesource\.com/chromium/src/\+log/[^\s]+"
            )
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
                self.create_push_request(issue["url"])
            return

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
            self.create_push_request(issue_url)

    def execute(self):
        if not _is_gh_cli_logged_in():
            raise BadOutcomeException('GitHub CLI is not logged in.')

        self.create_or_update_version_issue(with_pr=True)


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

    def execute(self):
        """Restarts the upgrade process.

        This function will reset the repository to the state it was before the
        upgrade started. It will also clear the continuation file.
        """
        working_version = Version.from_git('HEAD')
        if self.target_version != working_version:
            raise InvalidInputException(
                f'Running with `--restart` but the target version does not '
                f'match the current version. {self.target_version} '
                f'vs {working_version}')

        starting_change = repository.brave.last_changed(
            PINSLIST_TIMESTAMP_FILE)
        commit_message = repository.brave.get_commit_short_description(
            starting_change)
        if not commit_message.startswith(
                'Update from Chromium ') or not commit_message.endswith(
                    f' to Chromium {self.target_version}.'):
            raise InvalidInputException(
                f'Running with `--restart` but the last change does not match '
                f'the arguments provided. {starting_change} {commit_message}')

        console.log('Discarding the following changes:')
        console.log(
            Padding(
                '[dim]%s' % repository.brave.run_git(
                    'log', '--pretty=%h %s', f'HEAD...{starting_change}~1'),
                (0, 4)))

        ContinuationFile.clear()
        repository.brave.run_git('reset', '--hard', f'{starting_change}~1')


class Upgrade(Versioned):
    """The upgrade process, holding the data related to the upgrade.

  This class produces an object that is reponsible for keeping track of the
  upgrade process step-by-step. It acquires all the common data necessary for
  its completion.
  """

    def __init__(self,
                 target_version: Version,
                 is_continuation: bool,
                 base_version: Version | None = None):
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
                raise InvalidInputException(
                    f'Running with `--continue` on a branch with a different '
                    f'version what the target should be. {target_version} '
                    f'vs {version_on_head}')

            # Loads the working version from the continuation file, because the
            # current branch has already updated the working version to the
            # target version.
            try:
                continuation = ContinuationFile.load(
                    target_version=target_version)
            except FileNotFoundError as e:
                raise InvalidInputException(
                    f'{VERSION_UPGRADE_FILE} continuation file does not exist. '
                    '(Are you sure you meant to pass [bold cyan]--continue[/]?)'
                ) from e

            self.working_version = continuation.working_version
            base_version = continuation.base_version
        else:
            self.working_version = Version.from_git('HEAD')

        # The version currently set in the VERSION file.
        self.chromium_src_version = versioning.read_chromium_version_file()

        super().__init__(base_version, target_version)

    def status_message(self):
        return "Upgrading Chromium base version"

    def apply_patches_3way(self) -> ApplyPatchesRecord:
        """Applies patches that have failed using the --3way option to allow for
        manual conflict resolution.

        This method will apply the patches and reset the state of applied
        patches. Additionally, it will also produce a list of the files that
        are waiting for conflict resolution.

        A list of the patches applied will be produced as well.

        When running brockit in a vscode terminal, this method will open any
        files that need attention in the editor session.
        """
        # This is a flat list of patches that cannot be applied due to their
        # source file being deleted.
        patches_to_deleted_files: list[Patchfile] = []

        # A list of files that require manual conflict resolution before
        # continuing.
        files_with_conflicts: list[Path] = []

        # These are patches that fail entirely when running apply with `--3way`.
        broken_patches: list[Patchfile] = []

        # Patches where the apply failure was resolved by plaster.
        plaster_fixed_patches: list[Path] = []

        # Patches where the apply failure could not be resolved by plaster.
        plaster_broken_patches: list[Patchfile] = []

        # A list of all patchfiles that failed to apply, grouped by repository.
        patch_files = _get_apply_patches_list()
        if patch_files is None:
            raise ValueError('Apply patches had no failed patches.')

        if patch_files:
            terminal.log_task(
                '[bold]Reapplying patch files with --3way:\n[/]%s' %
                '\n'.join(f'    * {patchfile.path}'
                          f'{" 🩹" if patchfile.has_plaster else ""}'
                          for patch_list in patch_files.values()
                          for patchfile in patch_list))

        vscode_files: list[Path] = []
        for repo, patches in patch_files.items():
            for patchfile in patches:
                status: Patchfile.ApplyStatus = patchfile.apply()

                if status == Patchfile.ApplyStatus.CONFLICT:
                    files_with_conflicts.append(patchfile.source_from_brave())
                elif status == Patchfile.ApplyStatus.BROKEN:
                    broken_patches.append(patchfile)
                elif status == Patchfile.ApplyStatus.DELETED:
                    patches_to_deleted_files.append(patchfile)
                elif status == Patchfile.ApplyStatus.PLASTER_FIXED:
                    plaster_fixed_patches.append(patchfile.path)
                elif status == Patchfile.ApplyStatus.PLASTER_BROKEN:
                    plaster_broken_patches.append(patchfile)

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
            deletion_report: dict[str, dict[Patchfile,
                                            Patchfile.SourceStatus]] = {}

            for patchfile in patches_to_deleted_files:
                # Finding the culptrit commit hash.
                commit = patchfile.get_last_commit_for_source()
                deletion_report.setdefault(
                    commit,
                    {})[patchfile] = patchfile.get_source_removal_status(
                        commit)

            for commit, patches in deletion_report.items():
                for patchfile, status in patches.items():
                    if status.status == 'D':
                        console.log(
                            Padding(
                                f'✘ {patchfile.source} [red bold](deleted)',
                                (0, 4)))
                        vscode_files.append(patchfile.path)
                    elif status.status == 'R':
                        renamed_to = Path(patchfile.repository.from_brave() /
                                          status.renamed_to)
                        console.log(
                            Padding(
                                f'✘ {patchfile.source_from_brave()}\n    '
                                f'([yellow bold]renamed to[/] {renamed_to})',
                                (0, 4)))
                        vscode_files += [patchfile.path, renamed_to]

                # Printing the commmit message for the grouped changes.
                console.log(
                    Padding(
                        f'{next(iter(patches.items()))[1].commit_details}\n',
                        (0, 8),
                        style="dim"))

        if broken_patches:
            terminal.log_task(
                '[bold]Broken patches that fail to apply entirely '
                f'{ACTION_NEEDED_DECORATOR}:[/]')

            for patchfile in broken_patches:
                source = patchfile.source_from_brave()
                console.log(Padding(f'✘ {patchfile.path} ➜ {source}', (0, 4)))
                vscode_files += [patchfile.path, source]

        if plaster_broken_patches:
            terminal.log_task('[bold]Plaster failed to fix patches '
                              f'{ACTION_NEEDED_DECORATOR}:[/]')

            for patchfile in plaster_broken_patches:
                source = patchfile.source_from_brave()
                console.log(
                    Padding(f'✘ {patchfile.plaster} ➜ {source}', (0, 4)))
                vscode_files += [patchfile.plaster, source]

        if files_with_conflicts:
            vscode_files += files_with_conflicts
            file_list = '\n'.join(f'    ✘ {file}'
                                  for file in files_with_conflicts)
            terminal.log_task(f'[bold]Manually resolve conflicts for '
                              f'{ACTION_NEEDED_DECORATOR}:[/]\n{file_list}')

        VsCodeIpcConnection().open_file(vscode_files)

        # The continuation file is updated at the end of the process, in case
        # the process has to be continued later.
        apply_record = ApplyPatchesRecord(
            patch_files=patch_files,
            patches_to_deleted_files=patches_to_deleted_files,
            files_with_conflicts=files_with_conflicts,
            broken_patches=broken_patches,
            plaster_fixed_patches=plaster_fixed_patches,
            plaster_broken_patches=plaster_broken_patches)

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
        with Path(versioning.PACKAGE_FILE).open('w',
                                                encoding='utf-8',
                                                newline='') as package_file:
            package_file.write(json.dumps(package, indent=2) + '\n')

        repository.brave.run_git('add', versioning.PACKAGE_FILE)

        # Pinlist timestamp update occurs with the package version update.
        _update_pinslist_timestamp()
        repository.brave.run_git('add', PINSLIST_TIMESTAMP_FILE)
        repository.brave.git_commit(
            f'Update from Chromium {self.base_version} '
            f'to Chromium {self.target_version}.')

    def _commit_pinned_patches_and_fixups(self,
                                          patch_paths: set[str],
                                          commit_message: str,
                                          no_verify: bool = False) -> None:
        """Commits patch paths, routing dev-cycle patches as fixups.

        For major version upgrades, inspects each patch's branch history. If
        the most-recent branch commit touching a patch is not a brockit upgrade
        commit, the patch is committed as a fixup! for that dev-cycle commit
        instead. The remaining patches are committed together under
        commit_message.
        """
        if self.is_major():
            up_to_git_ref = _solve_brave_ref('@previous-major')

            fixup_groups: dict[str, list[str]] = {}
            for patch_path in patch_paths:
                commits = repository.brave.run_git('log', '--pretty=%H %s',
                                                   f'{up_to_git_ref}..HEAD',
                                                   '--', patch_path)

                for line in commits.splitlines():
                    commit_hash, subject = line.split(' ', 1)
                    if not any(
                            subject.startswith(p)
                            for p in _UPGRADE_COMMIT_WITH_PATCHES_PREFIXES):
                        fixup_groups.setdefault(commit_hash,
                                                []).append(patch_path)
                        break

            fixup_patches: set[str] = set()
            for fixup_target, fixup_patch_paths in fixup_groups.items():
                for patch_path in fixup_patch_paths:
                    repository.brave.run_git('add', patch_path)
                    fixup_patches.add(patch_path)
                repository.brave.git_commit_fixup(fixup_target)

            patch_paths -= fixup_patches

        if patch_paths:
            repository.brave.run_git('add', *patch_paths)
            repository.brave.git_commit(commit_message, no_verify=no_verify)

    def _commit_plaster_fixed_patches(self, apply_record: ApplyPatchesRecord):
        """Commits patches that were fixed by plaster."""
        patch_paths = {
            path.as_posix()
            for path in apply_record.plaster_fixed_patches
        }
        # Adding no_verify to avoid issues if someone is using an old version
        # of the commit-msg hook that has not included this name as an
        # exception for the branch tag.
        self._commit_pinned_patches_and_fixups(
            patch_paths,
            f'Apply-fixed 🩹 patches from Chromium {self.base_version} '
            f'to Chromium {self.target_version}.',
            no_verify=True)

    def _run_update_patches(self) -> GitStatus:
        """Runs update_patches and returns the resulting GitStatus.

        This function is usually preferred, as it checks if any patches are
        deleted after running update_patches. Deleted patches should be
        committed manually with a history of why the patching is not required
        anymore.

        return:
          The GitStatus after running update_patches.
        """
        terminal.run_npm_command('update_patches', '--', '--no-plaster-check')

        status = GitStatus()
        if status.has_deleted_patch_files():
            raise InvalidInputException(
                'Deleted patches detected. These should be committed as their '
                'own changes:\n%s' %
                '\n'.join(status.staged.deleted + status.unstaged.deleted))
        if status.has_untracked_patch_files():
            raise InvalidInputException(
                'Untracked patch files detected. These should be committed as '
                'their own changes:\n%s' % '\n'.join(status.unstaged.added))
        if status.has_staged_files():
            raise InvalidInputException(
                'Staged files detected after running update_patches. Please '
                'make sure to commit or unstage any changes, to avoid '
                'committing changes unintentionally.\n'
                'Staged files:\n%s' %
                '\n'.join(status.get_all_staged_entries()))

        # The resulting updated patches should not be doing anything beyond
        # what they were doing already, both for "Update patches" and
        # "Conflict-resolved patches". Therefore, we check all the modified
        # patches to make sure that the number of hunks in these patch files
        # have not changed, as any significant change to a patchfile should be
        # submitted as its own change, with a culprit for visibility.
        all_modified = status.staged.modified + status.unstaged.modified
        modified_patches = [
            path for path in all_modified
            if path.startswith('patches/') and path.endswith('.patch')
        ]
        if not modified_patches:
            return status

        def count_hunks(contents: str) -> int:
            return contents.count('\n@@ -')

        patches_with_hunk_changes = []
        for patch in modified_patches:
            hunks_before = count_hunks(repository.brave.read_file(patch))
            hunks_after = count_hunks(
                (repository.brave.root / patch).read_bytes().decode('utf-8'))
            if hunks_before != hunks_after:
                patches_with_hunk_changes.append(
                    (patch, hunks_before, hunks_after))

        if patches_with_hunk_changes:
            list_str = '\n'.join([
                f'  * {patch}: {b} hunks before, {a} hunks after'
                for patch, b, a in patches_with_hunk_changes
            ])
            raise InvalidInputException(
                'The following modified patches have changes in the number of '
                'hunks, and are expected to be submitted separately as fixes '
                'with Chromium culprits:\n'
                f'{list_str}')

        return status

    def _prerun_checks(self) -> bool:
        """Runs pre-run checks for the upgrade.

    This function runs a series of checks to make sure the upgrade can proceed
    without any issues. If any advisories have been found, this function will
    print a summary that looks something like:

    * Pre-run advisory (attention needed)
        * The rust toolchain has been updated.
            CL: Roll clang+rust llvmorg-21-init-1655-g7b473dfe-1 : llvmorg-2...
                https://chromium.googlesource.com/chromium/src/+/f9fada98083846
            Run the jobs in https://ci.brave.com/view/toolchains/ to generate
            a new...

    Returns:
        True if all checks pass, and False otherwise.
        """
        # Fetching the tags between the current version and the target to check
        # for certain things that may have changed that require attention
        _ensure_chromium_tags(self.working_version, self.target_version)

        toolchains = (toolchain.WindowsToolchain(), toolchain.XcodeToolchain(),
                      toolchain.RustToolchain())
        advisories = []
        for tc in toolchains:
            advisory = tc.check(self.working_version, self.target_version)
            if advisory is None:
                continue
            # Some toolchain can be recovered before showing the advisory, but
            # kicking out the CI job and committing the latest toolchain update.
            if tc.recover(self.target_version, advisory.commit_hash):
                continue
            advisories.append(advisory)

        if not advisories:
            return True

        terminal.log_task(
            '[bold]Pre-run advisory ([bold yellow]attention needed[/])')
        for advisory in advisories:
            console.print(Padding(f'* {advisory.description}', (0, 15)))
            console.print(
                Padding(f'CL: [dim]{advisory.commit_message}', (0, 19)))
            console.print(
                Padding(
                    GOOGLESOURCE_COMMIT_LINK.format(
                        commit=advisory.commit_hash), (0, 23)))
            console.print(Padding(f'{advisory.advice}', (0, 19)))

        replace(ContinuationFile.load(target_version=self.target_version),
                has_shown_advisory=True).save()

        return False

    def _continue(self,
                  no_conflict_continuation: bool = False,
                  apply_record: ApplyPatchesRecord | None = None):
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

        if apply_record:
            apply_record.check_broken_plasters_fixed()

        update_status = self._run_update_patches()

        # `apply_records` is not guaranteed to exist for every continuation. In
        # some cases `_run_update_patches` is reponsible for pausing the lift
        # process (e.g. cherry-pick shows up in upstream and causes patch
        # deletions without 3way apply).
        conflict_resolved_patches = None
        if apply_record:
            # A list of all "Conflict-resolved" candidates still waiting to be
            # committed.
            conflict_resolved_patches = {
                patch.path.as_posix()
                for patch in apply_record.all_conflict_resolved_patches()
                if patch.path.as_posix() in update_status.unstaged.modified
            }

        if not conflict_resolved_patches and not no_conflict_continuation:
            raise InvalidInputException(
                'Nothing has been staged to commit conflict-resolved patches. '
                '(Did you mean to pass [bold cyan]--no-conflict-change[/]?)')

        if conflict_resolved_patches:
            # For major upgrades, patches last touched by dev-cycle commits
            # are routed as fixup! commits to avoid rebase conflicts when
            # running rebase --squash-minor-bumps.
            self._commit_pinned_patches_and_fixups(
                conflict_resolved_patches,
                f'Conflict-resolved patches from Chromium {self.base_version} '
                f'to Chromium {self.target_version}.')

        self._save_updated_patches()
        # Run init again to make sure nothing is missing after updating
        # patches.
        terminal.run_npm_command('init')

        terminal.run_npm_command('chromium_rebase_l10n')
        self._save_rebased_l10n()

        # With the continuation finished there's no need to keep the
        # continuation file around.
        ContinuationFile.clear()

    def _start(self, ack_advisory: bool):
        """Starts the upgrade process.

    This function is responsible for starting the upgrade process. It will
    update the package version, run `npm run init`, and then run
    `npm run update_patches`. If any patches fail to apply, it will run
    `apply_patches_3way` to allow for manual conflict resolution.

    For cases where no conflict resolution is required, the process will
    will continue, concluding the whole four steps of the upgrade process.

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

        if self.is_major():
            # When doing a major lift, the branch name should indicate that
            # (e.g. `cr149`).
            expected_branch = f'cr{self.target_version.major}'
            current_branch = repository.brave.current_branch()
            if current_branch != expected_branch:
                raise InvalidInputException(
                    f'Major version upgrades must be done on a branch named '
                    f'"{expected_branch}", but the current branch is '
                    f'"{current_branch}".')

        if not ack_advisory and not self._prerun_checks():
            raise ActionNeededException(
                '👋 (Address advisories and then rerun with '
                '[bold cyan]--ack-advisory[/])')

        self._update_package_version()

        try:
            terminal.run_npm_command('init')

            # When no conflicts come back, we can proceed with the
            # update_patches.
            self._run_update_patches()
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
                apply_record = self.apply_patches_3way()
                if apply_record.plaster_fixed_patches:
                    self._commit_plaster_fixed_patches(apply_record)
                if apply_record.requires_conflict_resolution():
                    # Manual resolution required.
                    raise ActionNeededException(
                        '👋 (Address all sections with '
                        f'{ACTION_NEEDED_DECORATOR} above, and then rerun '
                        '[italic]🚀Brockit![/] with [bold cyan]--continue[/])'
                    ) from e

                # With all conflicts resolved, it is necessary to close the
                # upgrade with all the same steps produced when running an
                # upgrade continuation, as recovering from a conflict-
                # resolution failure.
                self._continue(apply_record=apply_record)
                return
            if e.returncode != 0:
                raise InvalidInputException(
                    f'Failures found when running npm run init\n{e.stderr}'
                ) from e

        self._save_updated_patches()

        terminal.run_npm_command('chromium_rebase_l10n')
        self._save_rebased_l10n()

    def execute(self, no_conflict_continuation: bool, with_github: bool,
                ack_advisory: bool):
        """Executes the upgrade process.

    Keep in this function all code that is common to both start and continue.

    Args:
        no_conflict_continuation:
            Indicates that a continuation does not produce a conflict-resolved
            change.
        with_github:
            Indicates the user wants to create or update the github issue for
            the upgrade.
        """
        if self.target_version == self.working_version:
            raise InvalidInputException(
                f'This branch is already in {self.target_version}. (Maybe you '
                'meant to pass [bold cyan]--continue[/]?)')

        if self.target_version < self.working_version:
            raise InvalidInputException(
                f'Cannot upgrade version from {self.target_version} '
                f'to {self.working_version}')

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
                    raise InvalidInputException(
                        'Use [bold cyna]--ack-advisory[/] just after being '
                        'shown advisories.')
            # We initialise the continuation file here rather than in the
            # constructor to avoid overwritting the file if the user made the
            # mistake of calling brockit again without `--continue`.
            ContinuationFile(target_version=self.target_version,
                             working_version=self.working_version,
                             base_version=self.base_version).save()

        if with_github and not _is_gh_cli_logged_in():
            # Fail early if gh cli is not logged in.
            raise InvalidInputException('GitHub CLI is not logged in.')

        if self.is_continuation:
            if self.target_version != self.chromium_src_version:
                raise InvalidInputException(
                    'To run with [bold cyan]--continue[/] the Chromium '
                    'version has to be in Sync with Brave. Brave '
                    f'{self.target_version} ➜ '
                    f'Chromium {self.chromium_src_version}')

            self._continue(no_conflict_continuation=no_conflict_continuation)
        else:
            self._start(ack_advisory=ack_advisory)

        if with_github:
            GitHubIssue(base_version=self.base_version,
                        target_version=self.target_version
                        ).create_or_update_version_issue(with_pr=False)

        try:
            terminal.run([VPYTHON3_PATH, plaster.__file__, 'check'])
        except subprocess.CalledProcessError:
            terminal.log_task(
                '[bold]❌[/] Plaster check. Please investigate it.')

        try:
            self._save_gnrt_rerun()
        except subprocess.CalledProcessError:
            terminal.log_task('[bold]❌[/] GNRT rerun. Please investigate it.')


def _solve_brave_ref(from_ref: str | None) -> str:
    """Solves the git reference.

    This function is used to resolve the git reference provided by the user.
    This reference can be either a branch name, a commit hash, or a special
    label used by brockit to find specific changes.

    Args:
        from_ref:
            The git reference to resolve. If not provided, it defaults
            `@upstream`.

    Returns:
        The resolved git reference, if the reference is valid, or the
        reference for the solved special label:

        +-----------------+--------------------------------------------------+
        | Label           | Description                                      |
        +-----------------+--------------------------------------------------+
        | @upstream       | Upstream branch of the current branch            |
        | @previous       | Commit just before the last version bump         |
        | @previous-major | Commit just before the last major version bump   |
        +-----------------+--------------------------------------------------+
    """
    if not from_ref:
        from_ref = '@upstream'

    if from_ref and from_ref[0] != '@':
        # No special handling needed
        if not repository.brave.is_valid_git_reference(from_ref):
            raise InvalidInputException(
                'Value provided to [bold cyan]--from-ref[/] is not a valid '
                f'git ref: {from_ref}')
        return from_ref

    if from_ref == "@upstream":
        upstream_branch = _get_current_branch_upstream_name()
        if upstream_branch is None:
            raise InvalidInputException(
                'Could not determine the upstream branch. (Maybe set '
                '[bold cyan]--set-upstream-to[/] in your branch?)')
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
    def recommit_in_rebase_plan(todo_file: Path):
        """Recommits the first commit in the rebase plan.

    This function replaces the first `pick` in the rebase plan with `edit`,
    which forces the first commit to be recommitted.
        """
        contents = todo_file.read_bytes().decode('utf-8')
        todo_file.write_text(contents.replace('pick', 'edit', 1),
                             encoding='utf-8',
                             newline='')

    def execute(self, from_ref: str | None, to_ref: str | None, recommit: bool,
                discard_regen_changes: bool, squash_minor_bumps: bool) -> bool:
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
        to_ref = _solve_brave_ref(to_ref)

        if from_ref is None:
            if Version.from_git(to_ref).major != Version.from_git(
                    'HEAD').major:
                # If the major version is different, we default to the
                # previous major version.
                from_ref = _solve_brave_ref(from_ref or '@previous-major')
            else:
                # If the major version is the same, we default to the
                # previous version.
                from_ref = _solve_brave_ref(from_ref or '@previous')

        from_ref = _solve_brave_ref(from_ref)

        current_branch = repository.brave.current_branch()
        terminal.log_task(
            f'Rebasing {current_branch} onto {to_ref} starting from {from_ref}'
        )

        # We have to receive the rebase plan from git, and then modify it
        # if that's desired. That's done by calling this script again with
        # special internal flags.
        env = os.environ.copy()
        # Capture the user's preferred editors BEFORE we override
        # `GIT_SEQUENCE_EDITOR` / `GIT_EDITOR` below -- the editor fallback
        # flows through `--internal-rebase-crash-*-editor` and needs to know
        # what the user originally configured.
        crash_seq_editor = rebase.get_git_editor('GIT_SEQUENCE_EDITOR')
        crash_msg_editor = rebase.get_git_editor()
        editor = [str(VPYTHON3_PATH), __file__]
        # The plan-rewriting flags share the `--internal-rebase-plan-` prefix
        # so the dispatch can detect them with one check and consolidate both
        # into a single `rewrite_plan` call.
        if discard_regen_changes:
            editor.append('--internal-rebase-plan-discard-recyclable')
        if recommit:
            editor.append('--internal-rebase-recommit')
        if squash_minor_bumps:
            editor.append('--internal-rebase-plan-squash-pinned')

            # Squashes will cause `GIT_EDITOR` also to open for the commit
            # message, so we need to handle those too.
            env["GIT_EDITOR"] = (
                f'{str(VPYTHON3_PATH)} {__file__} '
                '--internal-rebase-fix-message '
                f'--internal-rebase-crash-msg-editor={crash_msg_editor}')

        if len(editor) > 2:
            # Pass the captured sequence-editor fallback alongside the plan
            # flags so the subprocess can hand off to it when
            # `EditorRecoverableFailure` fires.
            editor.append(
                f'--internal-rebase-crash-sequence-editor={crash_seq_editor}')
            env["GIT_SEQUENCE_EDITOR"] = " ".join(editor)
        else:
            # If there are no internal operation, we can just return always
            # true to whatever plan git gives us.
            env["GIT_SEQUENCE_EDITOR"] = 'cmd /c "exit 0"' if platform.system(
            ) == 'Windows' else 'true'

        try:
            # `interactive=True` as we may need to open an editor if
            # `EditorRecoverableFailure` is raised, so we can capture the pipes.
            terminal.run([
                'git', 'rebase', '--interactive', '--autosquash',
                '--empty=drop', '--onto', to_ref, from_ref, current_branch
            ],
                         env=env,
                         interactive=True)
            if recommit:
                repository.brave.run_git('commit', '--amend', '--no-edit')
                repository.brave.run_git('rebase', '--continue')
        except subprocess.CalledProcessError as e:
            raise InvalidInputException('Rebase failed.') from e


class _MarkChangeTask(Task):
    """Base for tasks that mark a change with an empty `<prefix><hash>!` commit.

    This class creates an empty commit with a subcommand that is understood by
    brockit's `rebase` command to mark a change in the branch.
    """

    # Abstract base: instantiated only through its concrete subclasses.
    # pylint: disable=abstract-method

    def __init__(self, prefix: str):
        # The `<prefix>` placed before the target hash (e.g. `reassign!`).
        self._prefix = prefix

    def status_message(self):
        # Derive the action word from the prefix, e.g. `drop!` → "drop".
        return f"Creating {self._prefix.rstrip('!')} commit..."

    def execute(self, change: str):
        """Creates the empty `<prefix><hash>! <subject>` commit.

    Args:
        change:
            The change to mark. This is any valid git reference that resolves
            to a single commit.
        """
        status = GitStatus()
        if status.has_staged_files():
            raise InvalidInputException(
                'Staged files detected. Please commit or unstage changes '
                'before marking the change:\n%s' %
                '\n'.join(status.get_all_staged_entries()))

        commit, message = repository.brave.run_git('log', '-1', change, '-s',
                                                   '--format=%h %s').split(
                                                       ' ', 1)
        repository.brave.git_commit(f"{self._prefix}{commit}! {message}",
                                    allows_empty=True,
                                    no_verify=True)


class Reassign(_MarkChangeTask):
    """Creates a reassignment commit for a given change in the branch.

    This task is used for cases where the authorship of a given change should be
    reassigned to a different author. The empty `reassign!` commit it creates is
    picked up by brockit's `rebase` command and squashed as the base commit for
    the original change, resulting in a change of authorship.
    """

    def __init__(self):
        super().__init__(REASSIGN_COMMIT_MSG_PREFIX)


class Drop(_MarkChangeTask):
    """Creates a drop commit for a given change in the branch.

    This task is used to create a `drop!` commit, which gets picked up by the
    rebase engine, and causes the target change to be dropped during rebase.
    """

    def __init__(self):
        super().__init__(DROP_COMMIT_MSG_PREFIX)


class _RepinToolchainTask(Task):
    """A this wrapper around `toolchain.Toolchain.repin`.

     This class provides access to the toolchains `repin` method, which updates
     the toolchain version in the repository.
    """

    def __init__(self, target_toolchain: toolchain.Toolchain):
        self._toolchain = target_toolchain

    def status_message(self) -> str:
        return f'Updating the {self._toolchain.spec.label} toolchain...'

    def execute(self, chromium_ref: str, culprit: str | None):
        version = _fetch_chromium_tag(chromium_ref)
        _ensure_chromium_tags(str(version))
        self._toolchain.repin(version, culprit)


class _GenToolchainTask(Task):
    """Resolves the Chromium tag and triggers a toolchain's CI job(s).

    A thin wrapper to kick out the toolchain's CI jobs.
    """

    def __init__(self, target_toolchain: toolchain.Toolchain, **properties):
        self._toolchain = target_toolchain
        self._properties = properties

    def status_message(self) -> str:
        return f'Triggering {self._toolchain.spec.label} builds...'

    def execute(self, tag: str):
        self._toolchain.trigger(_fetch_chromium_tag(tag),
                                watch=False,
                                **self._properties)

    def run_watching(self, tag: str) -> None:
        """Entry point for `--watch` that bypasses `Task.run`'s spinner."""
        console.log(self.start_banner)
        self._toolchain.trigger(_fetch_chromium_tag(tag),
                                watch=True,
                                **self._properties)
        console.log(self.end_banner)


def fetch_chromium_dash_version(channel: str) -> Version:
    """Fetches the highest latest version across all platforms for a channel.

    For the canary channel, the ASAN variant on Windows is also considered.
    """

    def _fetch(channel: str, target_platform: str) -> Version:
        response = requests.get(CHROMIUMDASH_LATEST_RELEASE.format(
            channel=channel, platform=target_platform),
                                timeout=10)
        return Version(response.json()[0].get('version'))

    platforms = ('Windows', 'Linux', 'Android', 'Mac', 'ios')
    versions = [_fetch(channel, platform) for platform in platforms]
    if channel == 'canary':
        versions.append(_fetch('canary_asan', target_platform='Windows'))
    return max(versions)


def _fetch_chromium_tag(to: str) -> Version:
    """Resolves the --to flag value to a Version.

    +---------------------------+---------------------------------------------+
    | Labels                    | Description                                 |
    +---------------------------+---------------------------------------------+
    | @latest-tag               | Latest tag in the Chromium repo             |
    | @latest-m{MAJOR}          | Latest tag for the given major version      |
    | @latest-for-branch        | Latest tag for the major inferred from the  |
    |                           | current branch name ( format cr{MAJOR})     |
    | @latest-canary            | Latest canary release from ChromiumDash     |
    | @latest-beta              | Latest beta release from ChromiumDash       |
    | @latest-dev               | Latest dev release from ChromiumDash        |
    | @latest-stable            | Latest stable release from ChromiumDash     |
    +---------------------------+---------------------------------------------+
    """

    # If not a label, then this value is expected to be a valid Chromium
    # version.
    if not to.startswith('@'):
        return Version(to)

    if to == '@latest-for-branch':
        branch = repository.brave.current_branch()
        match = re.fullmatch(r'cr(\d+)', branch)
        if not match:
            raise InvalidInputException(
                '@latest-for-branch requires the current branch to be named '
                f'cr{{MAJOR}} (e.g. cr135), but the current branch is '
                f'"{branch}".')
        return _fetch_chromium_tag(f'@latest-m{match.group(1)}')

    if to == '@latest-tag':
        version = Version.get_latest_googlesource_tag_version()
        if version is None:
            raise InvalidInputException(
                'Could not fetch latest Googlesource tag.')
        return version
    if to.startswith('@latest-m'):
        major_str = to[len('@latest-m'):]
        if not major_str.isdigit():
            raise InvalidInputException(
                f'Invalid major version in "{to}": '
                f'"{major_str}" is not a valid integer.')
        version = Version.get_latest_googlesource_tag_version(
            major=int(major_str))
        if version is None:
            raise InvalidInputException(
                'Could not find a Googlesource tag for major version '
                f'{major_str}.')
        return version
    if to.startswith('@latest-'):
        [_, channel] = to.split('-', 1)
        if channel not in ('canary', 'beta', 'dev', 'stable'):
            raise InvalidInputException(
                f'Invalid @latest channel: "{channel}". '
                'Valid options: canary, beta, dev, stable.')

        return fetch_chromium_dash_version(channel)

    raise InvalidInputException(
        f'Unknown label: "{to}". '
        'Valid labels: @latest-tag, @latest-m{MAJOR}, @latest-for-branch, '
        '@latest-canary, @latest-beta, @latest-dev, @latest-stable.')


def show(args: argparse.Namespace):
    """Prints various insights about brave-core.

    This is a helper command line that allows us to inspect a few things about
    brave-core and how brockit process things.
    """
    if args.package_version:
        console.print(f'upstream version: {Version.from_git("HEAD")}')

    if args.from_ref_value is not None:
        from_ref_value = Version.from_git(_solve_brave_ref(
            args.from_ref_value))
        if from_ref_value is not None:
            console.print(f'base version: {from_ref_value}')

    if args.log_link:
        console.print('googlesource link: %s' %
                      Version.from_git('HEAD').get_googlesource_diff_link(
                          Version.from_git(_solve_brave_ref('@previous'))))

    if args.chromium_version_label is not None:
        console.print('version: %s' %
                      _fetch_chromium_tag(args.chromium_version_label))


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
    base_version_parser = argparse.ArgumentParser(
        add_help=False, formatter_class=argparse.RawTextHelpFormatter)
    base_version_parser.add_argument(
        '--from-ref',
        help=
        ('A brave-core git reference for the Chromium version to upgrade\n'
         'from (branch, commit hash, tag, etc.), or one of these labels:\n'
         '  @upstream        Upstream branch of the current branch (default)\n'
         '  @previous        Commit just before the last version bump\n'
         '  @previous-major  Commit just before the last major version bump'),
        default=None)

    parser = argparse.ArgumentParser()

    subparsers = parser.add_subparsers(dest='command', required=True)
    lift_parser = subparsers.add_parser(
        'lift',
        parents=[global_parser, base_version_parser],
        formatter_class=argparse.RawTextHelpFormatter,
        help='Upgrade the chromium base version. Special tags: '
        '@latest-[beta|dev|canary] pulls the version from chromium dash; '
        '@latest-tag pulls the latest tag from Googlesource.')
    lift_parser.add_argument(
        '--to',
        required=True,
        help=(
            'The Chromium version to upgrade to (e.g. 147.0.7727.117),\n'
            'or one of the following labels:\n'
            '  @latest-tag         Latest tag from the Chromium Googlesource'
            ' repo\n'
            '  @latest-m{MAJOR}    Latest Googlesource tag for the given major'
            ' version\n'
            '  @latest-for-branch  Latest tag for the major inferred from the\n'
            '                      current branch name (must be named'
            ' cr{MAJOR})\n'
            '  @latest-canary      Latest canary release from ChromiumDash\n'
            '  @latest-beta        Latest beta release from ChromiumDash\n'
            '  @latest-dev         Latest dev release from ChromiumDash\n'
            '  @latest-stable      Latest stable release from ChromiumDash'),
    )
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
        '--no-conflict-change',
        action='store_true',
        help='Indicates that a continuation does not have conflict patches to '
        'commit any longer.',
        dest='no_conflict')

    regen_parser = subparsers.add_parser(
        'regen',
        parents=[global_parser, base_version_parser],
        help='Regenerates all patches and strings for the current branch.')
    regen_parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Nothing is commited to git with this flag.')

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
        '--chromium-version-label',
        default=None,
        help='Prints the version for the given label (e.g. @latest-canary).')

    reassign_parser = subparsers.add_parser(
        'reassign',
        parents=[global_parser],
        help=(
            f'Creates a {REASSIGN_COMMIT_MSG_PREFIX} commit for a given change '
            'to change authorship to current user.'))
    reassign_parser.add_argument(
        'change',
        help='The commit reference to reassign (hash, HEAD~N, etc.).')

    drop_parser = subparsers.add_parser(
        'drop',
        parents=[global_parser],
        help=(f'Creates a {DROP_COMMIT_MSG_PREFIX} commit marking a given '
              'change to be dropped during rebase.'))
    drop_parser.add_argument(
        'change', help='The commit reference to drop (hash, HEAD~N, etc.).')

    update_xcode_parser = subparsers.add_parser(
        'update-xcode-toolchain',
        parents=[global_parser],
        formatter_class=argparse.RawTextHelpFormatter,
        help='Pins build/mac/download_hermetic_xcode.py to the published '
        'hermetic Xcode toolchain for a Chromium tag\'s macOS SDK, and commits '
        'the change.')
    update_xcode_parser.add_argument(
        '--to',
        required=True,
        dest='to',
        help=(
            'The Chromium version whose pinned macOS SDK to repin against\n'
            '(e.g. 150.0.7850.1), or one of the @latest-* labels accepted by\n'
            '`lift --to` (e.g. @latest-canary, @latest-m150,\n'
            '@latest-for-branch, @latest-tag).'))
    update_xcode_parser.add_argument(
        '--culprit',
        default=None,
        help='Chromium commit hash to reference in the commit body. Defaults '
        'to auto-detecting the culprit.',
        dest='culprit')

    def _add_gen_parser(command: str, description: str):
        """Adds a `gen-*-toolchain` subparser (a `tag` positional + `--watch`).

        All three toolchains trigger their CI job(s) through the same launcher,
        so their subparsers share the same shape.
        """
        parser_ = subparsers.add_parser(
            command,
            parents=[global_parser],
            formatter_class=argparse.RawTextHelpFormatter,
            help=description)
        parser_.add_argument(
            'tag',
            help=('The Chromium version to build the toolchain for (e.g.\n'
                  '150.0.7850.1).'))
        parser_.add_argument(
            '--watch',
            action='store_true',
            help='After triggering, show a live-updating table of each '
            'pipeline.')
        return parser_

    gen_rust_parser = _add_gen_parser(
        'gen-rust-toolchain',
        'Triggers the Rust/WASM toolchain Jenkins pipelines.')
    gen_rust_parser.add_argument(
        '--brave-subrevision',
        type=int,
        required=True,
        dest='brave_subrevision',
        help=(
            'The Brave subrevision, if a respin is required for the Rust/WASM'
            'toolchain.'))
    _add_gen_parser('gen-xcode-toolchain',
                    'Triggers the hermetic Xcode toolchain Jenkins pipeline.')
    _add_gen_parser(
        'gen-windows-toolchain',
        'Triggers the hermetic Windows toolchain Jenkins pipeline.')

    update_rust_parser = subparsers.add_parser(
        'update-rust-wasm-toolchain',
        parents=[global_parser],
        formatter_class=argparse.RawTextHelpFormatter,
        help='Repins the Rust/WASM toolchain objects in '
        'tools/cr/install_extra_deps.py to the latest published '
        'archives for a Chromium tag\'s Rust+Clang revision, and commits the '
        'change.')
    update_rust_parser.add_argument(
        '--to',
        required=True,
        dest='to',
        help=(
            'The Chromium version whose Rust+Clang revision to repin against\n'
            '(e.g. 150.0.7850.1), or one of the @latest-* labels accepted by\n'
            '`lift --to` (e.g. @latest-canary, @latest-m150,\n'
            '@latest-for-branch, @latest-tag).'))
    update_rust_parser.add_argument(
        '--culprit',
        default=None,
        help='Chromium commit hash to reference in the commit body. Defaults '
        'to the last Chromium commit touching the Rust/Clang revision '
        '(tools/rust/update_rust.py, tools/clang/scripts/update.py).',
        dest='culprit')

    subparsers.add_parser('reference',
                          help='Detailed documentation for this tool.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if is_verbose() else logging.INFO,
                        format='%(message)s',
                        handlers=[IncendiaryErrorHandler()],
                        force=True)

    if hasattr(args, 'infra_mode') and args.infra_mode:
        terminal.set_infra_mode()

    if hasattr(args, 'from_ref'):
        if args.command == 'lift' and args.is_continuation:
            if args.from_ref is not None:
                parser.error(
                    'Switch --from-ref not supported with --continue.')

    def resolve_version_with_from_ref_arg() -> Version:
        return Version.from_git(_solve_brave_ref(args.from_ref))

    if args.command == 'lift' and args.no_conflict and not args.is_continuation:
        parser.error('--no-conflict-change can only be used with --continue')
    if args.command == 'lift' and args.restart and args.is_continuation:
        parser.error('--restart does not support --continue')
    if args.command == 'lift' and args.ack_advisory and args.is_continuation:
        parser.error('--ack-advisory does not support --continue')
    try:
        if args.command == 'lift':
            target = _fetch_chromium_tag(args.to)
            if args.restart:
                ReUpgrade(target).run()

            if not args.is_continuation:
                upgrade = Upgrade(target, args.is_continuation,
                                  resolve_version_with_from_ref_arg())
            else:
                upgrade = Upgrade(_fetch_chromium_tag(args.to),
                                  args.is_continuation)

            upgrade.run(no_conflict_continuation=args.no_conflict,
                        with_github=args.with_github,
                        ack_advisory=args.ack_advisory)
        if args.command == 'rebase':
            Rebase().run(from_ref=args.from_ref,
                         to_ref=args.to_ref,
                         recommit=args.recommit,
                         discard_regen_changes=args.discard_regen_changes,
                         squash_minor_bumps=args.squash_minor_bumps)
        if args.command == 'regen':
            Regen(
                resolve_version_with_from_ref_arg()).run(dry_run=args.dry_run)
        if args.command == 'update-version-issue':
            GitHubIssue(resolve_version_with_from_ref_arg()).run()
        if args.command == 'reassign':
            Reassign().run(change=args.change)
        if args.command == 'drop':
            Drop().run(change=args.change)
        if args.command == 'update-xcode-toolchain':
            _RepinToolchainTask(toolchain.XcodeToolchain()).run(
                chromium_ref=args.to, culprit=args.culprit)
        if args.command == 'update-rust-wasm-toolchain':
            _RepinToolchainTask(toolchain.RustToolchain()).run(
                chromium_ref=args.to, culprit=args.culprit)
        gen_toolchains = {
            'gen-rust-toolchain': toolchain.RustToolchain,
            'gen-xcode-toolchain': toolchain.XcodeToolchain,
            'gen-windows-toolchain': toolchain.WindowsToolchain,
        }
        if args.command in gen_toolchains:
            properties = ({
                'brave_subrevision': args.brave_subrevision
            } if args.command == 'gen-rust-toolchain' else {})
            task = _GenToolchainTask(gen_toolchains[args.command](),
                                     **properties)
            if args.watch:
                # `--watch` provides live updates, therefore it doesn't use the
                # spinner provided by `Task.run`.
                task.run_watching(tag=args.tag)
            else:
                task.run(tag=args.tag)
        if args.command == 'reference':
            console.print(Markdown(__doc__))
        if args.command == 'show':
            show(args)
    except (InvalidInputException, BadOutcomeException, ActionNeededException):
        return 1

    return 0


if __name__ == '__main__':
    if any(arg.startswith("--internal-rebase") for arg in sys.argv):
        # Special flags used to carry out some of the rebase tasks fed by git
        # during rebase --interactive mode.
        if '--internal-rebase-recommit' in sys.argv:
            Rebase.recommit_in_rebase_plan(Path(sys.argv[-1]))

        def _crash_editor_from_argv(flag_prefix: str) -> str:
            """Pulls a `--<flag_prefix>=<editor>` value out of `sys.argv`.

            Raises `NotImplementedError` when the flag is absent or its
            value is empty -- `Rebase.execute` appends this flag
            unconditionally for these rebase steps, so a missing value
            means a wiring bug rather than something recoverable at
            runtime.
            """
            prefix = f'{flag_prefix}='
            for arg in sys.argv:
                if arg.startswith(prefix):
                    value = arg[len(prefix):]
                    if value:
                        return value
                    break
            raise NotImplementedError(
                f'Expected --{flag_prefix}=<editor> in sys.argv but '
                f'none was found (or it was empty). `Rebase.execute` '
                f'should have appended it for this dispatch.')

        if any(a.startswith('--internal-rebase-plan-') for a in sys.argv):
            editor_path = Path(sys.argv[-1])
            crash_editor = _crash_editor_from_argv(
                '--internal-rebase-crash-sequence-editor')
            try:
                rebase.rewrite_plan(
                    todo_file=editor_path,
                    discard_recyclable=(
                        '--internal-rebase-plan-discard-recyclable'
                        in sys.argv),
                    pinned_squashed=('--internal-rebase-plan-squash-pinned'
                                     in sys.argv))
            except rebase.EditorRecoverableFailure as e:
                rebase.hand_off_to_editor(editor_path,
                                          reason=str(e),
                                          editor=crash_editor)
        if '--internal-rebase-fix-message' in sys.argv:
            editor_path = Path(sys.argv[-1])
            crash_editor = _crash_editor_from_argv(
                '--internal-rebase-crash-msg-editor')
            try:
                writer = rebase.MessageWriter.parse(editor_path)
                writer.rewrite_with_last_message()
            except rebase.EditorRecoverableFailure as e:
                rebase.hand_off_to_editor(editor_path,
                                          reason=str(e),
                                          editor=crash_editor)
        sys.exit(0)

    sys.exit(main())
