# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The core `path` module API."""

from __future__ import annotations

from collections.abc import Callable
import ntpath
import os
from pathlib import Path, PurePosixPath
import posixpath
import tempfile

import config_types
from recipe_api import RecipeApi


class _RealFs:  # pragma: no cover - production filesystem backend.
    """Production backend: the real filesystem."""

    def exists(self, path: str | Path) -> bool:
        return Path(path).exists()

    def is_dir(self, path: str | Path) -> bool:
        return Path(path).is_dir()

    def is_file(self, path: str | Path) -> bool:
        return Path(path).is_file()

    def mkdir(self, path: str | Path, *, parents: bool,
              exist_ok: bool) -> None:
        Path(path).mkdir(parents=parents, exist_ok=exist_ok)

    def mkdtemp(self, parent: Path, prefix: str) -> Path:
        parent.mkdir(parents=True, exist_ok=True)
        return Path(tempfile.mkdtemp(prefix=f'{prefix}_', dir=parent))

    def mkstemp(self, parent: Path, prefix: str) -> Path:
        parent.mkdir(parents=True, exist_ok=True)
        fd, name = tempfile.mkstemp(prefix=f'{prefix}_', dir=parent)
        os.close(fd)
        return Path(name)

    def abs(self, path: str | Path) -> Path:
        return Path(path).expanduser().resolve()

    def home(self) -> Path:
        return Path.home()

    def relpath(self, path: str | Path, start: str | Path) -> str:
        return os.path.relpath(str(path), str(start))

    def dirname(self, path: str | Path) -> str | Path:
        return path.parent if isinstance(path, Path) else os.path.dirname(path)

    def basename(self, path: str | Path) -> str:
        return os.path.basename(str(path))

    def splitext(self, path: str | Path) -> tuple[str | Path, str]:
        if isinstance(path, Path):
            return path.parent / path.stem, path.suffix
        return os.path.splitext(path)

    def join(self, path: str | Path, *paths: str | Path) -> str:
        return os.path.join(str(path), *[str(p) for p in paths])

    def normpath(self, path: str | Path) -> str:
        return os.path.normpath(str(path))

    def abspath(self, path: str | Path) -> str:
        # Not bare os.path.abspath: that skips `~`-expansion, which `abs()`
        # (and _SimFs.abspath below) does -- routing both through `abs()`
        # keeps the two backends' contracts identical.
        return str(self.abs(path))

    def realpath(self, path: str | Path) -> str:
        return str(self.abs(path))

    def is_absolute(self, path: str | Path) -> bool:
        return Path(path).is_absolute()

    @property
    def sep(self) -> str:
        return os.sep

    @property
    def pathsep(self) -> str:
        return os.pathsep



