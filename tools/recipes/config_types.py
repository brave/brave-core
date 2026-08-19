# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Copyright 2013 The LUCI Authors
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.
"""A platform-independent `Path`, ported from recipes-py's `config_types.py`.
"""

from __future__ import annotations

import itertools

from dataclasses import dataclass, field
from typing import ClassVar, Generator


def reset_global_variable_assignments() -> None:
    """Resets `Path._OS_SEP` to `None`.

    Called by the test runner before each test case, so a case whose `DEPS`
    never instantiate the `path` module doesn't inherit a stale separator left
    behind by a previous case.
    """
    Path._OS_SEP = None


# These two exception classes inherit from ValueError because the
# corresponding errors in stdlib `pathlib` use ValueError.
class RelativeToDifferentBases(ValueError):
    pass


class RelativeToNotParent(ValueError):
    pass


@dataclass(frozen=True, order=True)
class ResolvedBasePath:
    """A resolved base path.

    In test mode this holds a literal placeholder string like `'[CACHE]'`,
    `'[HOME]'`, the token *being* the value, from construction, so there is
    nothing to rewrite into a token later. Outside of test mode it holds a
    real absolute filesystem path string.
    """
    resolved: str

    @classmethod
    def for_recipe_module(cls, test_enabled: bool, module_name: str,
                          module_dir: str) -> ResolvedBasePath:
        """The base for a recipe module's `resources/` directory.
        """
        if not test_enabled:
            return cls(module_dir)
        return cls(f'RECIPE_MODULE[{module_name}]')

    @classmethod
    def for_recipe_script_resources(cls, test_enabled: bool, recipe_name: str,
                                    resources_dir: str) -> ResolvedBasePath:
        """The base for a recipe script's `<recipe>.resources/` directory.
        """
        if not test_enabled:
            return cls(resources_dir)
        return cls(f'RECIPE[{recipe_name}].resources')

    def __repr__(self) -> str:
        return self.resolved


