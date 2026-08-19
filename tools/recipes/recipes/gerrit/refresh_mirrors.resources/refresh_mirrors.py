#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Publish the local git cache into our Gerrit mirror instance.
"""

from __future__ import annotations

import argparse
import logging
import os
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from urllib.parse import urlsplit

# Our gerrit endpoint
GERRIT_SSH_HOST = 'gerrit-ssh.brave.com'
GERRIT_SSH_PORT = 29418

# The parent project under which all mirrors are created.
MIRROR_PARENT = 'mirror'
MIRROR_OWNER_GROUP = 'mirrors-admins'

# Name of the cache repos' upstream remote
UPSTREAM_REMOTE = 'origin'
GERRIT_REMOTE = 'gerrit'

# Projects whose history is too large for Gerrit to accept in a single push.
LARGE_REPOS = frozenset({
    'mirror/chromium.googlesource.com/chromium/src',
})



# Max refs to push per `git push` when mirroring many refs at once.
MIRROR_REFS_PER_PUSH = 200


# Commits to advance a large repo's default branch per push. A single push of
# all chromium/src history (~600k objects / 1 GiB) OOMs Gerrit's JGit unpacker,
# so seeding advances the branch through ancestor checkpoints this many commits
# apart.
LARGE_REPO_COMMIT_CHUNK = 5000

# Suffix dirs git_cache leaves beside each repo for locking; not repos.
LOCK_DIR_SUFFIX = '.locked'


def _query(*command: str,
           cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    """Run *command*, capturing output and never raising on a non-zero exit.

    Use this for read-only queries where a non-zero exit is an expected,
    handled condition (e.g. asking Gerrit whether a project exists).
    """
    logging.debug('>>> %s', ' '.join(command))
    return subprocess.run(list(command),
                          cwd=cwd,
                          capture_output=True,
                          text=True,
                          check=False)


def _run(*command: str, cwd: Path | None = None) -> None:
    """Run *command*, logging it first, and raise on a non-zero exit.

    Raises:
        subprocess.CalledProcessError: If the command exits non-zero.
    """
    logging.info('>>> %s', ' '.join(command))
    subprocess.run(list(command), cwd=cwd, check=True)


def project_name_for(upstream_url: str) -> str:
    """Return the Gerrit project name mirroring *upstream_url*.

    The project is the URL's host and path (no trailing `.git`) under the
    `mirror/` parent, so the on-Gerrit layout reads like the upstream one and
    stays unique across hosts that share a path:

        https://aomedia.googlesource.com/aom.git
            -> mirror/aomedia.googlesource.com/aom

    Raises:
        ValueError: If *upstream_url* lacks a host or a path component.
    """
    parts = urlsplit(upstream_url)
    host = parts.hostname or ''
    path = parts.path.strip('/')
    if path.endswith('.git'):
        path = path[:-len('.git')]
    path = path.strip('/')
    if not host or not path:
        raise ValueError(f'cannot derive a project name from URL: '
                         f'{upstream_url!r}')
    return f'{MIRROR_PARENT}/{host}/{path}'


@dataclass(frozen=True)
class Gerrit:
    """A handle to the Gerrit instance: runs admin commands and builds URLs.

    The instance is fixed (`gerrit-ssh.brave.com:29418`); only the SSH user --
    the account whose key authorises the create/push -- varies per caller.
    """

    # SSH user the `gerrit` CLI and git pushes authenticate as.
    user: str
    dry_run: bool = False

    def _ssh_prefix(self) -> list[str]:
        return [
            'ssh', '-p',
            str(GERRIT_SSH_PORT), f'{self.user}@{GERRIT_SSH_HOST}'
        ]

    def project_url(self, project: str) -> str:
        """Return the SSH git URL for *project*."""
        return (f'ssh://{self.user}@{GERRIT_SSH_HOST}:{GERRIT_SSH_PORT}/'
                f'{project}')

    def project_exists(self, project: str) -> bool:
        """Return whether *project* already exists on the Gerrit instance.

        Uses `gerrit ls-projects --prefix`, which returns every project whose
        name starts with the prefix; an exact match in that set is the answer.
        """
        result = _query(*self._ssh_prefix(), 'gerrit', 'ls-projects',
                        '--prefix', project)
        if result.returncode != 0:
            # A query failure (auth, connectivity) is not "absent"; surface it
            # rather than silently trying to create a project that may exist.
            raise RuntimeError(f'gerrit ls-projects failed for {project!r}: '
                               f'{result.stderr.strip()}')
        return project in result.stdout.split()

    def create_project(self, project: str) -> None:
        """Create *project* under the mirror parent, owned by the admin group.

        Mirrors inherit their push-only ACLs from `MIRROR_PARENT`; the explicit
        owner keeps the mirrors-admins group in control even if the parent
        changes.
        """
        if self.dry_run:
            logging.info('[dry-run] would create project %s', project)
            return
        _run(*self._ssh_prefix(), 'gerrit', 'create-project', project,
             '--owner', MIRROR_OWNER_GROUP, '--parent', MIRROR_PARENT)


def _is_bare_repo(path: Path) -> bool:
    """Return whether *path* is an existing, valid bare git repository."""
    if not (path / 'config').is_file():
        return False
    return _query('git', '-C', str(path), 'rev-parse',
                  '--is-bare-repository').stdout.strip() == 'true'


class Repo:
    """A git_cache bare repo, and every git operation performed on it.

    Wraps the repo's path on disk and owns all the plumbing needed to mirror it
    into Gerrit: inspecting its upstream URL and branches, managing its
    `gerrit` remote, and pushing refs/tags/history to it.
    """

    def __init__(self, path: Path) -> None:
        self.path = path

    @property
    def name(self) -> str:
        return self.path.name

    def __repr__(self) -> str:
        return f'Repo({self.path!r})'

    @staticmethod
    def is_cache_repo(path: Path) -> bool:
        """Return whether *path* is a git_cache bare repository to mirror.

        Skips the `*.locked` lock directories git_cache leaves beside each
        repo, and anything that is not a bare repo with a config (e.g. stray
        files).
        """
        if not path.is_dir() or path.name.endswith(LOCK_DIR_SUFFIX):
            return False
        return _is_bare_repo(path)

    def upstream_url(self) -> str | None:
        """Return this repo's upstream remote URL, or None if it has none."""
        result = _query('git', '-C', str(self.path), 'config', '--get',
                        f'remote.{UPSTREAM_REMOTE}.url')
        return result.stdout.strip() or None

    def head_ref(self) -> str | None:
        """Return the branch ref this repo's HEAD resolves to, or None.

        git_cache keeps a bare repo's HEAD as a symbolic ref to the upstream
        default branch (e.g. `refs/heads/main`); that single branch is the one
        we mirror. Returns None when HEAD is detached, unborn, or *dangling* --
        a symbolic ref to a branch that has no commit. The last case is common
        on Chromium's external mirrors, whose HEAD reads `refs/heads/master`
        while the real branches live under `refs/heads/upstream/*`; the caller
        then mirrors all heads instead.
        """
        ref = _query('git', '-C', str(self.path), 'symbolic-ref', '--quiet',
                     'HEAD').stdout.strip()
        if not ref:
            return None
        resolves = _query('git', '-C', str(self.path), 'rev-parse', '--verify',
                          '--quiet', f'{ref}^{{commit}}').returncode == 0
        return ref if resolves else None

    def has_local_heads(self) -> bool:
        """Return whether this repo has any local branch under refs/heads/."""
        return bool(
            _query('git', '-C', str(self.path), 'for-each-ref', '--count=1',
                   'refs/heads/').stdout.strip())

    def push_refs_in_batches(self, refs: list[str], label: str) -> None:
        """Force-push *refs* (full refnames) to the `gerrit` remote, in batches.

        Some repos carry thousands of refs, which overwhelm Gerrit's receive
        timeout if pushed at once, so at most MIRROR_REFS_PER_PUSH go up per
        push.

        Raises:
            subprocess.CalledProcessError: If a batch push fails.
        """
        total = len(refs)
        logging.info('Pushing %d %s in batches of %d', total, label,
                     MIRROR_REFS_PER_PUSH)
        for start in range(0, total, MIRROR_REFS_PER_PUSH):
            batch = refs[start:start + MIRROR_REFS_PER_PUSH]
            logging.info('  %s %d-%d/%d', label, start + 1, start + len(batch),
                         total)
            refspecs = [f'+{ref}:{ref}' for ref in batch]
            _run('git', '-C', str(self.path), 'push', '-o', 'skip-validation',
                 GERRIT_REMOTE, *refspecs)

    def mirror_all_heads(self) -> None:
        """Force-push every local branch to the `gerrit` remote, in batches.

        Used for repos whose HEAD doesn't resolve to a single default branch
        (e.g. Chromium external mirrors), whose branches must all be mirrored.
        """
        refs = _query('git', '-C', str(self.path), 'for-each-ref',
                      '--format=%(refname)', 'refs/heads/').stdout.split()
        self.push_refs_in_batches(refs, 'branch(es)')

    def local_tags(self) -> dict[str, str]:
        """Return this repo's local tags as {refname: object sha}."""
        out = _query('git', '-C', str(self.path), 'for-each-ref',
                     '--format=%(refname) %(objectname)', 'refs/tags/').stdout
        tags = {}
        for line in out.splitlines():
            name, _, sha = line.partition(' ')
            if name and sha:
                tags[name] = sha
        return tags

    def remote_tags(self) -> dict[str, str]:
        """Return the `gerrit` remote's tags as {refname: object sha}."""
        out = _query('git', '-C', str(self.path), 'ls-remote', '--tags',
                     GERRIT_REMOTE).stdout
        tags = {}
        for line in out.splitlines():
            sha, tab, ref = line.partition('\t')
            # Keep the tag object line, skipping the `^{}` peeled entry
            # ls-remote emits alongside annotated tags.
            if tab and ref and not ref.endswith('^{}'):
                tags[ref] = sha
        return tags

    def push_new_tags(self) -> None:
        """Push tags the `gerrit` remote is missing or that point somewhere else.

        Compares the local tags against the tags Gerrit advertises (both by
        the object each points at) and pushes only the new or changed ones, so
        tags already present at the same object are never re-sent.
        """
        local = self.local_tags()
        if not local:
            return
        remote = self.remote_tags()
        to_push = sorted(name for name, sha in local.items()
                         if remote.get(name) != sha)
        if not to_push:
            logging.info('Tags already up to date (%d tag(s)).', len(local))
            return
        self.push_refs_in_batches(to_push, 'tag(s)')

    def ensure_gerrit_remote(self, url: str) -> None:
        """Point this repo's `gerrit` remote at *url*, adding or updating it."""
        current = _query('git', '-C', str(self.path), 'remote', 'get-url',
                         GERRIT_REMOTE)
        if current.returncode != 0:
            _run('git', '-C', str(self.path), 'remote', 'add', GERRIT_REMOTE,
                 url)
        elif current.stdout.strip() != url:
            _run('git', '-C', str(self.path), 'remote', 'set-url',
                 GERRIT_REMOTE, url)

    def is_shallow(self) -> bool:
        """Return whether this repo has a truncated (shallow) history."""
        return _query('git', '-C', str(self.path), 'rev-parse',
                      '--is-shallow-repository').stdout.strip() == 'true'

    def ensure_full_history(self) -> None:
        """Deepen a shallow repo to its full history before it is ever pushed.

        A shallow clone's boundary commits are stored with their real parent
        SHAs but those parents were never fetched, so pushing history that
        crosses the boundary requires the shallow-push protocol extension to
        graft it in on the other end. Gerrit's JGit-based receive-pack doesn't
        support that, and rejects the whole pack with a "missing commit
        <parent>" unpack error instead -- so a shallow cache repo is deepened
        here, unconditionally, rather than ever letting that push be attempted.

        A cache repo can end up shallow if something else (e.g. a recipe using
        `chromium_checkout.ensure_checkout(depth=...)`) populated this same
        `GIT_CACHE_PATH` with a depth-limited fetch before a full mirror pass
        ever ran; a plain fetch afterwards only extends history forward from
        that boundary; it never backfills it.

        Raises:
            subprocess.CalledProcessError: If the unshallow fetch fails.
        """
        logging.info(
            '%s is a shallow clone; fetching full history before mirroring...',
            self.name)
        _run('git', '-C', str(self.path), 'fetch', '--unshallow',
             UPSTREAM_REMOTE)

    def remote_branch_sha(self, head_ref: str) -> str | None:
        """Return the `gerrit` remote's current tip for *head_ref*, or None."""
        result = _query('git', '-C', str(self.path), 'ls-remote', '--heads',
                        GERRIT_REMOTE, head_ref)
        line = result.stdout.strip()
        return line.split()[0] if line else None

    def commit_checkpoints(self,
                           head_ref: str,
                           chunk_size: int,
                           start: str | None = None) -> list[str]:
        """Return ancestor checkpoints of *head_ref* to push, oldest first.

        Lists first-parent history from *start* (exclusive; the root when
        None) to the tip and picks one commit every *chunk_size* commits,
        always ending at the tip. Pushing these in order advances the branch
        in fast-forwarding steps of *chunk_size* commits each, so a huge
        history is seeded without a single oversized push that would OOM
        Gerrit. One `rev-list` plus a slice keeps this cheap even on
        million-commit histories. Empty means nothing to push.
        """
        rev_range = head_ref if start is None else f'{start}..{head_ref}'
        commits = _query('git', '-C', str(self.path), 'rev-list', '--reverse',
                         '--first-parent', rev_range).stdout.split()
        if not commits:
            return []

        checkpoints = commits[chunk_size - 1::chunk_size]
        # Always finish exactly at the tip.
        if not checkpoints or checkpoints[-1] != commits[-1]:
            checkpoints.append(commits[-1])
        return checkpoints

    def seed_branch(self, branch_ref: str) -> None:
        """Advance the `gerrit` remote's *branch_ref* to local, in commit chunks.

        Resumes from the remote's current tip (the whole branch when the
        remote lacks it), then force-pushes ancestor checkpoints in order so a
        huge history lands without a single oversized push.
        """
        start = self.remote_branch_sha(branch_ref)
        logging.info('Computing seed checkpoints for %s...', branch_ref)
        checkpoints = self.commit_checkpoints(branch_ref,
                                              LARGE_REPO_COMMIT_CHUNK, start)
        if not checkpoints:
            logging.info('%s already up to date.', branch_ref)
            return
        logging.info('Seeding %s in %d push(es) of up to %d commits each',
                     branch_ref, len(checkpoints), LARGE_REPO_COMMIT_CHUNK)
        for index, sha in enumerate(checkpoints, start=1):
            logging.info('  push %d/%d -> %s', index, len(checkpoints),
                         sha[:12])
            _run('git', '-C', str(self.path), 'push', '-o', 'skip-validation',
                 GERRIT_REMOTE, f'+{sha}:{branch_ref}')

    def refresh(self, gerrit: Gerrit) -> bool:
        """Mirror this cache repo into Gerrit.

        Derives the project from the upstream URL, creates it if absent, then
        force-pushes the repo's default branch (the ref HEAD points at) to it
        via a `gerrit` remote on the cache repo. Large repos (LARGE_REPOS) are
        *always* advanced through commit-chunked checkpoints -- resuming from
        whatever the remote already has -- since one big push would exceed
        Gerrit's receive timeout (or OOM its unpacker); they never issue a
        single full-ref push. A shallow repo is deepened to full history first
        (see `ensure_full_history`), since Gerrit can't accept a push across a
        shallow boundary.

        Returns:
            True if the mirror was handled (pushed, or already in sync);
            False if the repo was skipped (no upstream URL, or no branch to
            mirror).

        Raises:
            subprocess.CalledProcessError: On a git failure (e.g. the push).
            RuntimeError: On a Gerrit query failure or unusable URL.
        """
        upstream = self.upstream_url()
        if upstream is None:
            logging.warning('%s has no %s remote URL; skipping.', self.name,
                            UPSTREAM_REMOTE)
            return False

        # The default branch HEAD points at, when it resolves. When it
        # doesn't (detached/unborn/dangling HEAD, as on Chromium external
        # mirrors), fall back to mirroring every branch so the repo's commits
        # still reach Gerrit.
        head_ref = self.head_ref()
        if head_ref is None and not self.has_local_heads():
            logging.warning('%s has no branch to mirror; skipping.', self.name)
            return False

        project = project_name_for(upstream)
        target = head_ref or 'refs/heads/* (HEAD unresolved)'
        logging.info('Mirroring %s (%s) -> %s', self.name, target, project)

        exists = gerrit.project_exists(project)
        if not exists:
            logging.info('Project %s not found; creating it.', project)
            gerrit.create_project(project)
        else:
            logging.info('Project %s already exists.', project)

        if gerrit.dry_run:
            seeded = ' (seeded in chunks)' if project in LARGE_REPOS \
                and head_ref else ''
            logging.info('[dry-run] would push %s to %s%s', target,
                         gerrit.project_url(project), seeded)
            return True

        self.ensure_gerrit_remote(gerrit.project_url(project))

        if self.is_shallow():
            self.ensure_full_history()

        # A large repo must ALWAYS advance in chunks: a single push of its
        # full history (or a big catch-up) exceeds Gerrit's receive timeout /
        # OOMs its unpacker. Resume from the remote's current tip so a partial
        # seed continues where it stopped, and never fall through to the
        # full-ref push below.
        if project in LARGE_REPOS:
            if head_ref is None:
                logging.warning(
                    '%s is large but HEAD does not resolve; skipping to '
                    'avoid a bulk push.', project)
                return False
            self.seed_branch(head_ref)
            # Publish tags too (release tags point off the default branch);
            # batched and incremental so only newly cut tags ship each run.
            self.push_new_tags()
            return True

        # Push the default branch when HEAD resolves; otherwise mirror every
        # branch (batched) so a dangling-HEAD repo isn't lost. Forced, so the
        # mirror tracks upstream even across a history rewrite.
        if head_ref is not None:
            _run('git', '-C', str(self.path), 'push', '-o', 'skip-validation',
                 GERRIT_REMOTE, f'+{head_ref}:{head_ref}')
        else:
            self.mirror_all_heads()
        return True