class _SimFs:
    """Test backend: the simulated filesystem on the run's TestContext.

    `abs` normalizes lexically (expanding `~` to the simulated home) rather than
    resolving against the real cwd/filesystem, so paths stay deterministic.
    """

    def __init__(self, test) -> None:
        self._test = test
        # `ntpath`/`posixpath` are pure lexical stdlib modules -- no real
        # filesystem or host-OS interaction -- so either is safely usable
        # regardless of the real host, unlike `os.path` (which is always
        # whichever of the two the real host happens to be).
        self._path_mod = ntpath if test.platform == 'win' else posixpath

    def exists(self, path: str | Path | config_types.Path) -> bool:
        return self._test.fs.exists(path)

    def is_dir(self, path: str | Path | config_types.Path) -> bool:
        return self._test.fs.is_dir(path)

    def is_file(self, path: str | Path | config_types.Path) -> bool:
        return self._test.fs.is_file(path)

    def mkdir(self, path: str | Path | config_types.Path, *, parents: bool,
              exist_ok: bool) -> None:
        # The simulated fs has no real directory tree: `add_dir` is idempotent
        # and implies parents, so these flags don't apply here.
        del parents, exist_ok
        self._test.fs.add_dir(path)

    def mkdtemp(self, parent: config_types.Path,
                prefix: str) -> config_types.Path:
        # Numbered per prefix rather than randomized, so a run that makes
        # temporary directories still has reproducible expectations.
        self._test.temp_counter[prefix] += 1
        path = parent / f'{prefix}_tmp_{self._test.temp_counter[prefix]}'
        self._test.fs.add_dir(path)
        return path

    def mkstemp(self, parent: config_types.Path,
                prefix: str) -> config_types.Path:
        # Shares `mkdtemp`'s counter (keyed by prefix, not by file-vs-dir), so
        # a temp file and a temp dir with the same prefix never collide.
        self._test.temp_counter[prefix] += 1
        path = parent / f'{prefix}_tmp_{self._test.temp_counter[prefix]}'
        self._test.fs.add_file(path)
        return path

    def abs(self, path: str | Path | config_types.Path) -> config_types.Path:
        if isinstance(path, config_types.Path):
            return path
        text = str(path)
        if text == '~':
            return self.home()
        if text.startswith('~/'):
            return self.home().joinpath(text[2:])
        # No named base to resolve a bare string against (e.g. a test-seeded
        # `env.which()` result): split it into an empty base plus one piece
        # per path segment, rather than one opaque unsplit base, so `.parent`/
        # `.name`/further joins on the result behave like any other Path --
        # only the anchor ('/') is dropped, since it isn't a real segment.
        # Lexically posix-only, never the real host's os.path.
        pure = PurePosixPath(text)
        pieces = [p for p in pure.parts if p != pure.anchor]
        return config_types.Path(config_types.ResolvedBasePath(''), *pieces)

    def home(self) -> config_types.Path:
        return config_types.Path(config_types.ResolvedBasePath(
            self._test.home))

    def relpath(self, path: str | Path | config_types.Path,
                start: str | Path | config_types.Path) -> str:
        if isinstance(path, config_types.Path) and isinstance(
                start, config_types.Path):
            return path.relative_to(start, walk_up=True)
        return posixpath.relpath(str(path), str(start))

    def dirname(
        self, path: str | Path | config_types.Path
    ) -> str | Path | config_types.Path:
        if isinstance(path, config_types.Path):
            return path.parent
        return self._path_mod.dirname(str(path))

    def basename(self, path: str | Path | config_types.Path) -> str:
        return self._path_mod.basename(str(path))

    def splitext(
        self, path: str | Path | config_types.Path
    ) -> tuple[str | Path | config_types.Path, str]:
        if isinstance(path, config_types.Path):
            return path.parent.joinpath(path.stem), path.suffix
        return self._path_mod.splitext(str(path))

    def join(self, path: str | Path | config_types.Path, *paths:
             str | Path | config_types.Path) -> str:
        return self._path_mod.join(str(path), *[str(p) for p in paths])

    def normpath(self, path: str | Path | config_types.Path) -> str:
        # Purely lexical (unlike abspath/realpath below): normpath never
        # consults the real cwd, so delegating straight to the simulated
        # platform's path module is safe.
        return self._path_mod.normpath(str(path))

    def abspath(self, path: str | Path | config_types.Path) -> str:
        # NOT self._path_mod.abspath: that falls back to the real host's
        # os.getcwd() for a non-absolute input, which would silently depend
        # on the real machine running the test. `abs()` already handles this
        # deterministically (never touching the real cwd/filesystem).
        return str(self.abs(path))

    def realpath(self, path: str | Path | config_types.Path) -> str:
        # Same reasoning as abspath: no real filesystem resolution here.
        return str(self.abs(path))

    def is_absolute(self, path: str | Path | config_types.Path) -> bool:
        # Any config_types.Path is inherently absolute by construction (it's
        # always rooted in a resolved base). Purely lexical either way --
        # unlike abspath/realpath, isabs never needs to resolve anything, so
        # there's no cwd/symlink pitfall to avoid here.
        if isinstance(path, config_types.Path):
            return True
        return self._path_mod.isabs(str(path))

    @property
    def sep(self) -> str:
        # The single source of truth for the simulated separator, so this
        # always agrees with how a `config_types.Path` renders.
        return config_types.Path._OS_SEP

    @property
    def pathsep(self) -> str:
        return ';' if self.sep == '\\' else ':'


