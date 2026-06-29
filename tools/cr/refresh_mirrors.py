#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Publish the local git cache into our Gerrit mirror instance.

Walks every bare repository under `GIT_CACHE_PATH` (the shared cache populated
by depot_tools' `git_cache`), and for each one mirrors it into Gerrit:

  * The Gerrit project name is derived from the repo's `remote.origin.url`:
    its host and path (minus a trailing `.git`) under the `mirror/` parent. So
    `https://aomedia.googlesource.com/aom.git` becomes the project
    `mirror/aomedia.googlesource.com/aom`. Keeping the host means two upstreams
    that share a path still map to distinct mirrors.
  * If that project does not exist yet it is created under the `mirror` parent,
    owned by the `mirrors-admins` group (see tools/cr `setup.sh`).
  * The repo's default branch -- the single ref its HEAD points at, e.g.
    `refs/heads/main` -- is then force-pushed to the project. Gerrit is added as
    a `gerrit` remote on the cache repo so it doubles as the push trampoline --
    no separate working copy is needed. The push is forced so a mirror stays in
    exact sync with upstream even across a history rewrite.

Usage:
    GIT_CACHE_PATH=~/dev/git-cache \\
        python3 refresh_mirrors.py --user=chromium-mirror-bot [--verbose]
"""

from __future__ import annotations

import argparse
import logging
import os
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import urlsplit

# Gerrit SSH endpoint, mirroring tools/cr `setup.sh`. The HTTP host is
# `gerrit.brave.com`; SSH (used for both the `gerrit` admin CLI and git pushes)
# is a distinct host on the Gerrit listen port.
GERRIT_SSH_HOST = 'gerrit-ssh.brave.com'
GERRIT_SSH_PORT = 29418

# Parent project every mirror inherits its (push-only, admin-owned) permissions
# from, and the group that owns each created mirror. Both are established by
# `setup.sh`; new projects must match so they inherit the right ACLs.
MIRROR_PARENT = 'mirror'
MIRROR_OWNER_GROUP = 'mirrors-admins'

# Name of the cache repos' upstream remote (depot_tools' git_cache uses
# `origin`) and the remote this script points back at Gerrit.
UPSTREAM_REMOTE = 'origin'
GERRIT_REMOTE = 'gerrit'

# Gerrit push option that bypasses the server-side commit validators. Those run
# per pushed commit and dominate the push time on the large histories we mirror,
# while adding nothing for an exact upstream copy. Gerrit gates it on the Forge
# Author/Committer/Server and Push Merge rights, which `setup.sh` already grants
# the mirrors-admins group.
SKIP_VALIDATION_OPTION = 'skip-validation'

# Projects whose history is too large for Gerrit to accept in a single push.
# Their default branch is advanced incrementally (see LARGE_REPO_OBJECT_BUDGET)
# instead. Keyed by derived project name (see project_name_for).
LARGE_REPOS = frozenset({
    'mirror/chromium.googlesource.com/chromium/src',
})

# Max refs to push per `git push` when mirroring many refs at once -- all
# branches of a dangling-HEAD repo, or a large repo's tags. Some repos carry
# thousands of refs (LiteRT's `refs/heads/chromium/*`, chromium/src's release
# tags); updating them all in one push exceeds Gerrit's receive timeout even
# when the objects are few. Batching bounds the ref-update work per push.
MIRROR_REFS_PER_PUSH = 200

# Commits to advance a large repo's default branch per push. A single push of
# all chromium/src history (~600k objects / 1 GiB) OOMs Gerrit's JGit unpacker,
# so seeding advances the branch through ancestor checkpoints this many commits
# apart. Object density is roughly constant on these histories (~15 objects per
# commit on chromium/src), so a commit count is a good proxy for the per-push
# cost. The binding limit is Gerrit's `receive.timeout` (4m): the *whole* server
# receive -- transfer plus delta resolution -- must finish inside it, and ~10000
# commits (~120k objects, heavy delta chains) can exceed it. 5000 keeps each
# push well under the deadline. Lower it further if pushes still time out or
# OOM; raise it (fewer, larger pushes) only if the server is fast with headroom.
LARGE_REPO_COMMIT_CHUNK = 5000


@dataclass(frozen=True)
class ManagedMirror:
    """A repo the script fetches itself and mirrors, not from the git cache.

    The default branch is always seeded in commit chunks (like chromium/src);
    channel-release branches (`N.N.x`) and tags are mirrored only when opted in.
    The target project must already exist -- it is never created here.
    """
    # Short name; also the local bare-repo directory under MANAGED_SUBDIR.
    name: str
    # Upstream clone URL (SSH or HTTPS).
    url: str
    # Existing Gerrit project the refs are published to.
    project: str
    # The default branch to seed in commit chunks (e.g. `refs/heads/main`).
    branch: str
    # Also mirror the `N.N.x` channel-release branches.
    channels: bool = False
    # Also mirror all tags.
    tags: bool = False


# Repos not present in the git cache that the script clones/fetches on its own.
# brave-core gets the full large treatment (master + channels + tags); the
# depot_tools fork mirrors only its main branch.
MANAGED_MIRRORS = [
    ManagedMirror('brave-core',
                  'git@github.com:brave/brave-core.git',
                  'brave-browser/brave-core',
                  'refs/heads/master',
                  channels=True,
                  tags=True),
    ManagedMirror(
        'depot_tools',
        'https://chromium.googlesource.com/chromium/tools/depot_tools',
        'brave-browser/forks/depot_tools', 'refs/heads/main'),
]

# Subdirectory of the git cache where managed mirrors keep their bare repos.
# Nested (not a direct child), so discover_cache_repos never picks them up.
MANAGED_SUBDIR = 'managed'

# Channel-release branches to mirror for managed repos: `NUMBER.NUMBER.x`
# (e.g. `1.92.x`), the branches brave-core cuts for channel releases.
CHANNEL_BRANCH_RE = re.compile(r'^refs/heads/\d+\.\d+\.x$')

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


def is_cache_repo(path: Path) -> bool:
    """Return whether *path* is a git_cache bare repository to mirror.

    Skips the `*.locked` lock directories git_cache leaves beside each repo,
    and anything that is not a bare repo with a config (e.g. stray files).
    """
    if not path.is_dir() or path.name.endswith(LOCK_DIR_SUFFIX):
        return False
    return _is_bare_repo(path)


def discover_cache_repos(git_cache_path: Path) -> list[Path]:
    """Return the bare cache repos directly under *git_cache_path*, sorted."""
    return sorted(p for p in git_cache_path.iterdir() if is_cache_repo(p))


def _upstream_url(repo: Path) -> str | None:
    """Return *repo*'s upstream remote URL, or None if it has none."""
    result = _query('git', '-C', str(repo), 'config', '--get',
                    f'remote.{UPSTREAM_REMOTE}.url')
    return result.stdout.strip() or None


def _local_ref_exists(repo: Path, ref: str) -> bool:
    """Return whether *ref* resolves to a commit in *repo*."""
    return _query('git', '-C', str(repo), 'rev-parse', '--verify', '--quiet',
                  f'{ref}^{{commit}}').returncode == 0


def _head_ref(repo: Path) -> str | None:
    """Return the branch ref *repo*'s HEAD resolves to, or None.

    git_cache keeps a bare repo's HEAD as a symbolic ref to the upstream default
    branch (e.g. `refs/heads/main`); that single branch is the one we mirror.
    Returns None when HEAD is detached, unborn, or *dangling* -- a symbolic ref
    to a branch that has no commit. The last case is common on Chromium's
    external mirrors, whose HEAD reads `refs/heads/master` while the real
    branches live under `refs/heads/upstream/*`; the caller then mirrors all
    heads instead.
    """
    ref = _query('git', '-C', str(repo), 'symbolic-ref', '--quiet',
                 'HEAD').stdout.strip()
    if not ref:
        return None
    resolves = _query('git', '-C', str(repo), 'rev-parse', '--verify',
                      '--quiet', f'{ref}^{{commit}}').returncode == 0
    return ref if resolves else None


def _has_local_heads(repo: Path) -> bool:
    """Return whether *repo* has any local branch under refs/heads/."""
    return bool(
        _query('git', '-C', str(repo), 'for-each-ref', '--count=1',
               'refs/heads/').stdout.strip())


def _push_refs_in_batches(repo: Path, refs: list[str], label: str) -> None:
    """Force-push *refs* (full refnames) to the `gerrit` remote, in batches.

    Some repos carry thousands of refs, which overwhelm Gerrit's receive timeout
    if pushed at once, so at most MIRROR_REFS_PER_PUSH go up per push.

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
        _run('git', '-C', str(repo), 'push', '-o', SKIP_VALIDATION_OPTION,
             GERRIT_REMOTE, *refspecs)


def _mirror_all_heads(repo: Path) -> None:
    """Force-push every local branch to the `gerrit` remote, in batches.

    Used for repos whose HEAD doesn't resolve to a single default branch (e.g.
    Chromium external mirrors), whose branches must all be mirrored.
    """
    refs = _query('git', '-C', str(repo), 'for-each-ref',
                  '--format=%(refname)', 'refs/heads/').stdout.split()
    _push_refs_in_batches(repo, refs, 'branch(es)')


def _local_tags(repo: Path) -> dict[str, str]:
    """Return *repo*'s local tags as {refname: object sha}."""
    out = _query('git', '-C', str(repo), 'for-each-ref',
                 '--format=%(refname) %(objectname)', 'refs/tags/').stdout
    tags = {}
    for line in out.splitlines():
        name, _, sha = line.partition(' ')
        if name and sha:
            tags[name] = sha
    return tags


