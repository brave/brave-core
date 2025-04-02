# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import tempfile
import subprocess
from pathlib import Path
from typing import List

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
        self.base_path: Path = Path(self.temp_dir.name)
        self._init_repo(self.chromium)
        self._init_repo(self.brave)  # Create the brave repository

    @property
    def chromium(self) -> Path:
        """Returns the path to the Chromium source directory."""
        return self.base_path / "src"

    @property
    def brave(self) -> Path:
        """Returns the path to the Brave directory"""
        return self.chromium / "brave"

    def _run_git_command(self, command: List[str], cwd: Path) -> str:
        """Runs a git command in the specified directory and returns the stdout.

        Args:
            command: The git command to execute as a list of strings.
            cwd: The directory in which to run the command.

        Returns:
            The stdout output of the git command as a string.
        """
        return subprocess.check_output(command,
                                       cwd=cwd,
                                       stderr=subprocess.DEVNULL,
                                       text=True).strip()

    def _init_repo(self, path: Path) -> None:
        """Initializes a git repository at the specified path.

        Creates a `README.md` file, stages it, and makes an initial commit.

        Args:
            path: The path where the repository should be initialized.
        """
        path.mkdir(parents=True, exist_ok=True)
        self._run_git_command(["git", "init"], path)
        (path / "README.md").write_text(f"# Fake {path.name} repo\n")
        self._run_git_command(["git", "add", "README.md"], path)
        self._run_git_command(["git", "commit", "-m", "Initial commit"], path)

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
            ["git", "submodule", "add",
             str(dep_path), relative_path], self.chromium)
        self._run_git_command(
            ["git", "commit", "-m", f"Add submodule {relative_path}"],
            self.chromium)

    def add_tag(self, version: str) -> None:
        """Adds a git tag to the repository.

        Creates a `VERSION` file with the specified version and commits it,
        then tags the commit with the version.

        Args:
            version: The version string in the format `MAJOR.MINOR.BUILD.PATCH`.
        """
        major, minor, build, patch = version.split(".")
        version_file: Path = self.chromium / "chrome" / "VERSION"
        version_file.parent.mkdir(parents=True, exist_ok=True)
        version_file.write_text(
            CHROME_VERSION_TEMPLATE.format(major=major,
                                           minor=minor,
                                           build=build,
                                           patch=patch))
        self._run_git_command(["git", "add", str(version_file)], self.chromium)
        self._run_git_command(["git", "commit", "-m", f"VERSION {version}"],
                              self.chromium)
        self._run_git_command(["git", "tag", version], self.chromium)

    def commit_empty(self, commit_message: str, repo_path: Path) -> str:
        """Creates an empty commit for a repository and returns a hash.

        Args:
            commit_message: The message to use for the empty commit.
            repo_path: The repository path.

        Returns:
            The hash of the commit made.
        """
        self._run_git_command(
            ["git", "commit", "--allow-empty", "-m", commit_message],
            repo_path)
        return self._run_git_command(["git", "rev-parse", "HEAD"], repo_path)

    def commit(self, commit_message: str, repo_path: Path) -> str:
        """Creates commit in the passed repository and returns a hash.

        Args:
            commit_message: The message to use for the commit.
            repo_path: The repository path.

        Returns:
            The hash of the commit made.
        """
        self._run_git_command(["git", "commit", "-m", commit_message],
                              repo_path)
        return self._run_git_command(["git", "rev-parse", "HEAD"], repo_path)

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
        self._run_git_command(["git", "add", str(file_path)], repo_path)

    def cleanup(self) -> None:
        """Cleans up the temporary directory used for the fake repository."""
        self.temp_dir.cleanup()
