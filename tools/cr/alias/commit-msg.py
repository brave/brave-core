#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Notice: Keep this script free of dependencies outside the standard library,
# so users can install it with little effort and no deps surprises. The one
# `tools/cr` module it uses, `pinned_commits`, keeps to the standard library
# too.

# This script has several convenience features when committing changes to
# brave-core.
#
# 1. Chromium upgrade branches automatically added as tags
#
#    Any branch that starts with "cr" and is followed by a number will be added
#    to the commit message as a tag (e.g. [cr100] Some change).
#
# 2. Tags can be passed in via the environment variable tags
#
#    For instance `tags=CodeHealth,canary git commit -m "Some change"` will
#    add [CodeHealth] and [canary] to the commit message.
#
# 3. Labels in the branch name behind '+' will be added as tags
#
#    This means that a branch `canary+fix-failure` will have [canary] added to
#    the commit message.
#
# 4. `culprit` environment variable for upstream commit links
#
#    If the culprit environment variable is set, it will be used to add links to
#    the commit message. The culprit variable should be a comma-separated list
#    of commit hashes. The script will look up the commit messages for each hash
#    and add them to the commit message, with a link to each commit built from
#    its repository's `origin` push URL: on googlesource,
#    https://chromium.googlesource.com/chromium/src/+/{hash}, and on GitHub,
#    https://github.com/{owner}/{repo}/commit/{hash}.
#
#    A culprit may also target a subrepo (e.g. v8, that lives outside of the
#    Chromium checkout) using the `<subrepo-path>:<hash>` syntax. For instance
#    `culprit=v8/src:3ba47f111` looks up the commit in `../v8/src` and links to
#    it through that checkout's remote (e.g.
#    https://chromium.googlesource.com/v8/v8/+/{hash}). A bare hash resolves
#    against the Chromium checkout at `../`.
#
#    Each culprit is written as its full hash, however short the one given, and
#    one that isn't a commit stops the commit. With `culprit_in_range=1` (what
#    `git cr commit --culprit-in-range` sets), a Chromium culprit on a `cr<N>`
#    branch must also be in the branch's range: after the Chromium tag its
#    upstream's `package.json` pins, up to the one HEAD's pins. Without it, a
#    culprit from before the range is fine, e.g. for a fix of a latent issue.
#
# 5. `issue` environment variable for "Resolves" links
#
#    The `issue` environment variable can be set to a comma-separated list of
#    issue numbers. The script will add "Resolves" links at the end of the
#    commit message.
#
# 6. `fixup!` commits are ignored
#
#    If the commit message starts with "fixup!", the script will not add any
#    tags or links to the commit message. A `fixup!`, `amend!` or `squash!` of
#    a pinned commit (see `tools/cr/pinned_commits.py`) is refused:
#    `brockit rebase` squashes a pinned group and moves it, so the fixup can't
#    be replayed reliably. Such a change is a commit of its own with the same
#    subject, which the rebase squashes into the group.
#
# 7. Issue numbers in the branch name
#
#    If the branch name contains "issue-" followed by a number, the script will
#    add a "Resolves" link to the commit message. For example, if the branch
#    name is "codehealth+fix-issue-123", the commit message will include a link
#    to "Resolves https://github.com/brave/brave-browser/issues/123".
#
# 8. `force_commit_hook` environment variable
#
#    The hook leaves commit messages alone during a rebase or cherry-pick, and
#    for `fixup!` and `amend!` commits. Setting `force_commit_hook=1` (what
#    `git cr commit --force-commit-hook` does) runs it anyway, for a commit made
#    by hand while a rebase is stopped. During a rebase, the branch's tags,
#    upstream and range come from the branch being rebased. A `fixup!` or
#    `amend!` subject gets no tags, so autosquash still finds its target.
#
# 9. Gerrit `Change-Id` trailer
#
#    Every commit gets a `Change-Id: I<sha1>` trailer, made and placed as
#    Gerrit's own commit-msg hook does (`gerrit.createChangeId` and
#    `gerrit.reviewUrl` included), so that a re-upload of a rebased commit
#    becomes a new patch set of the same change. A Change-Id trailer the
#    message already has is never replaced: its trailers move below the links
#    the hook adds. Those in quoted culprit messages don't count. Commits whose
#    subject is a directive (`fixup!`, `amend!`, `squash!`, `reassign!<hash>!`,
#    `drop!<hash>!`, ...) get none, as they fold into another commit.
#
# This script can be set up either as a commit-msg hook, or as a
# prepare-commit-msg hook. The difference is that the commit-msg hook will run
# after the commit message is created, while the prepare-commit-msg hook will
# run before the commit message is created, which allows you to inspect the
# insertions.
#
# To install this script as a commit-msg hook, run `git cr install-hook`,
# which links the hook to this file rather than copying it.
#

