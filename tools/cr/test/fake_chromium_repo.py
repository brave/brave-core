# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import tempfile
import subprocess
from pathlib import Path
from typing import List
import json
import shutil

CHROME_VERSION_TEMPLATE: str = """MAJOR={major}
MINOR={minor}
BUILD={build}
PATCH={patch}
"""

class FakeChromiumRepo:
    """A fake Chromium repository for testing purposes."""

    def __init__(self) -> None:
        """Initializes the fake Chromium repository.

        Creates a temporary directory and initializes a fake Chromium repository
        with a `src` directory. Also creates a `brave` repository inside `src`.
        """
        self.temp_dir: tempfile.TemporaryDirectory = (
            tempfile.TemporaryDirectory())
        self.base_path: Path = Path(self.temp_dir.name) / 'workspace'
        self._init_repo(self.chromium)
        self._init_repo(self.brave)  # Create the brave repository

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

    def _run_git_command(self,
                         command: List[str],
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
        self._run_git_command(['init'], path)
        (path / 'README.md').write_text(f'# Fake {path.name} repo\n')
        self._run_git_command(['add', 'README.md'], path)
        self._run_git_command(['commit', '-m', 'Initial commit'], path)

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
        self._run_git_command(['tag', version], self.chromium)

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
        file_path.write_text(content)
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
        """Similar to `npm run update_patches`.

        This method generates patches for all modified files in Chromium and
        its dependencies. For simplicity, running this function always results
        in all of brave's patches being erased and generated again. This
        eliminates any stales.
        """
        # Delete the patches directory and recreate it
        if self.brave_patches.exists():
            shutil.rmtree(self.brave_patches)
        self.brave_patches.mkdir(parents=True, exist_ok=True)

        for repo_path in self.base_path.glob('src/**'):
            if repo_path == self.brave or not (repo_path / '.git').exists():
                continue

            # Find all files dirty in the tree.
            modified_files = self._run_git_command(['diff', '--name-only'],
                                                   repo_path).splitlines()

            # Determine the relative path of the repo to Chromium
            relative_repo_path = repo_path.relative_to(self.chromium)
            if modified_files:
                sub_patches_dir = self.brave_patches / relative_repo_path
                sub_patches_dir.mkdir(parents=True, exist_ok=True)

            for filename in modified_files:
                # Generate the patch file path
                patch_file = self.brave / self.get_patchfile_path_for_source(
                    relative_repo_path, filename)

                # Generate and write the patch content
                patch_content = self._run_git_command([
                    'diff', '--src-prefix=a/', '--dst-prefix=b/',
                    '--full-index', filename
                ],
                                                      repo_path,
                                                      strip=False)
                patch_file.write_text(patch_content)

    def get_patchfile_path_for_source(self, repo_path: Path,
                                      filename: str) -> Path:
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
                f'{str(filename).replace("/", "-")}.patch').relative_to(
                    self.brave)

    def run_apply_patches(self) -> None:
        """Similar to `npm run apply_patches`"""
        if not self.brave_patches.exists():
            raise FileNotFoundError(
                f'Patches directory {self.brave_patches} does not exist.')

        for patch_file in self.brave_patches.rglob('*.patch'):
            # Using the relative path of the patch file to determine the target
            # repository path.
            relative_repo_path = patch_file.relative_to(
                self.brave_patches).parent
            target_repo_path = self.chromium / relative_repo_path

            if not (target_repo_path / '.git').exists():
                raise FileNotFoundError(
                    f'Target repository {target_repo_path} does not exist.')

            # Apply the patch
            self._run_git_command(['apply', str(patch_file)], target_repo_path)

            # Reset the file to ensure applied changes are not staged
            self._run_git_command(['reset'], target_repo_path)

    def cleanup(self) -> None:
        """Cleans up the temporary directory used for the fake repository."""
        self.temp_dir.cleanup()