def _remote_tags(repo: Path) -> dict[str, str]:
    """Return the `gerrit` remote's tags as {refname: object sha}."""
    out = _query('git', '-C', str(repo), 'ls-remote', '--tags',
                 GERRIT_REMOTE).stdout
    tags = {}
    for line in out.splitlines():
        sha, tab, ref = line.partition('\t')
        # Keep the tag object line, skipping the `^{}` peeled entry ls-remote
        # emits alongside annotated tags.
        if tab and ref and not ref.endswith('^{}'):
            tags[ref] = sha
    return tags


def _push_new_tags(repo: Path) -> None:
    """Push tags the `gerrit` remote is missing or that point somewhere else.

    Compares the local tags against the tags Gerrit advertises (both by the
    object each points at) and pushes only the new or changed ones, so tags
    already present at the same object are never re-sent.
    """
    local = _local_tags(repo)
    if not local:
        return
    remote = _remote_tags(repo)
    to_push = sorted(name for name, sha in local.items()
                     if remote.get(name) != sha)
    if not to_push:
        logging.info('Tags already up to date (%d tag(s)).', len(local))
        return
    _push_refs_in_batches(repo, to_push, 'tag(s)')


def _ensure_gerrit_remote(repo: Path, url: str) -> None:
    """Point *repo*'s `gerrit` remote at *url*, adding or updating as needed."""
    current = _query('git', '-C', str(repo), 'remote', 'get-url',
                     GERRIT_REMOTE)
    if current.returncode != 0:
        _run('git', '-C', str(repo), 'remote', 'add', GERRIT_REMOTE, url)
    elif current.stdout.strip() != url:
        _run('git', '-C', str(repo), 'remote', 'set-url', GERRIT_REMOTE, url)