import json
import os
import sys
import subprocess
import re
from pathlib import Path

# `tools/cr`, found through this file even when git runs it via the hook's
# symlink.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
try:
    import pinned_commits
except ImportError:
    print(
        "The commit-msg hook can't find `tools/cr/pinned_commits.py`: it is "
        'probably a copy. Reinstall it with `git cr install-hook`.',
        file=sys.stderr,
    )
    sys.exit(1)

_SQUASH_MARKERS = ('fixup!', 'amend!', 'squash!')

# A subject that tells a rebase what to do with another commit, as Gerrit's
# hook recognises them, plus brockit's `reassign!<hash>!` and `drop!<hash>!`.
_DIRECTIVE = re.compile(r'[a-z]+!')
_SCISSORS = '------------------------ >8 ------------------------'


def pinned_squash_problem(first_line: str) -> str | None:
    """Says why a `fixup!`/`amend!`/`squash!` commit can't target the commit
    its subject names, if that commit is pinned."""
    if not first_line.startswith(_SQUASH_MARKERS):
        return None
    if pinned_commits.get_pinned_group_for_subject(first_line) is None:
        return None
    target = pinned_commits.strip_autosquash_prefixes(first_line).strip()
    subject = pinned_commits.strip_cr_tag(target)
    return (
        f'"{first_line.split("!", 1)[0]}!" can\'t target the pinned commit '
        f'"{target}": `brockit rebase` squashes every commit with a pinned '
        'subject into one and moves the group, so the fixup has nothing '
        'stable to replay onto. Commit the change on its own, with the same '
        f'subject, and the rebase squashes it into the group:\n'
        f"    git commit -m '{subject}'"
    )


PREFIXES_FOR_UPGRADE_COMMITS = [
    'Update from Chromium ',
    'Apply-fixed 🩹 patches from Chromium ',
    'Conflict-resolved patches from Chromium ',
    'Update patches from Chromium ',
    'Updated strings for Chromium ',
]

# A googlesource remote (`https://<host>.googlesource.com/[a/]<project>[.git]`),
# whose commits browse at `/<project>/+/<hash>`.
_GOOGLESOURCE_REMOTE = re.compile(
    r'https://([\w.-]+\.googlesource\.com)/(?:a/)?(.+?)(?:\.git)?/?'
)
# A GitHub remote, over https or ssh, whose commits browse at
# `/<owner>/<repo>/commit/<hash>`.
_GITHUB_REMOTE = re.compile(
    r'(?:https://|ssh://git@|git@)github\.com[:/](.+?)(?:\.git)?/?'
)


def parse_culprit(culprit: str) -> tuple[str, str]:
    """Splits a culprit value into its subrepo path and commit hash.

    A bare hash (e.g. `abc123`) targets the Chromium checkout, returning a
    subrepo of ''. A `<subrepo-path>:<hash>` value (e.g. `v8/src:abc123`)
    targets a repo that lives outside the Chromium checkout, at
    `../<subrepo-path>`.
    """
    repo, separator, commit_hash = culprit.partition(':')
    if separator:
        return repo, commit_hash
    return '', repo


def _git(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        ['git', *args], capture_output=True, text=True, check=False
    )


def commit_url_base(git_dir: str) -> str | None:
    """The URL a commit hash completes, from `git_dir`'s origin push URL.

    The push URL rather than the fetch one, as a checkout fetching from a git
    cache mirror still pushes to the real remote. None for a remote that is
    neither on googlesource nor on GitHub.
    """
    result = _git('-C', git_dir, 'remote', 'get-url', '--push', 'origin')
    url = result.stdout.strip()
    if match := _GOOGLESOURCE_REMOTE.fullmatch(url):
        host, project = match.groups()
        return f'https://{host}/{project}/+/'
    if match := _GITHUB_REMOTE.fullmatch(url):
        return f'https://github.com/{match.group(1)}/commit/'
    return None


def resolve_commit(git_dir: str, commit_hash: str) -> str | None:
    """The full hash of `commit_hash` in `git_dir`, or None if it isn't one."""
    result = _git(
        '-C',
        git_dir,
        'rev-parse',
        '--verify',
        '--quiet',
        f'{commit_hash}^{{commit}}',
    )
    return result.stdout.strip() if result.returncode == 0 else None


