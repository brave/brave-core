# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

from pathlib import Path
import json
import os
import subprocess
import tempfile

CHROME_VERSION_TEMPLATE: str = """MAJOR={major}
MINOR={minor}
BUILD={build}
PATCH={patch}
"""

BRAVE_ROOT_FROM_FILE = Path(__file__).resolve().parents[3]

# The branch every fake repository is initialised with. Named explicitly
# because `init.defaultBranch` varies from machine to machine, and tests need
# to be able to name the branch they are pushing to or rebasing onto.
DEFAULT_BRANCH = 'master'

# The marker line `run_chromium_rebase_l10n` and `run_gnrt` stamp their
# generated files with. Real string rebases and `gnrt` runs regenerate their
# outputs from the Chromium tree currently synced, so the stamp is what makes
# these files change from one lift to the next.
L10N_VERSION_STAMP = '<!-- Generated for Chromium {version} -->'
GNRT_VERSION_STAMP = '# Generated for Chromium {version}'


class FakeChromiumRepo:
    """A fake Chromium repository for testing purposes."""

    def __init__(self) -> None:
        """Initializes the fake Chromium repository.

        Creates a temporary directory and initializes a fake Chromium repository
        with a `src` directory. Also creates a `brave` repository inside `src`.
        """
        # Every repository initialised by this fixture, in creation order.
        self._repos: list[Path] = []

        self.temp_dir: tempfile.TemporaryDirectory = (
            tempfile.TemporaryDirectory())
        # Resolve the temp dir so derived paths are symlink-canonical. On
        # macOS, tempfile returns paths under /var/folders/..., but /var is a
        # symlink to /private/var; without resolving here, equality checks
        # against `Repository.root.resolve()` (which follows the symlink) fail.
        self.base_path: Path = Path(self.temp_dir.name).resolve() / 'workspace'
        self._init_repo(self.chromium)

        # Set a brave repository under src/.
        self._init_repo(self.brave)
        self.brave_patches.mkdir(parents=True, exist_ok=True)
        self._original_cwd: Path | None = None

    def setup(self) -> None:
        """Creates chromium_src/ and rewrite/, then chdirs into the brave repo.

        Pair with `cleanup()` to restore the original cwd. Must be called
        before any tools/cr code that resolves the brave-core root against
        cwd.
        """
        (self.brave / 'chromium_src').mkdir(exist_ok=True)
        (self.brave / 'rewrite').mkdir(exist_ok=True)
        (self.brave / 'patches').mkdir(exist_ok=True)

        # `FakeChromiumRepo` will change the current directory to a mirro path
        # inside the fake brave repo, relative to the cwd in brave-core when
        # launched. This is intentional, although it means that tests run from
        # different cwds will see different relatative paths for brave-core
        # root, and chromium root. This should always work, as none of the code
        # under tools/cr should be calling `chdir`, and it allows us to check
        # that code works correctly no matter from where it was launched.
        self._original_cwd = Path.cwd()
        brave_root = BRAVE_ROOT_FROM_FILE
        original_cwd = self._original_cwd.resolve()
        try:
            rel_from_brave = original_cwd.relative_to(brave_root)
        except ValueError:
            raise ValueError(
                'FakeChromiumRepo.setup() must be called from within the '
                f'brave-core tree ({brave_root}).') from None
        fake_cwd = self.brave / rel_from_brave
        fake_cwd.mkdir(parents=True, exist_ok=True)
        os.chdir(fake_cwd)

    @property
    def chromium(self) -> Path:
        """Returns the path to the Chromium source directory."""
        return self.base_path / 'src'

    @property
    def brave(self) -> Path:
        """Returns the path to the Brave directory"""
        return self.chromium / 'brave'

    @property
    def brave_patches(self) -> Path:
        """Returns the path to the Brave patches directory."""
        return self.brave / 'patches'

    @property
    def remote(self) -> Path:
        """Returns the path to the Brave directory"""
        return self.base_path / 'remote'

    @property
    def chromium_repos(self) -> list[Path]:
        """Every Chromium-side repository, in creation order.

        This is `src/` plus any repository added with `add_repo`/`add_dep`,
        which together are the repositories `apply_patches` and
        `update_patches` operate on. `brave/` (whose changes are patches, not
        patched sources) and the push remote are excluded.
        """
        return [
            path for path in self._repos
            if path != self.brave and path.is_relative_to(self.chromium)
        ]

    def _run_git_command(self,
                         command: list[str],
                         cwd: Path,
                         strip: bool = True) -> str:
        """Runs a git command in the specified directory and returns the stdout.

        Args:
            command: The git command to execute as a list of strings.
            cwd: The directory in which to run the command.
            strip: Whether to strip the output.

        Returns:
            The stdout output of the git command as a string.
        """
        result = subprocess.check_output(['git'] + command,
                                         cwd=cwd,
                                         stderr=subprocess.DEVNULL,
                                         text=True)
        if strip:
            return result.strip()
        return result

    def _init_repo(self, path: Path) -> None:
        """Initializes a git repository at the specified path.

        Creates a `README.md` file, stages it, and makes an initial commit.

        Args:
            path: The path where the repository should be initialized.
        """
        path.mkdir(parents=True, exist_ok=True)
        self._run_git_command(['init', '-b', DEFAULT_BRANCH], path)
        self._run_git_command(['config', 'core.autocrlf', 'false'], path)
        (path / 'README.md').write_text(f'# Fake {path.name} repo\n')
        self._run_git_command(['add', 'README.md'], path)
        self._run_git_command(['config', 'user.name', 'Fake User'], path)
        self._run_git_command(['config', 'user.email', 'fake@brave.com'], path)
        # Disable background gc to avoid failures cleaning up these repos during
        # teardown.
        for key in ('gc.auto', 'gc.autodetach', 'gc.autopacklimit'):
            self._run_git_command(['config', key, '0'], path)
        self._run_git_command(['commit', '-m', 'Initial commit'], path)
        self._repos.append(path)

    def create_brave_remote(self) -> None:
        """Creates a remote repository for Brave and sets it as the origin.

        Initializes a git repository at the path returned by `self.remote` and
        adds it as the `origin` remote for the Brave repository.
        """
        # Initialize the remote repository
        self._init_repo(self.remote / 'brave')

        # Add the remote as 'origin' for the Brave repository
        self._run_git_command(
            ['remote', 'add', 'origin',
             str(self.remote / 'brave')], self.brave)

    def add_repo(self, relative_path: str) -> None:
        """Adds a new repository at the specified relative path.

        Args:
            relative_path: The relative path for the new repository.
        """
        repo_path: Path = self.chromium / relative_path
        self._init_repo(repo_path)

    def add_dep(self, relative_path: str) -> None:
        """Adds a dependency as a git submodule.

        Initializes a repository at the specified path and adds it as a
        submodule to the main repository.

        Args:
            relative_path: The relative path of the dependency to add.
        """
        dep_path: Path = self.chromium / relative_path
        self._init_repo(dep_path)
        self._run_git_command(
            ['submodule', 'add',
             str(dep_path), relative_path], self.chromium)
        self._run_git_command(
            ['commit', '-m', f'Add submodule {relative_path}'], self.chromium)

    def add_tag(self, version: str) -> None:
        """Adds a git tag to the repository.

        Creates a `VERSION` file with the specified version and commits it,
        then tags the commit with the version.

        Args:
            version: The version string in the format `MAJOR.MINOR.BUILD.PATCH`.
        """
        major, minor, build, patch = version.split('.')
        version_file: Path = self.chromium / 'chrome' / 'VERSION'
        version_file.parent.mkdir(parents=True, exist_ok=True)
        version_file.write_text(
            CHROME_VERSION_TEMPLATE.format(major=major,
                                           minor=minor,
                                           build=build,
                                           patch=patch))
        self._run_git_command(['add', str(version_file)], self.chromium)
        self._run_git_command(['commit', '-m', f'VERSION {version}'],
                              self.chromium)
        self._run_git_command(['tag', version, '-m', f'VERSION {version}'],
                              self.chromium)

    def commit_empty(self, commit_message: str, repo_path: Path) -> str:
        """Creates an empty commit for a repository and returns a hash.

        Args:
            commit_message: The message to use for the empty commit.
            repo_path: The repository path.

        Returns:
            The hash of the commit made.
        """
        self._run_git_command(
            ['commit', '--allow-empty', '-m', commit_message], repo_path)
        return self._run_git_command(['rev-parse', 'HEAD'], repo_path)

    def commit(self, commit_message: str, repo_path: Path) -> str:
        """Creates commit in the passed repository and returns a hash.

        Args:
            commit_message: The message to use for the commit.
            repo_path: The repository path.

        Returns:
            The hash of the commit made.
        """
        self._run_git_command(['commit', '-m', commit_message], repo_path)
        return self._run_git_command(['rev-parse', 'HEAD'], repo_path)

    def write_file(self, relative_path: str, content: str,
                   repo_path: Path) -> Path:
        """Writes a file in a repository without staging it.

        This is how a patched source looks in a synced checkout: the change
        lives in the working tree only.

        Args:
            relative_path: The relative path of the file to write.
            content: The content to write to the file.
            repo_path: The path to the repository.

        Returns:
            The full path of the file written.
        """
        file_path = repo_path / relative_path
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text(content, encoding='utf-8', newline='')
        return file_path

    def write_and_stage_file(self, relative_path: str, content: str,
                             repo_path: Path) -> None:
        """Writes content to a file and stages it in the specified repository.

        Args:
            relative_path: The relative path of the file to write.
            content: The content to write to the file.
            repo_path: The path to the repository.
        """
        file_path = repo_path / relative_path
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text(content, newline='\n')
        self._run_git_command(['add', str(file_path)], repo_path)

    def delete_file(self, relative_path: str, repo_path: Path) -> None:
        """Deletes a file and stages the deletion in the specified repository.

        Args:
            relative_path: The relative path of the file to delete.
            repo_path: The path to the repository.
        """
        file_path = repo_path / relative_path
        if file_path.exists():
            file_path.unlink()
        self._run_git_command(['add', str(file_path)], repo_path)

    def update_brave_version(self, version: str) -> str:
        """Updates the Brave version in package.json and commits the change.

        Args:
            version: The new version string to set.

        Returns:
            The hash of the commit made.
        """
        package_json_path = self.brave / 'package.json'
        old_version = None

        # Check if package.json exists and read the old version if present
        if package_json_path.exists():
            with package_json_path.open('r') as f:
                package_data = json.load(f)
                old_version = package_data.get('config',
                                               {}).get('projects',
                                                       {}).get('chrome',
                                                               {}).get('tag')
        else:
            package_data = {}

        # Update the version in the JSON structure
        package_data.setdefault('config',
                                {}).setdefault('projects', {}).setdefault(
                                    'chrome', {})['tag'] = version

        # Write the updated JSON back to package.json
        with package_json_path.open('w') as f:
            json.dump(package_data, f, indent=2)

        # Stage the file and commit the change
        self._run_git_command(['add', str(package_json_path)], self.brave)
        commit_message = (
            f'Update from Chromium {old_version or "N/A"} to Chromium {version}'
        )
        self._run_git_command(['commit', '-m', commit_message], self.brave)

        # Return the hash of the commit
        return self._run_git_command(['rev-parse', 'HEAD'], self.brave)

    def run_update_patches(self) -> None:
        """Emulates `npm run update_patches`.

        Follows `build/commands/lib/updatePatches.js`: for every Chromium-side
        repository, each *modified* tracked file (`--diff-filter=M`) has its
        patch (re)written into that repository's patch directory, and any patch
        file whose source is no longer modified is deleted as stale. Files that
        are merely added or deleted in the working tree produce no patch, just
        like the real command.
        """
        for repo_path in self.chromium_repos:
            # Find every tracked file modified in the tree. Submodules are
            # ignored because a dependency repo moving ahead of the gitlink
            # recorded in `src/` is not a patched source.
            modified_files = self._run_git_command([
                'diff', '--ignore-submodules', '--diff-filter=M',
                '--name-only', '--ignore-space-at-eol'
            ], repo_path).splitlines()

            # Determine the relative path of the repo to Chromium
            relative_repo_path = repo_path.relative_to(self.chromium)
            patches_dir = self.brave_patches / relative_repo_path
            if modified_files:
                patches_dir.mkdir(parents=True, exist_ok=True)

            written: set[str] = set()
            for filename in modified_files:
                # Generate the patch file path
                patch_file = self.brave / self.get_patchfile_path_for_source(
                    relative_repo_path, Path(filename))
                written.add(patch_file.name)

                # Generates the patch file for the changed file. This is an
                # ad-hoc call to `git diff`, capturing the full binary output,
                # to prevent string encoding from dropping any carriage
                # returns from the diff file.
                result = subprocess.run(
                    [
                        'git', 'diff', '--src-prefix=a/', '--dst-prefix=b/',
                        '--default-prefix', '--full-index',
                        '--ignore-space-at-eol', filename
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    check=True,
                    cwd=repo_path,
                )
                patch_file.write_bytes(result.stdout)

            # Patches whose source is no longer modified are stale, and the
            # real command removes them. This is what turns an upstream
            # deletion of a patched source into a deleted patch file.
            if patches_dir.is_dir():
                for stale in sorted(patches_dir.glob('*.patch')):
                    if stale.name not in written:
                        stale.unlink()

    def commit_patches(self, message: str = 'Add patches') -> str:
        """Regenerates every patch file and commits the result in brave.

        This is how patched sources end up represented in brave's history:
        `update_patches` writes the patch files out, and all of them (including
        additions and deletions) are committed together.

        Args:
            message: The commit message to use.

        Returns:
            The hash of the commit made, or of `HEAD` when there was nothing to
            commit.
        """
        self.run_update_patches()
        self._run_git_command(
            ['add', '--all', str(self.brave_patches)], self.brave)
        if self._run_git_command(['diff', '--cached', '--name-only'],
                                 self.brave) == '':
            return self._run_git_command(['rev-parse', 'HEAD'], self.brave)
        return self.commit(message, self.brave)

    def get_patchfile_path_for_source(self, repo_path: Path,
                                      filename: Path) -> Path:
        """Generates the patch file path for a given source file.

        Args:
            relative_repo_path: The relative path to the repository from
                the Chromium source directory, which will also serve as a sub-
                directory in the patches directory.
            filename: The name of the source file.

        Returns:
            The full path to the patch file.
        """
        if repo_path.is_absolute():
            repo_path = repo_path.relative_to(self.chromium)
        return (self.brave_patches / repo_path /
                f'{filename.as_posix().replace("/", "-")}.patch').relative_to(
                    self.brave)

    def _patch_sources(self, patch_file: Path,
                       target_repo_path: Path) -> list[str]:
        """The repo-relative paths a patch file applies to.

        Read from the patch itself with `git apply --numstat`, the same way
        `gitPatcher.getAppliesTo` does. Raises `subprocess.CalledProcessError`
        when the patch cannot be parsed at all.
        """
        numstat = self._run_git_command(
            ['apply', '--numstat', str(patch_file)], target_repo_path)
        return [
            line.split('\t')[2] for line in numstat.splitlines()
            if '\t' in line
        ]

    def run_apply_patches(self) -> list[dict]:
        """Emulates `npm run apply_patches`.

        Follows `build/commands/lib/gitPatcher.js`: the sources a patch applies
        to are read from the patch file and reset before applying, patches
        whose source is gone are reported without being applied at all (which
        is how a source deleted or renamed upstream is surfaced), and each
        patch is applied with the same `git apply` flags the real command uses.

        Returns:
            One entry per failed patch, shaped exactly like the entries
            `printFailedPatchesInJsonFormat` writes out.
        """
        if not self.brave_patches.exists():
            raise FileNotFoundError(
                f'Patches directory {self.brave_patches} does not exist.')

        failed_patches = []

        for patch_file in sorted(self.brave_patches.rglob('*.patch')):
            # Using the relative path of the patch file to determine the target
            # repository path.
            relative_repo_path = patch_file.relative_to(
                self.brave_patches).parent
            target_repo_path = self.chromium / relative_repo_path

            if not (target_repo_path / '.git').exists():
                raise FileNotFoundError(
                    f'Target repository {target_repo_path} does not exist.')

            failure = {
                'patchPath': str(patch_file.relative_to(self.brave)),
                'path': None,
                'reason': 'PATCH_CHANGED',
            }

            try:
                sources = self._patch_sources(patch_file, target_repo_path)
            except subprocess.CalledProcessError:
                # An unreadable patch file is a failure of its own, and no
                # source can be named for it.
                failed_patches.append(failure)
                continue

            missing = [
                source for source in sources
                if not (target_repo_path / source).exists()
            ]
            if missing:
                # Patches to sources that are gone are never handed to
                # `git apply`, as an early bail-out there would skip every
                # patch listed after them.
                failed_patches.append({
                    **failure, 'path': missing[0],
                    'reason': 'SRC_REMOVED'
                })
                continue

            # Sources are reset before applying, so applying twice in a row
            # produces the same outcome both times.
            self._run_git_command(['checkout', '--', *sources],
                                  target_repo_path)

            try:
                self._run_git_command([
                    'apply', '--ignore-space-change', '--ignore-whitespace',
                    str(patch_file)
                ], target_repo_path)
            except subprocess.CalledProcessError:
                failed_patches.append({**failure, 'path': sources[0]})

        return failed_patches

    def chromium_version(self) -> str:
        """The version in the Chromium `chrome/VERSION` file on disk."""
        version_file = self.chromium / 'chrome' / 'VERSION'
        parts = dict(
            line.split('=', 1)
            for line in version_file.read_bytes().decode('utf-8').splitlines()
            if '=' in line)
        return '{MAJOR}.{MINOR}.{BUILD}.{PATCH}'.format(**parts)

    def package_version(self) -> str:
        """The Chromium tag currently set in brave's `package.json`."""
        package = json.loads(
            (self.brave / 'package.json').read_bytes().decode('utf-8'))
        return package['config']['projects']['chrome']['tag']

    def sync_chromium(self, version: str | None = None) -> None:
        """Emulates the `gclient sync` stage of `npm run init`.

        Discards every working-tree change in the Chromium-side repositories
        and checks `src/` out at `version`, leaving it detached exactly as a
        synced checkout is.

        Args:
            version: The Chromium tag to sync to. Defaults to the tag set in
                brave's `package.json`, which is what a real sync uses.
        """
        for repo_path in self.chromium_repos:
            self._run_git_command(['reset', '--hard', 'HEAD'], repo_path)
        self._run_git_command([
            'checkout', '--force', '--detach', version
            or self.package_version()
        ], self.chromium)

    def _stamp_version(self, relative_paths: list[str], template: str) -> None:
        """Rewrites each file's generated-for-version marker line.

        Args:
            relative_paths: Brave-relative paths of the generated files.
            template: The marker line, with a `{version}` placeholder.
        """
        version = self.chromium_version()
        prefix = template.partition('{version}')[0]
        for relative_path in relative_paths:
            path = self.brave / relative_path
            lines = path.read_bytes().decode('utf-8').splitlines(keepends=True)
            if lines and lines[0].startswith(prefix):
                lines.pop(0)
            lines.insert(0, template.format(version=version) + '\n')
            path.write_text(''.join(lines), encoding='utf-8', newline='')

    def run_chromium_rebase_l10n(self) -> list[str]:
        """Emulates `npm run chromium_rebase_l10n`.

        The real command regenerates brave's `.grd`/`.grdp`/`.xtb` files from
        the strings of the Chromium tree currently synced. Here every tracked
        l10n file is stamped with that version instead, which produces the
        string changes a lift is expected to commit.

        Returns:
            The brave-relative paths of the l10n files regenerated.
        """
        files = self._run_git_command(['ls-files', '*.grd', '*.grdp', '*.xtb'],
                                      self.brave).splitlines()
        self._stamp_version(files, L10N_VERSION_STAMP)
        return files

    def run_gnrt(self, subcommand: str) -> list[str]:
        """Emulates `tools/crates/run_gnrt.py <vendor|gen>`.

        `vendor` refreshes the vendored crates, which leaves brave's tree
        untouched here. `gen` regenerates the `BUILD.gn` files under brave's
        `third_party/rust/`, stamped with the version currently synced.

        Returns:
            The brave-relative paths of the files regenerated.
        """
        if subcommand == 'vendor':
            return []
        if subcommand != 'gen':
            raise ValueError(f'Unsupported gnrt subcommand: {subcommand}')
        files = self._run_git_command(
            ['ls-files', 'third_party/rust/*BUILD.gn'],
            self.brave).splitlines()
        self._stamp_version(files, GNRT_VERSION_STAMP)
        return files

    def cleanup(self) -> None:
        """Cleans up the temporary directory used for the fake repository."""
        if self._original_cwd is not None:
            os.chdir(self._original_cwd)
            self._original_cwd = None
        try:
            self.temp_dir.cleanup()
        except OSError:
            print(f'Failed to clean up temp dir: {self.temp_dir.name}')
            for dirpath, dirnames, filenames in os.walk(self.temp_dir.name):
                for name in dirnames + filenames:
                    full = os.path.join(dirpath, name)
                    try:
                        file_stat = os.stat(full)
                        writable = os.access(full, os.W_OK)
                        print(f'  {oct(file_stat.st_mode)} '
                              f'{"rw" if writable else "ro"} {full}')
                    except OSError as stat_err:
                        print(f'  <stat error: {stat_err}> {full}')
            raise