def _remote_branch_sha(repo: Path, head_ref: str) -> str | None:
    """Return the `gerrit` remote's current tip for *head_ref*, or None."""
    result = _query('git', '-C', str(repo), 'ls-remote', '--heads',
                    GERRIT_REMOTE, head_ref)
    line = result.stdout.strip()
    return line.split()[0] if line else None


def _commit_checkpoints(repo: Path,
                        head_ref: str,
                        chunk_size: int,
                        start: str | None = None) -> list[str]:
    """Return ancestor checkpoints of *head_ref* to push, oldest first.

    Lists first-parent history from *start* (exclusive; the root when None) to
    the tip and picks one commit every *chunk_size* commits, always ending at
    the tip. Pushing these in order advances the branch in fast-forwarding steps
    of *chunk_size* commits each, so a huge history is seeded without a single
    oversized push that would OOM Gerrit. One `rev-list` plus a slice keeps this
    cheap even on million-commit histories. Empty means nothing to push.
    """
    rev_range = head_ref if start is None else f'{start}..{head_ref}'
    commits = _query('git', '-C', str(repo), 'rev-list', '--reverse',
                     '--first-parent', rev_range).stdout.split()
    if not commits:
        return []

    checkpoints = commits[chunk_size - 1::chunk_size]
    # Always finish exactly at the tip.
    if not checkpoints or checkpoints[-1] != commits[-1]:
        checkpoints.append(commits[-1])
    return checkpoints