def _chromium_tag(ref: str) -> str | None:
    """The Chromium tag brave-core's `package.json` pins at `ref`."""
    result = _git('show', f'{ref}:package.json')
    if result.returncode:
        return None
    try:
        return json.loads(result.stdout)['config']['projects']['chrome']['tag']
    except (ValueError, KeyError, TypeError):
        return None


def current_branch() -> str:
    """The branch being committed to: the one being rebased, during a rebase,
    as HEAD is detached then."""
    for state in ('rebase-merge', 'rebase-apply'):
        head_name = _git('rev-parse', '--git-path', f'{state}/head-name')
        path = Path(head_name.stdout.strip())
        if head_name.returncode == 0 and path.is_file():
            ref = path.read_bytes().decode('utf-8').strip()
            return ref.removeprefix('refs/heads/')
    return _git('rev-parse', '--abbrev-ref', 'HEAD').stdout.strip()


def culprit_range_problem(
    git_dir: str, full_hash: str, branch: str
) -> str | None:
    """Says why a Chromium culprit is outside the branch's range, if it is.

    On a `cr<N>` branch, an upstream culprit is a commit between the
    Chromium tag its upstream (the base) pins, and the one HEAD pins. The
    check is skipped, with a warning, when those tags can't be resolved.
    """
    upstream = _git('rev-parse', '--abbrev-ref', f'{branch}@{{upstream}}')
    if upstream.returncode:
        return None
    old, new = _chromium_tag(upstream.stdout.strip()), _chromium_tag('HEAD')
    if not old or not new or old == new:
        return None
    for tag in (old, new):
        if resolve_commit(git_dir, f'refs/tags/{tag}') is None:
            print(
                f"Warning: tag {tag} isn't in {git_dir}, so the culprit's "
                'range is not checked.',
                file=sys.stderr,
            )
            return None

    def is_ancestor(ancestor: str, tag: str) -> bool:
        return not _git(
            '-C',
            git_dir,
            'merge-base',
            '--is-ancestor',
            ancestor,
            f'refs/tags/{tag}',
        ).returncode

    if is_ancestor(full_hash, new) and not is_ancestor(full_hash, old):
        return None
    return (
        f'Culprit {full_hash} is not in the branch\'s Chromium range, '
        f'{old}..{new}. Commit without --culprit-in-range to cite it anyway, '
        'or without --culprit if no upstream change is to blame.'
    )


def operation_in_progress() -> bool:
    """Whether a rebase or cherry-pick is in progress, in this worktree."""
    for state in ('rebase-merge', 'rebase-apply', 'CHERRY_PICK_HEAD'):
        path = _git('rev-parse', '--git-path', state).stdout.strip()
        if path and os.path.exists(path):
            return True
    return False


def split_editor_tail(message: str) -> tuple[str, str]:
    """Splits off what git strips after the hook: everything from the
    scissors line, and the comment lines the message ends with."""
    comment = _git('config', 'core.commentChar').stdout.strip()
    if len(comment) != 1:
        comment = '#'
    lines = message.splitlines(keepends=True)
    end = next(
        (
            i
            for i, line in enumerate(lines)
            if line.rstrip('\r\n') == f'{comment} {_SCISSORS}'
        ),
        len(lines),
    )
    while end and (not lines[end - 1].strip() or lines[end - 1][0] == comment):
        end -= 1
    return ''.join(lines[:end]), ''.join(lines[end:])


def _git_filter(args: list[str], text: str) -> str:
    """Runs git over `text` on stdin, in bytes so newlines stay as they are
    on Windows, and returns its output."""
    return subprocess.run(
        ['git', *args],
        input=text.encode('utf-8'),
        capture_output=True,
        check=True,
    ).stdout.decode('utf-8')


