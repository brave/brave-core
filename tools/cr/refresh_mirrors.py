#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Refresh git mirrors from upstream sources into our Gerrit instance.

This script fetches each configured upstream repository and pushes it to its
corresponding Gerrit mirror. It is designed to run standalone in CI without
any third-party dependencies.

Usage:
    python3 refresh_mirrors.py --mirrors-path=./mirrors [--verbose]

Or directly from GitHub:
    curl -sSLf \\
        "https://raw.githubusercontent.com/brave/brave-core/master/tools/cr/refresh_mirrors.py" \\
        | python3 - --mirrors-path=./mirrors
"""

import argparse
import logging
import subprocess
from dataclasses import dataclass, replace
from pathlib import Path
from typing import List, Optional

BRAVE_GERRIT_URL = 'ssh://{username}gerrit.brave.com:29418'


def _query(*command: str,
           cwd: Optional[Path] = None) -> subprocess.CompletedProcess:
    """Run command and return its result without raising on non-zero exit.

    Use this for read-only queries where a non-zero exit is an expected,
    handled condition (e.g. checking whether a git remote exists).

    Args:
        *command: Program and its arguments.
        cwd: Optional working directory for the subprocess.
    """
    logging.info('>>> %s', ' '.join(command))
    return subprocess.run(list(command),
                          cwd=cwd,
                          capture_output=True,
                          text=True,
                          check=False)


def _run(*command: str, cwd: Optional[Path] = None) -> None:
    """Run command, printing it first; log stderr and raise on failure.

    Args:
        *command: Program and its arguments.
        cwd: Optional working directory for the subprocess.

    Raises:
        subprocess.CalledProcessError: If the command exits non-zero.
    """
    logging.info('>>> %s', ' '.join(command))
    subprocess.run(list(command), cwd=cwd, check=True)


@dataclass
class Mirror:
    """A git mirror pairing an upstream source with a Gerrit destination.

    Encapsulates all operations needed to keep a Gerrit mirror in sync:
    initialising the local bare repository, configuring remotes, fetching
    from upstream, and pushing to Gerrit.

    The static fields (mirror, upstream, refs) describe what to mirror.
    The runtime fields (mirrors_path, gerrit_url) are set once via
    dataclasses.replace() in main() before any operation is called.
    """
    # URI path component of the Gerrit repository, e.g. "googlesource/chromium".
    mirror: str
    # Upstream git URL to fetch from.
    upstream: str
    # Refspecs to sync, e.g. ["refs/heads/*:refs/heads/*"].
    refs: List[str]
    # Root directory under which mirror repos are stored; set at runtime.
    mirrors_path: Optional[Path] = None
    # Gerrit base URL, e.g. "ssh://gerrit.brave.com:29418"; set at runtime.
    gerrit_url: Optional[str] = None

    def local_path(self) -> Path:
        """Return the local bare repo path for this mirror."""
        return self.mirrors_path / self.mirror

    def _ensure_remote(self, path: Path, name: str, url: str) -> None:
        """Ensure a git remote exists and points to the expected URL.

        Adds the remote if absent; updates it if the URL has drifted.

        Args:
            path: Local bare repo directory.
            name: Remote name, either 'upstream' or 'mirror'.
            url: Expected remote URL.
        """
        result = _query('git', 'remote', 'get-url', name, cwd=path)
        if result.returncode != 0:
            _run('git', 'remote', 'add', name, url, cwd=path)
        elif result.stdout.strip() != url:
            logging.info('Updating remote %s from %s to %s', name,
                         result.stdout.strip(), url)
            _run('git', 'remote', 'set-url', name, url, cwd=path)

    def _setup(self) -> None:
        """Initialise the local bare repo and configure remotes.

        Creates the bare repo under mirrors_path if it does not already exist.
        For existing repos the remotes are verified and updated if they have
        drifted from the values stored in this Mirror.
        """
        path = self.local_path()
        mirror_url = f'{self.gerrit_url}/{self.mirror}'
        if not path.exists():
            path.mkdir(parents=True)
            _run('git', 'init', '--bare', str(path))
            _run('git', 'remote', 'add', 'upstream', self.upstream, cwd=path)
            _run('git', 'remote', 'add', 'mirror', mirror_url, cwd=path)
        else:
            self._ensure_remote(path, 'upstream', self.upstream)
            self._ensure_remote(path, 'mirror', mirror_url)

    def fetch(self) -> None:
        """Fetch configured refs from upstream into the local bare repo.

        Raises:
            subprocess.CalledProcessError: If the fetch fails.
        """
        _run('git',
             'fetch',
             'upstream',
             '--no-tags',
             '--prune',
             '--prune-tags',
             *self.refs,
             cwd=self.local_path())

    def push(self) -> None:
        """Push configured refs from the local bare repo to the Gerrit mirror.

        Raises:
            subprocess.CalledProcessError: If the push fails.
        """
        _run('git',
             'push',
             'mirror',
             '--prune',
             *self.refs,
             cwd=self.local_path())

    def sync(self) -> None:
        """Set up, fetch, and push this mirror.

        Raises:
            subprocess.CalledProcessError: On any git failure; propagates to
                the caller so it can decide whether to continue with other
                mirrors.
        """
        self._setup()
        self.fetch()
        self.push()


MIRRORS: List[Mirror] = [
    Mirror(
        mirror='googlesource/chromium',
        upstream='https://chromium.googlesource.com/chromium/src',
        refs=[
            'refs/heads/*:refs/heads/*',
            'refs/tags/*:refs/tags/*',
        ],
    ),
]


def main() -> None:
    """Parse arguments and sync all configured mirrors."""
    parser = argparse.ArgumentParser(
        description='Refresh git mirrors from upstream sources into Gerrit.')
    parser.add_argument('--mirrors-path',
                        required=True,
                        help='Root directory under which mirrors are stored.')
    parser.add_argument('--user',
                        default='',
                        help='Gerrit SSH username, e.g. "cdesouza". When '
                        'omitted the push URL has no user prefix.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable debug logging.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    username = f'{args.user}@' if args.user else ''
    gerrit_url = BRAVE_GERRIT_URL.format(username=username)

    mirrors_path = Path(args.mirrors_path).expanduser()
    mirrors_path.mkdir(parents=True, exist_ok=True)

    for mirror in MIRRORS:
        try:
            replace(mirror, mirrors_path=mirrors_path,
                    gerrit_url=gerrit_url).sync()
        except subprocess.CalledProcessError:
            logging.error('Mirror %s failed; skipping.', mirror.mirror)


if __name__ == '__main__':
    main()