def _seed_branch(repo: Path, branch_ref: str) -> None:
    """Advance the `gerrit` remote's *branch_ref* to local, in commit chunks.

    Resumes from the remote's current tip (the whole branch when the remote
    lacks it), then force-pushes ancestor checkpoints in order so a huge history
    lands without a single oversized push.
    """
    start = _remote_branch_sha(repo, branch_ref)
    logging.info('Computing seed checkpoints for %s...', branch_ref)
    checkpoints = _commit_checkpoints(repo, branch_ref,
                                      LARGE_REPO_COMMIT_CHUNK, start)
    if not checkpoints:
        logging.info('%s already up to date.', branch_ref)
        return
    logging.info('Seeding %s in %d push(es) of up to %d commits each',
                 branch_ref, len(checkpoints), LARGE_REPO_COMMIT_CHUNK)
    for index, sha in enumerate(checkpoints, start=1):
        logging.info('  push %d/%d -> %s', index, len(checkpoints), sha[:12])
        _run('git', '-C', str(repo), 'push', '-o', SKIP_VALIDATION_OPTION,
             GERRIT_REMOTE, f'+{sha}:{branch_ref}')


def _channel_branches(repo: Path) -> list[str]:
    """Return *repo*'s local channel-release branches (`N.N.x`), sorted."""
    refs = _query('git', '-C', str(repo), 'for-each-ref',
                  '--format=%(refname)', 'refs/heads/').stdout.split()
    return sorted(ref for ref in refs if CHANNEL_BRANCH_RE.match(ref))


def _ensure_managed_repo(mirror: ManagedMirror, git_cache_path: Path) -> Path:
    """Create/update *mirror*'s bare repo and sync the refs we mirror.

    Whether the bare repo is freshly initialised or reused, it is brought fully
    up to date every run: the default branch, the channel-release branches
    (`N.N.x`), and all tags are force-fetched from *mirror*.url -- selected via
    `ls-remote` so unrelated branches (feature/PR branches) are never pulled --
    and channel branches that upstream has since deleted are dropped locally, so
    the repo always reflects upstream's current state. Returns that repo path.
    """
    repo = git_cache_path / MANAGED_SUBDIR / mirror.name
    if _is_bare_repo(repo):
        # Reuse the existing bare repo -- never re-clone -- and fetch on top of
        # it, so a persistent cache only pulls what's new each run.
        logging.info('Reusing managed repo at %s', repo)
        _run('git', '-C', str(repo), 'remote', 'set-url', 'origin', mirror.url)
    else:
        logging.info('Initialising managed repo at %s', repo)
        repo.parent.mkdir(parents=True, exist_ok=True)
        _run('git', 'init', '--bare', str(repo))
        _run('git', '-C', str(repo), 'remote', 'add', 'origin', mirror.url)

    # The branches to mirror right now: the default branch, plus channel
    # branches when the mirror opts in.
    listing = _query('git', '-C', str(repo), 'ls-remote', '--heads',
                     'origin').stdout
    branches = set()
    for line in listing.splitlines():
        _, tab, ref = line.partition('\t')
        if not tab:
            continue
        if ref == mirror.branch or (mirror.channels
                                    and CHANNEL_BRANCH_RE.match(ref)):
            branches.add(ref)

    # Drop channel branches upstream no longer has, so a reused repo mirrors the
    # current set rather than an ever-growing one.
    if mirror.channels:
        for ref in _channel_branches(repo):
            if ref not in branches:
                logging.info('Pruning stale channel branch %s', ref)
                _run('git', '-C', str(repo), 'update-ref', '-d', ref)

    # Force-fetch the current branches (and tags when opted in; `--prune` drops
    # removed tags), so the mirrored refs are always at upstream's latest.
    refspecs = [f'+{ref}:{ref}' for ref in sorted(branches)]
    if mirror.tags:
        refspecs.append('+refs/tags/*:refs/tags/*')
    _run('git', '-C', str(repo), 'fetch', '--prune', 'origin', *refspecs)
    return repo