def discover_cache_repos(git_cache_path: Path) -> list[Repo]:
    """Return the bare cache repos directly under *git_cache_path*, sorted."""
    return sorted(
        (Repo(p) for p in git_cache_path.iterdir() if Repo.is_cache_repo(p)),
        key=lambda repo: repo.path)


@dataclass
class MirrorSummary:
    """Tally of a mirror run's outcome: which repos were updated, skipped, or
    failed."""

    updated: list[str] = field(default_factory=list)
    skipped: list[str] = field(default_factory=list)
    failed: list[str] = field(default_factory=list)

    def run(self, git_cache_path: Path, gerrit: Gerrit) -> None:
        """Mirror every cache repo under *git_cache_path* into Gerrit.

        A failure on one repo is logged and the run continues with the rest, so
        one bad mirror never blocks the others. A summary of the outcome --
        which repos were updated, skipped, and failed -- is printed at the end.
        """
        repos = discover_cache_repos(git_cache_path)
        logging.info('Found %d cache repo(s) under %s', len(repos),
                     git_cache_path)
        for repo in repos:
            try:
                if repo.refresh(gerrit):
                    self.record_updated(repo.name)
                else:
                    self.record_skipped(repo.name)
            except (subprocess.CalledProcessError, RuntimeError,
                    ValueError) as e:
                self.record_failed(repo.name)
                logging.error('Failed to mirror %s: %s', repo.name, e)

        self.log()

    def record_updated(self, name: str) -> None:
        self.updated.append(name)

    def record_skipped(self, name: str) -> None:
        self.skipped.append(name)

    def record_failed(self, name: str) -> None:
        self.failed.append(name)

    @property
    def failure_count(self) -> int:
        return len(self.failed)

    def log(self) -> None:
        """Print a final tally of the run, listing updated and failed repos."""
        logging.info('')
        logging.info('==== Mirror refresh summary ====')
        logging.info('updated: %d  skipped: %d  failed: %d', len(self.updated),
                     len(self.skipped), len(self.failed))
        if self.updated:
            logging.info('Updated:')
            for name in self.updated:
                logging.info('  + %s', name)
        if self.skipped:
            # Skipped for a repo-specific reason logged inline when it
            # happened (no upstream URL, or no branch to mirror).
            logging.info('Skipped:')
            for name in self.skipped:
                logging.info('  . %s', name)
        if self.failed:
            logging.error('Failed:')
            for name in self.failed:
                logging.error('  ! %s', name)


def main() -> int:
    """Parse arguments and refresh every mirror under `GIT_CACHE_PATH`."""
    parser = argparse.ArgumentParser(
        description='Publish the local git cache into the Gerrit mirrors.')
    parser.add_argument(
        '--git-cache-path',
        default=os.environ.get('GIT_CACHE_PATH'),
        help='Root of the git cache (defaults to $GIT_CACHE_PATH).')
    parser.add_argument('--user', required=True, help='Gerrit username.')
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Log the project creates and pushes without performing them.')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable debug logging.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        format='%(message)s')

    if not args.git_cache_path:
        parser.error('GIT_CACHE_PATH is not set; pass --git-cache-path.')
    git_cache_path = Path(args.git_cache_path).expanduser()
    if not git_cache_path.is_dir():
        parser.error(f'git cache path is not a directory: {git_cache_path}')

    gerrit = Gerrit(user=args.user, dry_run=args.dry_run)

    summary = MirrorSummary()
    summary.run(git_cache_path, gerrit)
    if summary.failure_count:
        logging.error('%d mirror(s) failed.', summary.failure_count)
    return 1 if summary.failure_count else 0


if __name__ == '__main__':
    raise SystemExit(main())