class GerritTrailer:
    """The trailer that names a commit's Gerrit change, made and placed as
    Gerrit's own commit-msg hook does (`tools/hooks/commit-msg` on any Gerrit
    server), whose checks it keeps."""

    def __init__(self, message: str):
        head = _git('rev-parse', '--verify', '--quiet', 'HEAD').stdout.strip()
        if not head:
            head = _git_filter(['hash-object', '-t', 'tree', '--stdin'], '')
        ident = _git('var', 'GIT_COMMITTER_IDENT').stdout.strip()
        random = _git_filter(
            ['hash-object', '--stdin'], f'{ident}\n{head.strip()}\n{message}'
        ).strip()
        review_url = _git('config', '--get', 'gerrit.reviewUrl').stdout.strip()
        if review_url:
            self.token = 'Link'
            self.value = f'{review_url.rstrip("/")}/id/I{random}'
            self._pattern = re.compile(r'Link: .*/id/I[0-9a-f]{40}')
        else:
            self.token = 'Change-Id'
            self.value = f'I{random}'
            self._pattern = re.compile(r'Change-Id: .*')

    @staticmethod
    def wanted(first_line: str) -> bool:
        """Whether `gerrit.createChangeId` and the subject call for one: not
        for a directive, which folds into another commit, unless `always`."""
        setting = _git('config', '--get', 'gerrit.createChangeId').stdout
        if setting.strip() == 'false':
            return False
        return setting.strip() == 'always' or not _DIRECTIVE.match(first_line)

    @staticmethod
    def is_empty(message: str) -> bool:
        """Whether nothing is left once comments and the scissors part go."""
        lines = message.splitlines(keepends=True)
        cut = next((i for i, line in enumerate(lines) if '>8' in line), None)
        if cut is not None:
            lines = lines[: cut + 1]
        stripped = _git_filter(
            ['stripspace', '--strip-comments'], ''.join(lines)
        )
        return not stripped

    def is_in(self, message: str) -> bool:
        """Whether the message's trailers already name a change. Indented
        lines, as in quoted culprit messages, aren't trailers."""
        trailers = _git_filter(
            [
                '-c',
                'trailer.separators=:=',
                'interpret-trailers',
                '--no-divider',
                '--parse',
            ],
            message,
        )
        return any(map(self._pattern.fullmatch, trailers.splitlines()))

    def add_to(self, message: str) -> str:
        """Adds the trailer to the message's trailers, before any
        `Signed-off-by`, through a sentinel as Gerrit's hook does."""
        with_sentinel = _git_filter(
            [
                'interpret-trailers',
                '--no-divider',
                '--trailer',
                'Signed-off-by: SENTINEL',
            ],
            message,
        )
        added = _git_filter(
            [
                '-c',
                'trailer.where=before',
                'interpret-trailers',
                '--no-divider',
                '--trailer',
                f'Signed-off-by: {self.token}: {self.value}',
            ],
            with_sentinel,
        )
        added = re.sub(
            rf'^Signed-off-by: ({re.escape(self.token)}: )',
            r'\1',
            added,
            flags=re.M,
        )
        return re.sub(r'^Signed-off-by: SENTINEL.*\n?', '', added, flags=re.M)


def split_trailers(body: str) -> tuple[str, str]:
    """Splits the last paragraph off the body, where git looks for its
    trailers. The subject is never one."""
    body = body.rstrip()
    breaks = list(re.finditer(r'\n[ \t]*\n', body))
    if not breaks:
        return body, ''
    return body[: breaks[-1].start()], body[breaks[-1].end() :]