def refresh_managed(mirror: ManagedMirror, git_cache_path: Path,
                    gerrit: Gerrit) -> bool:
    """Fetch *mirror* from its upstream and publish it to its Gerrit project.

    Seeds the default branch in commit chunks (like chromium/src), then -- when
    the mirror opts in -- the channel-release branches (batched) and the tags
    (batched, incremental). The project must already exist; it is never created.

    Raises:
        subprocess.CalledProcessError: On a git failure (fetch or push).
    """
    extras = ''.join(
        part for part, on in [(' + channels',
                               mirror.channels), (' + tags', mirror.tags)]
        if on)
    logging.info('Mirroring %s (%s%s) -> %s [managed]', mirror.name,
                 mirror.branch, extras, mirror.project)
    if gerrit.dry_run:
        logging.info('[dry-run] would fetch %s and mirror %s%s -> %s',
                     mirror.url, mirror.branch, extras, mirror.project)
        return True

    repo = _ensure_managed_repo(mirror, git_cache_path)
    _ensure_gerrit_remote(repo, gerrit.project_url(mirror.project))

    if _local_ref_exists(repo, mirror.branch):
        _seed_branch(repo, mirror.branch)
    if mirror.channels:
        channels = _channel_branches(repo)
        if channels:
            _push_refs_in_batches(repo, channels, 'channel branch(es)')
    if mirror.tags:
        _push_new_tags(repo)
    return True


def refresh_repo(repo: Path, gerrit: Gerrit) -> bool:
    """Mirror a single cache *repo* into Gerrit.

    Derives the project from the upstream URL, creates it if absent, then
    force-pushes the repo's default branch (the ref HEAD points at) to it via a
    `gerrit` remote on the cache repo. Large repos (LARGE_REPOS) are *always*
    advanced through commit-chunked checkpoints -- resuming from whatever the
    remote already has -- since one big push would exceed Gerrit's receive
    timeout (or OOM its unpacker); they never issue a single full-ref push.

    Returns:
        True if the mirror was handled (pushed, or already in sync); False if
        the repo was skipped (no upstream URL, or no branch to mirror).

    Raises:
        subprocess.CalledProcessError: On a git failure (e.g. the push).
        RuntimeError: On a Gerrit query failure or unusable URL.
    """
    upstream = _upstream_url(repo)
    if upstream is None:
        logging.warning('%s has no %s remote URL; skipping.', repo.name,
                        UPSTREAM_REMOTE)
        return False

    # The default branch HEAD points at, when it resolves. When it doesn't
    # (detached/unborn/dangling HEAD, as on Chromium external mirrors), fall back
    # to mirroring every branch so the repo's commits still reach Gerrit.
    head_ref = _head_ref(repo)
    if head_ref is None and not _has_local_heads(repo):
        logging.warning('%s has no branch to mirror; skipping.', repo.name)
        return False

    project = project_name_for(upstream)
    target = head_ref or 'refs/heads/* (HEAD unresolved)'
    logging.info('Mirroring %s (%s) -> %s', repo.name, target, project)

    exists = gerrit.project_exists(project)
    if not exists:
        logging.info('Project %s not found; creating it.', project)
        gerrit.create_project(project)
    else:
        logging.info('Project %s already exists.', project)

    if gerrit.dry_run:
        seeded = ' (seeded in chunks)' if project in LARGE_REPOS and head_ref \
            else ''
        logging.info('[dry-run] would push %s to %s%s', target,
                     gerrit.project_url(project), seeded)
        return True

    _ensure_gerrit_remote(repo, gerrit.project_url(project))

    # A large repo must ALWAYS advance in chunks: a single push of its full
    # history (or a big catch-up) exceeds Gerrit's receive timeout / OOMs its
    # unpacker. Resume from the remote's current tip so a partial seed continues
    # where it stopped, and never fall through to the full-ref push below.
    if project in LARGE_REPOS:
        if head_ref is None:
            logging.warning(
                '%s is large but HEAD does not resolve; skipping to avoid a '
                'bulk push.', project)
            return False
        _seed_branch(repo, head_ref)
        # Publish tags too (release tags point off the default branch); batched
        # and incremental so only newly cut tags ship each run.
        _push_new_tags(repo)
        return True

    # Push the default branch when HEAD resolves; otherwise mirror every branch
    # (batched) so a dangling-HEAD repo isn't lost. Forced, so the mirror tracks
    # upstream even across a history rewrite.
    if head_ref is not None:
        _run('git', '-C', str(repo), 'push', '-o', SKIP_VALIDATION_OPTION,
             GERRIT_REMOTE, f'+{head_ref}:{head_ref}')
    else:
        _mirror_all_heads(repo)
    return True


