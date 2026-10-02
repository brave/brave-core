# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `git` module API.

Modelled on depot_tools' `recipes/recipe_modules/git`. Only `__call__` is
ported from it; its other methods have no consumer here.
"""

from __future__ import annotations

from collections.abc import Mapping
import dataclasses
import enum
from pathlib import Path
import re
from typing import Any, TYPE_CHECKING

import config_types
from recipe_api import RecipeApi
from step_data import StepData

if TYPE_CHECKING:
    from recipe_modules import git

# When disabling auto-gc, the keys we want to set.
_DISABLE_AUTO_GC_CONFIG = (
    ('gc.auto', '0'),
    ('gc.autodetach', '0'),
    ('gc.autopacklimit', '0'),
    # New versions of git requrire this to be set too on what used to be under
    # gc functionality.
    ('maintenance.gc.enabled', 'false'),
)

# What a branch, tag or ref name may look like. It must not start with `-`, so
# that git never reads it as an option.
_REF_RE = re.compile(r'[A-Za-z0-9_][A-Za-z0-9._/-]*')

_COMMIT_RE = re.compile(r'[0-9a-fA-F]{40}')

_HEADS = 'refs/heads/'
_TAGS = 'refs/tags/'


class RefKind(enum.Enum):
    """What a `GitRef` names."""

    # A branch, `refs/heads/*`. Mirrored by default.
    HEAD = enum.auto()

    # A tag, `refs/tags/*`. Mirrored by name.
    TAG = enum.auto()

    # A full commit hash. Mirrored with `--commit`.
    COMMIT = enum.auto()

    # Any other ref, e.g. `refs/branch-heads/*`. Mirrored by name.
    OTHER = enum.auto()


@dataclasses.dataclass(frozen=True)
class GitRef:
    """A fully-qualified ref, or a commit hash, to check out."""

    # The ref as given: `refs/...`, or the commit hash.
    name: str

    # What `name` refers to; decides how it is mirrored and fetched.
    kind: RefKind

    @classmethod
    def parse(cls, ref: str) -> GitRef:
        """Classify *ref*, which is what decides how it is mirrored and fetched.

        Args:
            ref: A fully-qualified ref (`refs/heads/main`, `refs/tags/v1`,
                `refs/branch-heads/6834`) or a full commit hash. Bare names are
                refused, as nothing tells a branch from a tag.

        Raises:
            ValueError: If *ref* is neither, or is not a plausible name.
        """
        if not _REF_RE.fullmatch(ref):
            raise ValueError(f'invalid ref: {ref!r}')
        if _COMMIT_RE.fullmatch(ref):
            return cls(ref, RefKind.COMMIT)
        if ref.startswith(_HEADS):
            return cls(ref, RefKind.HEAD)
        if ref.startswith(_TAGS):
            return cls(ref, RefKind.TAG)
        if ref.startswith('refs/'):
            return cls(ref, RefKind.OTHER)
        raise ValueError(
            f'ref must be fully qualified (refs/heads/..., refs/tags/...) or a '
            f'commit hash: {ref!r}'
        )

    @property
    def populate_ref(self) -> str | None:
        """The ref to mirror with `git cache populate --ref`. Branches are
        mirrored anyway, and a commit goes through `--commit`."""
        return (
            None if self.kind in (RefKind.HEAD, RefKind.COMMIT) else self.name
        )

    @property
    def populate_commit(self) -> str | None:
        """The hash to mirror with `git cache populate --commit`."""
        return self.name if self.kind == RefKind.COMMIT else None

    @property
    def branch(self) -> str:
        """The branch name, without `refs/heads/`.

        Raises:
            ValueError: If this is not a branch.
        """
        self._require(RefKind.HEAD)
        return self.name[len(_HEADS) :]

    @property
    def tag(self) -> str:
        """The tag name, without `refs/tags/`.

        Raises:
            ValueError: If this is not a tag.
        """
        self._require(RefKind.TAG)
        return self.name[len(_TAGS) :]

    @property
    def commit(self) -> str:
        """The commit hash.

        Raises:
            ValueError: If this is not a commit.
        """
        self._require(RefKind.COMMIT)
        return self.name

    @property
    def short_name(self) -> str:
        """The branch or tag name, as `git clone --branch` takes it.

        Raises:
            ValueError: If this is neither a branch nor a tag.
        """
        if self.kind == RefKind.HEAD:
            return self.branch
        if self.kind == RefKind.TAG:
            return self.tag
        raise ValueError(f'{self.name!r} is not a branch or a tag')

    def _require(self, kind: RefKind) -> None:
        if self.kind != kind:
            raise ValueError(
                f'{self.name!r} is a {self.kind.name.lower()} ref, not a '
                f'{kind.name.lower()} ref'
            )


class GitApi(RecipeApi):
    """Runs git commands, plus repository operations shared across modules."""

    m: git.DEPS

    def __call__(
        self,
        *args: str | Path | config_types.Path,
        name: str | None = None,
        git_config_options: Mapping[str, str] | None = None,
        **kwargs: Any,
    ) -> StepData:
        """Run `git *args` as a step.

        Unlike upstream, there is no `infra_step`, which the `step` module does
        not have, and no fallback to a checkout directory when `context.cwd` is
        unset, as the `path` module has none; the step then runs in the engine's
        cwd.

        Args:
            args: The git subcommand and its arguments.
            name: The step name; `git <subcommand>` if not given.
            git_config_options: Passed as `-c key=value` before *args*, sorted
                by key.
            kwargs: Forwarded to the `step` module (`cwd`, `stdout`, `check`,
                ...).

        Returns:
            The step's `StepData`.
        """
        if name is None:
            name = f'git {args[0]}'
        git_cmd = ['git']
        for k, v in sorted((git_config_options or {}).items()):
            git_cmd.extend(['-c', f'{k}={v}'])
        return self.m.step(name, [*git_cmd, *args], **kwargs)

    def disable_auto_gc(
        self, repo: str | Path, *, step_name: str = 'disable auto-gc'
    ) -> None:
        """Stop git's own automatic gc/maintenance from running in *repo*.

        Disabling auto-gc/maintenance is useful in our infra, as a lot of
        workspaces have a limited lifetime, and gc work means resources spent on
        a repo that will be discarded at some point.

        Args:
            repo: Path to the git repository, working tree or bare mirror
                alike; passed as `cwd`, so git's own repository discovery
                resolves it either way.
            step_name: Prefix for each `git config` step's name, so callers
                setting this up at multiple points (e.g. before and after
                another step) can tell them apart in the step list.
        """
        for key, value in _DISABLE_AUTO_GC_CONFIG:
            self(
                'config',
                key,
                value,
                name=f'{step_name}: {key}={value}',
                cwd=repo,
            )

    def parse_ref(self, ref: str) -> GitRef:
        """Classify *ref*; see `GitRef.parse`."""
        return GitRef.parse(ref)