@dataclass(frozen=True)
class Path:
    """An absolute path, relative to a `ResolvedBasePath` base.

    Made aware of the currently simulated path separator via the `path`
    recipe module's `initialise()`, which assigns to this class's `_OS_SEP`.
    """
    base: ResolvedBasePath
    pieces: tuple[str, ...]

    # HACK: directly assigned to by the `path` recipe module, populated with
    # the current (possibly simulated) path separator character ('/' or
    # '\\'). Reset between test cases by reset_global_variable_assignments().
    _OS_SEP: ClassVar[str | None] = None

    # Caches the output of __str__. Not a @functools.cache on __str__ itself.
    # since that would create a global dict keyed by Path instances that
    # captures whatever _OS_SEP was at call time, even though _OS_SEP can
    # change between test cases -- the plain instance field self-invalidates
    # by construction (a fresh Path is built every time a base path changes).
    _str: str | None = field(default=None,
                             repr=False,
                             hash=False,
                             compare=False)

    def __init__(self, base: ResolvedBasePath, *pieces: str):
        """Creates a Path.

        Args:
            base: The ResolvedBasePath this Path is relative to.
            pieces: The components of the path relative to base.
                - Pieces are always split on '/'. If `_OS_SEP` is '\\'
                  (a simulated Windows run), pieces are also split on '\\'.
                - A '..' piece must not go above `base`: `Path(base, '..',
                  'x')` raises ValueError, but `Path(base, 'x', '..')` is OK
                  (equivalent to `Path(base)`).
                - Empty pieces and '.' pieces are ignored.
                - A piece containing '\\' before `_OS_SEP` is known (i.e.
                  before the `path` module has initialised) raises
                  ValueError -- use '/' instead.
        """
        super().__init__()
        if not isinstance(base, ResolvedBasePath):
            raise ValueError(
                'First argument to Path must be a ResolvedBasePath, got '
                f'{base!r} ({type(base)!r})')

        has_backslashes = False
        for i, piece in enumerate(pieces):
            if not isinstance(piece, str):
                raise ValueError(
                    'Variadic arguments to Path must only be `str`, '
                    f'argument {i} was {piece!r} ({type(piece)!r})')
            has_backslashes = has_backslashes or '\\' in piece

        # We always separate on '/', regardless of _OS_SEP, since callers
        # routinely pass pieces that already contain a slash. A backslash is
        # ambiguous before _OS_SEP is known, so it's rejected outright rather
        # than guessed at.
        if self._OS_SEP is None and has_backslashes:
            raise ValueError(
                f'Cannot instantiate Path({base!r}, {pieces!r}) - pieces '
                'contain a backslash and the path module has not been '
                'initialised yet. Use "/" (even for windows) or pass the '
                'pieces to join separately.')
        need_backslash_split = has_backslashes and self._OS_SEP == '\\'

        normalized_pieces = []
        for piece in pieces:
            slash_pieces = piece.split('/')
            if need_backslash_split:
                new_slash_pieces = []
                for sp in slash_pieces:
                    new_slash_pieces.extend(sp.split(self._OS_SEP))
                slash_pieces = new_slash_pieces
            normalized_pieces.extend(p for p in slash_pieces if p and p != '.')

        # Collapse '..' against the immediately preceding piece. Starting at
        # index 1 (not 0) means a leading '..' is never collapsed inline. it
        # falls through to the check below and raises instead.
        i = 1
        while 0 < i < len(normalized_pieces):
            piece = normalized_pieces[i]
            if piece == '..':
                normalized_pieces[i - 1:i + 1] = []
                i -= 1
            else:
                i += 1

        if normalized_pieces and normalized_pieces[0] == '..':
            raise ValueError(
                f'Unable to compute {base!r} / {pieces!r} without going '
                'above the base.')

        # Frozen dataclass: assign via object.__setattr__ (documented escape
        # hatch for frozen-instance __init__).
        object.__setattr__(self, 'base', base)
        object.__setattr__(self, 'pieces', tuple(normalized_pieces))

    @property
    def parents(self) -> Generator[Path, None, None]:
        """For 'foo/bar/baz', yield 'foo/bar' then 'foo'."""
        prev: Path = self
        curr: Path = self.parent
        while prev != curr:
            yield curr
            prev, curr = curr, curr.parent

    @property
    def parent(self) -> Path:
        """For 'foo/bar/baz', return 'foo/bar'."""
        return Path(self.base, *self.pieces[0:-1])

    @property
    def name(self) -> str:
        """For 'foo/bar/baz', return 'baz'."""
        return self.pieces[-1]

    @property
    def stem(self) -> str:
        """For 'dir/foo.tar.gz', return 'foo.tar'."""
        return self.name.rsplit('.', 1)[0]

    @property
    def suffix(self) -> str:
        """For 'dir/foo.tar.gz', return '.gz'."""
        parts = self.name.rsplit('.', 1)
        if len(parts) == 1:
            return ''
        return '.' + parts[1]

    @property
    def suffixes(self) -> list[str]:
        """For 'dir/foo.tar.gz', return ['.tar', '.gz']."""
        return [f'.{x}' for x in self.name.split('.')[1:]]

    def as_posix(self) -> str:
        """The '/'-joined form of this path, regardless of `_OS_SEP`.

        Not part of upstream recipes-py: needed so callers that must key a
        path deterministically (e.g. a simulated filesystem's lookup table)
        aren't at the mercy of whichever separator the current test case
        happens to simulate.
        """
        return '/'.join(itertools.chain((str(self.base), ), self.pieces))

    def __eq__(self, other: object) -> bool:
        if isinstance(other, str):
            return str(self) == other
        if not isinstance(other, Path):
            return NotImplemented
        return self.base == other.base and self.pieces == other.pieces

    def __lt__(self, other: object) -> bool:
        if isinstance(other, str):
            return str(self) < other
        if not isinstance(other, Path):
            return NotImplemented
        return (self.base, self.pieces) < (other.base, other.pieces)

    def __truediv__(self, piece: str | Path) -> Path:
        """Shorthand '/'-operator for .joinpath(), returning a new path."""
        return self.joinpath(piece)

    def joinpath(self, *pieces: str | Path) -> Path:
        """Appends *pieces to this Path, returning a new Path.

        Args:
            pieces: The components of the path relative to base. If a
                component is a Path instance, the returned path is
                equivalent to calling joinpath on that component with any
                following components (an absolute-path "reroot", matching
                stdlib pathlib's joinpath behavior for absolute paths). The
                normal Path __init__ rules for '..' and '.' apply.

        Returns:
            The new Path.
        """
        if not pieces:
            return self
        for i, p in enumerate(pieces):
            if isinstance(p, Path):
                return p.joinpath(*pieces[i + 1:])
        return Path(
            self.base,
            # Propagate None so an accidental join with None raises in
            # Path.__init__'s type check, rather than being silently dropped.
            *[
                p for p in itertools.chain(self.pieces, pieces)
                if p or p is None
            ])

    def __str__(self) -> str:
        if self._str is None:
            if not self._OS_SEP:
                raise ValueError(
                    'Unable to render Path to string - the path module has '
                    'not been initialised yet.')
            str_val = self._OS_SEP.join(
                itertools.chain((str(self.base), ), self.pieces))
            object.__setattr__(self, '_str', str_val)
            return str_val
        return self._str

    def __repr__(self) -> str:
        s = 'Path(%r' % (self.base, )
        if self.pieces:
            s += ', %s' % ', '.join(repr(x) for x in self.pieces)
        return s + ')'

    def __hash__(self) -> int:
        return hash(('config_types.Path', self.base, self.pieces))

    def relative_to(self, other: Path, *, walk_up: bool = False) -> str:
        """Gives one path relative to another, as a plain (always '/'-joined,
        regardless of `_OS_SEP`) string.

        Examples:
            '[CACHE]/foo/bar'.relative_to('[CACHE]/foo') -> 'bar'
            '[CACHE]/foo/bar/baz'.relative_to('[CACHE]/foo') -> 'bar/baz'
            '[CACHE]/foo'.relative_to('[CACHE]/bar') -> ValueError
            '[CACHE]/foo'.relative_to('[CACHE]/bar', walk_up=True)
                -> '../foo'
            '[CACHE]/foo'.relative_to('[CLEANUP]/bar') -> ValueError

        Assumes other is a directory.

        Args:
            other: Path to give self relative to.
            walk_up: Allow '..' in the return value.
        """
        if self.base != other.base:
            raise RelativeToDifferentBases(
                f'{self!r} and {other!r} have different bases')

        if not walk_up and other not in self.parents:
            raise RelativeToNotParent(
                f'{other!r} not in parents of {self!r} and walk_up=False')

        result = []

        for parent in itertools.chain([self], self.parents):
            if parent != other and parent not in other.parents:
                result.append(parent.name)

        for parent in itertools.chain([other], other.parents):
            if parent == self or parent in self.parents:
                break
            result.append('..')

        return '/'.join(reversed(result))