def _log_summary(updated: list[str], skipped: list[str],
                 failed: list[str]) -> None:
    """Print a final tally of the run, listing updated and failed repos."""
    logging.info('')
    logging.info('==== Mirror refresh summary ====')
    logging.info('updated: %d  skipped: %d  failed: %d', len(updated),
                 len(skipped), len(failed))
    if updated:
        logging.info('Updated:')
        for name in updated:
            logging.info('  + %s', name)
    if skipped:
        # Skipped for a repo-specific reason logged inline when it happened
        # (no upstream URL, no branch to mirror, or published as a managed
        # mirror instead of from the cache).
        logging.info('Skipped:')
        for name in skipped:
            logging.info('  . %s', name)
    if failed:
        logging.error('Failed:')
        for name in failed:
            logging.error('  ! %s', name)


def refresh_mirrors(git_cache_path: Path,
                    gerrit: Gerrit,
                    refresh_owned_repos: bool = True,
                    refresh_cache_mirrors: bool = True) -> int:
    """Mirror the owned repos and the cache repos. Returns a failure count.

    A failure on one repo is logged and the run continues with the rest, so one
    bad mirror never blocks the others. Owned (managed) repos (MANAGED_MIRRORS,
    e.g. brave-core) that aren't in the cache are fetched and mirrored first,
    then the cache repos. Either phase can be turned off:

    Args:
        refresh_owned_repos: When False, skip the owned (managed) repos.
        refresh_cache_mirrors: When False, skip the git-cache mirrors.

    A summary of the outcome -- which repos were updated, skipped, and failed --
    is printed at the end.
    """
    updated: list[str] = []
    skipped: list[str] = []
    failed: list[str] = []

    # Owned repos first: they clone/fetch their own upstream and don't depend on
    # the cache, so publish them before walking the cache repos.
    if refresh_owned_repos:
        for mirror in MANAGED_MIRRORS:
            try:
                refresh_managed(mirror, git_cache_path, gerrit)
                updated.append(mirror.name)
            except (subprocess.CalledProcessError, RuntimeError,
                    ValueError) as e:
                failed.append(mirror.name)
                logging.error('Failed to mirror %s: %s', mirror.name, e)
    else:
        logging.info(
            'Skipping owned repos refresh (--no-owned-repos-refresh).')

    if refresh_cache_mirrors:
        repos = discover_cache_repos(git_cache_path)
        logging.info('Found %d cache repo(s) under %s', len(repos),
                     git_cache_path)
        for repo in repos:
            try:
                if refresh_repo(repo, gerrit):
                    updated.append(repo.name)
                else:
                    skipped.append(repo.name)
            except (subprocess.CalledProcessError, RuntimeError,
                    ValueError) as e:
                failed.append(repo.name)
                logging.error('Failed to mirror %s: %s', repo.name, e)
    else:
        logging.info('Skipping cache mirrors refresh (--no-mirrors-refresh).')

    _log_summary(updated, skipped, failed)
    return len(failed)


def main() -> int:
    """Parse arguments and refresh every mirror under `GIT_CACHE_PATH`."""
    parser = argparse.ArgumentParser(
        description='Publish the local git cache into the Gerrit mirrors.')
    parser.add_argument(
        '--git-cache-path',
        default=os.environ.get('GIT_CACHE_PATH'),
        help='Root of the git cache (defaults to $GIT_CACHE_PATH).')
    parser.add_argument(
        '--user',
        required=True,
        help='Gerrit SSH username, e.g. "chromium-mirror-bot".')
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Log the project creates and pushes without performing them.')
    parser.add_argument(
        '--no-mirrors-refresh',
        action='store_true',
        help="Don't refresh the git-cache mirrors (the `mirror/...` projects)."
    )
    parser.add_argument(
        '--no-owned-repos-refresh',
        action='store_true',
        help="Don't refresh the owned repos (brave-core, the depot_tools "
        'fork, ...).')
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

    failures = refresh_mirrors(
        git_cache_path,
        gerrit,
        refresh_owned_repos=not args.no_owned_repos_refresh,
        refresh_cache_mirrors=not args.no_mirrors_refresh)
    if failures:
        logging.error('%d mirror(s) failed.', failures)
    return 1 if failures else 0


if __name__ == '__main__':
    raise SystemExit(main())