def main():
    forced = os.getenv('force_commit_hook') == '1'
    if not forced and operation_in_progress():
        # There should be no changes to commit messages during a rebase or
        # cherry-pick.
        return 0

    commit_msg_file = Path(sys.argv[1])

    # Read the message provided so we can check for duplicate insertions
    # beforehand.
    commit_message = commit_msg_file.read_bytes().decode('utf-8')

    if not commit_message or not commit_message.split('\n', 1)[0].strip():
        # We should not commmit if the commit message first line is empty as
        # this will cause any tags to become the effective message.
        return 1

    if problem := pinned_squash_problem(commit_message.split('\n', 1)[0]):
        print(problem, file=sys.stderr)
        return 1

    # Skip fixup! changes.
    squash_marker = commit_message.startswith(('fixup!', 'amend!'))
    if squash_marker and not forced:
        return 0

    first_line = commit_message.split('\n', 1)[0]

    gerrit_trailer = None
    if GerritTrailer.wanted(first_line):
        if GerritTrailer.is_empty(commit_message):
            print(f'file is empty: {commit_msg_file}', file=sys.stderr)
            return 1
        gerrit_trailer = GerritTrailer(commit_message)
    # An existing trailer is kept as it is, with the trailers around it,
    # below what the hook adds, where Gerrit looks for it.
    kept_trailers = ''
    commit_message, editor_tail = split_editor_tail(commit_message)
    if gerrit_trailer and gerrit_trailer.is_in(commit_message):
        commit_message, kept_trailers = split_trailers(commit_message)
        gerrit_trailer = None

    # Getting the branch name to see if there's any tags that can be deduced
    # from it.
    branch_name = current_branch()

    tags = set()
    # Checking for chromium upgrade branches
    cr_branch_pattern = r"^cr\d+$"
    if re.fullmatch(cr_branch_pattern, branch_name):
        # The branch tag is excluded from the upgrade commit messages.
        if not any(
            suffix in first_line for suffix in PREFIXES_FOR_UPGRADE_COMMITS
        ):
            tags.add(branch_name)
    elif '+' in branch_name:
        # This is the simple case where `canary+fix-failure` produces a
        # [canary] tag.
        tags.update(branch_name.split('+')[:-1])

    # Checking for tags passed in via the environment variable tags
    tags_env = os.getenv("tags")
    if tags_env:
        tags.update(tags_env.split(','))

    # Making sure we are not re-inserting tags that are already in the commit
    # message, nor putting any in front of a `fixup!` that autosquash matches.
    tags = [
        f'[{tag}]'
        for tag in tags
        if f'[{tag}]' not in first_line and not squash_marker
    ]

    # These are the report links that are added to the commit message when
    # there's an upstream culprit.
    culprits = []
    for culprit in filter(None, (os.getenv("culprit") or '').split(',')):
        repo, commit_hash = parse_culprit(culprit)
        git_dir = os.path.join('..', repo)
        if _git('-C', git_dir, 'rev-parse', '--git-dir').returncode:
            print(
                f"Culprit subrepo '{repo}' is not a git checkout at {git_dir}.",
                file=sys.stderr,
            )
            return 1
        url_base = commit_url_base(git_dir)
        if url_base is None:
            print(
                f"Can't link to commits in {git_dir}: its origin push URL is "
                'neither on googlesource nor on GitHub.',
                file=sys.stderr,
            )
            return 1
        # Links and later checks need the full hash, whatever was passed.
        full_hash = resolve_commit(git_dir, commit_hash)
        if full_hash is None:
            print(
                f"Culprit '{commit_hash}' is not a commit in {git_dir}.",
                file=sys.stderr,
            )
            return 1
        if (
            os.getenv('culprit_in_range') == '1'
            and not repo
            and re.fullmatch(cr_branch_pattern, branch_name)
        ):
            if problem := culprit_range_problem(
                git_dir, full_hash, branch_name
            ):
                print(problem, file=sys.stderr)
                return 1
        culprits.append((git_dir, full_hash, url_base))
    # Skip culprits already in the commit message, even as a short hash.
    culprits = [
        culprit for culprit in culprits if culprit[1][:7] not in commit_message
    ]

    culprit_output = []
    culprit_links = []
    for git_dir, commit_hash, url_base in culprits:
        culprit_output.append(
            subprocess.check_output(
                ['git', '-C', git_dir, 'log', '-1', commit_hash]
            )
            .decode()
            .strip()
        )
        culprit_links.append(f'{url_base}{commit_hash}')

    if tags:
        if commit_message[0] == '[':
            # If the commit message already starts with a tag, do not add a
            # space in between them.
            commit_message = f'{"".join(tags)}{commit_message}'
        else:
            commit_message = f'{"".join(tags)} {commit_message}'

    issues = os.getenv("issue")
    if issues:
        issues = issues.split(',')
    else:
        issues = []

    # Extract issue numbers from the branch name
    issue_pattern = r"issue-(\d+)"
    branch_issues = re.findall(issue_pattern, branch_name)
    issues.extend(branch_issues)

    # Remove duplicates
    issues = list(set(issues))

    if issues:
        commit_message += '\n\n' + '\n'.join(
            [
                f"Resolves https://github.com/brave/brave-browser/issues/{issue}"
                for issue in issues
                if f'brave-browser/issues/{issue}' not in commit_message
            ]
        )

    if culprit_output:
        commit_message += (
            '\n\nChromium changes:\n{chromium_links}\n\n{culprits}'.format(
                chromium_links="\n".join(culprit_links),
                culprits="\n\n".join(culprit_output),
            )
        )

    commit_message = commit_message.rstrip('\n') + '\n'
    if kept_trailers:
        commit_message += f'\n{kept_trailers}\n'
    if gerrit_trailer:
        commit_message = gerrit_trailer.add_to(commit_message)
    if editor_tail:
        commit_message += f'\n{editor_tail}'

    with commit_msg_file.open('w', encoding='utf-8', newline='') as f:
        f.write(commit_message)

    return 0


if __name__ == "__main__":
    sys.exit(main())