class PathApi(RecipeApi):
    """Named, workspace-relative job paths, seeded by the engine.

    Recipes build paths from these named roots instead of hardcoding them or
    taking them as properties. Only the workspace root is configurable (engine
    `--workspace`); everything else is derived from it, so the on-disk layout is
    fixed and consistent across recipes.
    """

    @property
    def workspace(self) -> str | Path | config_types.Path:
        """Root directory the job runs in (engine-provided in production;
        a `[WORKSPACE]`-rooted `config_types.Path` token in test mode)."""
        if self._test is not None:
            return config_types.Path(
                config_types.ResolvedBasePath(self._workspace_token))
        return self._workspace  # pragma: no cover - production only

    @property
    def chromium_src(self) -> str | Path | config_types.Path:
        """Chromium `src/` checkout: `<workspace>/b/src`.

        Named `b` to keep paths short on Windows, as long paths on Windows can
        cause linking errors for code we have no control over (e.g. LLVM).
        """
        return self.workspace / 'b' / 'src'

    @property
    def brave_core(self) -> str | Path | config_types.Path:
        """brave-core checkout inside Chromium: `<chromium_src>/brave`."""
        return self.chromium_src / 'brave'

    @property
    def out(self) -> str | Path | config_types.Path:
        """Build output directory: `<workspace>/out`."""
        return self.workspace / 'out'

    @property
    def cleanup_dir(self) -> str | Path | config_types.Path:
        """Scratch directory: `<workspace>/rc`.

        Everything under here is disposable -- treat it as empty at the start of
        a job and gone afterwards. This is where `mkdtemp` puts the directories
        it makes, so a step that needs somewhere to write throwaway output has a
        home for it that nothing else has to clean up.
        """
        return self.workspace / 'rc'

    # Filesystem seams. The real-vs-simulated choice is made once in
    # `initialise()` by picking a backend. The methods below just delegate, so
    # in test mode recipes probe (and `mkdir` mutates) the simulated filesystem
    # without any real disk access, and with no per-call test-mode branching.

    def __init__(self) -> None:
        super().__init__()
        # Default to the real filesystem; swapped for a simulated backend in
        # test mode by initialise() (once the engine has seeded `_test`).
        self._fs = _RealFs()
        # Set for real by initialise() in test mode; the `workspace` property
        # only reads it there, so the production-only `None` is never used.
        self._workspace_token: str | None = None

    def initialise(self) -> None:
        if self._test is not None:
            # Local import: `simulation` (and the `gevent` it pulls in) is
            # test-only, so keep it off the production import path (matches
            # `step/api.py`'s `_prod_runner_lazy` convention).
            import simulation
            self._workspace_token = simulation.WORKSPACE_TOKEN
            self._fs = _SimFs(self._test)
            # Simulated separator, driven by the platform under test -- never
            # the real host's `sys.platform`/`os.sep`.
            config_types.Path._OS_SEP = ('\\' if self._test.platform == 'win'
                                         else '/')
        else:
            # Every other named path here bypasses `config_types.Path`
            # entirely in production (see `workspace` below), but
            # `RecipeApi.resource()` doesn't, so this still needs to be set.
            config_types.Path._OS_SEP = os.sep  # pragma: no cover - production only

    def exists(self, path: str | Path | config_types.Path) -> bool:
        """Whether *path* exists (a file or a directory)."""
        return self._fs.exists(path)

    def is_dir(self, path: str | Path | config_types.Path) -> bool:
        """Whether *path* exists and is a directory."""
        return self._fs.is_dir(path)

    def is_file(self, path: str | Path | config_types.Path) -> bool:
        """Whether *path* exists and is a regular file."""
        return self._fs.is_file(path)

    def mkdir(self,
              path: str | Path | config_types.Path,
              *,
              parents: bool = True,
              exist_ok: bool = True) -> None:
        """Create directory *path* (creating parents by default)."""
        self._fs.mkdir(path, parents=parents, exist_ok=exist_ok)

    def mkdtemp(self, prefix: str = 'tmp') -> str | Path | config_types.Path:
        """Create a new temporary directory under `cleanup_dir`, and return it.

        Args:
            prefix: Prepended to the directory's name, to say what it is for.

        In test mode nothing is created on disk and the name is numbered per
        *prefix* (`<cleanup_dir>/<prefix>_tmp_1`, `_2`, ...) rather than
        randomized, so expectations stay stable across runs.
        """
        return self._fs.mkdtemp(self.cleanup_dir, prefix)

    def mkstemp(self, prefix: str = 'tmp') -> str | Path | config_types.Path:
        """Create a new temporary file under `cleanup_dir`, and return it.

        Args:
            prefix: Prepended to the file's name, to say what it is for.

        Unlike `tempfile.mkstemp`, the file's file descriptor is closed. If you
        need the full security properties of `mkstemp`, outsource this to a
        resource script instead.

        In test mode nothing is created on disk (see `mkdtemp`).
        """
        return self._fs.mkstemp(self.cleanup_dir, prefix)

    def abs(
        self, path: str | Path | config_types.Path
    ) -> str | Path | config_types.Path:
        """Return an absolute, `~`-expanded, normalized `Path`.

        Replaces `Path(p).expanduser().resolve()`. In test mode `~` expands to
        the simulated home and the path is normalized lexically, never
        resolved against the real cwd or filesystem (so it stays deterministic).
        """
        return self._fs.abs(path)

    def home(self) -> str | Path | config_types.Path:
        """The user's home directory (simulated in test mode)."""
        return self._fs.home()

    def relpath(self, path: str | Path | config_types.Path,
                start: str | Path | config_types.Path) -> str:
        """*path* relative to *start*. Roughly equivalent to `os.path.relpath`,
        except *start* is required (there is no implicit real cwd here)."""
        return self._fs.relpath(path, start)

    def dirname(
        self, path: str | Path | config_types.Path
    ) -> str | Path | config_types.Path:
        """For `foo/bar/baz`, return `foo/bar`. Equivalent to
        `os.path.dirname`, except a `Path` argument returns a `Path`
        (`.parent`), matching the type of the input."""
        return self._fs.dirname(path)

    def basename(self, path: str | Path | config_types.Path) -> str:
        """For `foo/bar/baz`, return `baz`. Equivalent to `os.path.basename`
        -- always a `str`, regardless of *path*'s type."""
        return self._fs.basename(path)

    def split(
        self, path: str | Path | config_types.Path
    ) -> tuple[str | Path | config_types.Path, str]:
        """`(dirname(path), basename(path))`. Equivalent to `os.path.split`."""
        return self.dirname(path), self.basename(path)

    def splitext(
        self, path: str | Path | config_types.Path
    ) -> tuple[str | Path | config_types.Path, str]:
        """For `foo/bar.baz`, return `(foo/bar, '.baz')`. Equivalent to
        `os.path.splitext`; the first item matches the type of *path*."""
        return self._fs.splitext(path)

    def join(self, path: str | Path | config_types.Path, *paths:
             str | Path | config_types.Path) -> str:
        """Equivalent to `os.path.join`. Always returns a `str` -- a `Path`
        already has `joinpath`/`/` for joining and returning a `Path`."""
        return self._fs.join(path, *paths)

    def normpath(self, path: str | Path | config_types.Path) -> str:
        """Equivalent to `os.path.normpath`."""
        return self._fs.normpath(path)

    def abspath(self, path: str | Path | config_types.Path) -> str:
        """An absolute, `~`-expanded, fully resolved path, as a `str`.

        Unlike `os.path.abspath`, this also expands `~` -- it's `str(abs(p))`
        in both production and test mode, so the two backends agree exactly.
        Never resolves against a real cwd in test mode (see `abs`).
        """
        return self._fs.abspath(path)

    def realpath(self, path: str | Path | config_types.Path) -> str:
        """An absolute, `~`-expanded, fully resolved path, as a `str`.

        Same as `abspath` -- with `abs()` already fully resolving (including
        symlinks in production), there's no further distinction to draw
        between "absolute" and "real" here.
        """
        return self._fs.realpath(path)

    def is_absolute(self, path: str | Path | config_types.Path) -> bool:
        """Whether *path* is already absolute. Purely lexical (unlike
        `abspath`/`realpath`, never resolves anything)."""
        return self._fs.is_absolute(path)

    def expandvars(self, path: str) -> str:
        """Mostly equivalent to `os.path.expandvars`, with some limitations.

        Limited to variables set in the `context` module's `env`. Variables
        must be of the form `${VARNAME}`, not just `$VARNAME`.
        """
        for key, value in self.m.context.env.items():
            if value is not None:
                path = path.replace(f'${{{key}}}', value)
        return path

    def assert_absolute(self, path: str | Path | config_types.Path) -> None:
        """Raises `AssertionError` unless *path* is already absolute."""
        if not self.is_absolute(path):
            raise AssertionError(f'{path!r} is not absolute')

    def mock_add_file(self, path: str | Path | config_types.Path) -> None:
        """For testing purposes, mark that file *path* exists. No-op in
        production."""
        if self._test is not None:
            self._test.fs.add_file(path)

    def mock_add_directory(self, path: str | Path | config_types.Path) -> None:
        """For testing purposes, mark that directory *path* exists. No-op in
        production."""
        if self._test is not None:
            self._test.fs.add_dir(path)

    def mock_copy_paths(self, source: str | Path | config_types.Path,
                        dest: str | Path | config_types.Path) -> None:
        """For testing purposes, copy *source* (and everything nested under
        it) to *dest*. No-op in production."""
        if self._test is not None:
            self._test.fs.copy(source, dest)

    def mock_remove_paths(
            self,
            path: str | Path | config_types.Path,
            should_remove: Callable[[str], bool] = lambda p: True) -> None:
        """For testing purposes, mark that *path* (and everything nested
        under it for which *should_remove* returns True) no longer exists.
        No-op in production.
        """
        if self._test is not None:
            self._test.fs.remove(path, should_remove)

    @property
    def sep(self) -> str:
        """Equivalent to `os.sep` (the simulated platform's, in test mode)."""
        return self._fs.sep

    @property
    def pathsep(self) -> str:
        """Equivalent to `os.pathsep` (the simulated platform's, in test
        mode)."""
        return self._fs.pathsep
