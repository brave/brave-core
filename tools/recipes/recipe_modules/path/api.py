# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The core `path` module API."""

from __future__ import annotations

import os
from pathlib import Path
import tempfile

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

    def abs(self, path: str | Path) -> Path:
        return Path(path).expanduser().resolve()

    def home(self) -> Path:
        return Path.home()


class _SimFs:
    """Test backend: the simulated filesystem on the run's TestContext.

    `abs` normalizes lexically (expanding `~` to the simulated home) rather than
    resolving against the real cwd/filesystem, so paths stay deterministic.
    """

    def __init__(self, test) -> None:
        self._test = test

    def exists(self, path: str | Path) -> bool:
        return self._test.fs.exists(path)

    def is_dir(self, path: str | Path) -> bool:
        return self._test.fs.is_dir(path)

    def is_file(self, path: str | Path) -> bool:
        return self._test.fs.is_file(path)

    def mkdir(self, path: str | Path, *, parents: bool,
              exist_ok: bool) -> None:
        # The simulated fs has no real directory tree: `add_dir` is idempotent
        # and implies parents, so these flags don't apply here.
        del parents, exist_ok
        self._test.fs.add_dir(path)

    def mkdtemp(self, parent: Path, prefix: str) -> Path:
        # Numbered per prefix rather than randomized, so a run that makes
        # temporary directories still has reproducible expectations.
        self._test.temp_counter[prefix] += 1
        path = parent / f'{prefix}_tmp_{self._test.temp_counter[prefix]}'
        self._test.fs.add_dir(path)
        return path

    def abs(self, path: str | Path) -> Path:
        text = str(path)
        if text.startswith('~'):
            text = str(self._test.home) + text[1:]
        return Path(os.path.normpath(text))

    def home(self) -> Path:
        return self._test.home


class PathApi(RecipeApi):
    """Named, workspace-relative job paths, seeded by the engine.

    Recipes build paths from these named roots instead of hardcoding them or
    taking them as properties. Only the workspace root is configurable (engine
    `--workspace`); everything else is derived from it, so the on-disk layout is
    fixed and consistent across recipes.
    """

    @property
    def workspace(self) -> Path:
        """Root directory the job runs in (engine-provided)."""
        return self._workspace

    @property
    def chromium_src(self) -> Path:
        """Chromium `src/` checkout: `<workspace>/b/src`.

        Named `b` to keep paths short on Windows, as long paths on Windows can
        cause linking errors for code we have no control over (e.g. LLVM).
        """
        return self._workspace / 'b' / 'src'

    @property
    def brave_core(self) -> Path:
        """brave-core checkout inside Chromium: `<chromium_src>/brave`."""
        return self.chromium_src / 'brave'

    @property
    def out(self) -> Path:
        """Build output directory: `<workspace>/out`."""
        return self._workspace / 'out'

    @property
    def cleanup_dir(self) -> Path:
        """Scratch directory: `<workspace>/rc`.

        Everything under here is disposable -- treat it as empty at the start of
        a job and gone afterwards. This is where `mkdtemp` puts the directories
        it makes, so a step that needs somewhere to write throwaway output has a
        home for it that nothing else has to clean up.
        """
        return self._workspace / 'rc'

    # Filesystem seams. The real-vs-simulated choice is made once in
    # `initialise()` by picking a backend. The methods below just delegate, so
    # in test mode recipes probe (and `mkdir` mutates) the simulated filesystem
    # without any real disk access, and with no per-call test-mode branching.

    def __init__(self) -> None:
        super().__init__()
        # Default to the real filesystem; swapped for a simulated backend in
        # test mode by initialise() (once the engine has seeded `_test`).
        self._fs = _RealFs()

    def initialise(self) -> None:
        if self._test is not None:
            self._fs = _SimFs(self._test)

    def exists(self, path: str | Path) -> bool:
        """Whether *path* exists (a file or a directory)."""
        return self._fs.exists(path)

    def is_dir(self, path: str | Path) -> bool:
        """Whether *path* exists and is a directory."""
        return self._fs.is_dir(path)

    def is_file(self, path: str | Path) -> bool:
        """Whether *path* exists and is a regular file."""
        return self._fs.is_file(path)

    def mkdir(self,
              path: str | Path,
              *,
              parents: bool = True,
              exist_ok: bool = True) -> None:
        """Create directory *path* (creating parents by default)."""
        self._fs.mkdir(path, parents=parents, exist_ok=exist_ok)

    def mkdtemp(self, prefix: str = 'tmp') -> Path:
        """Create a new temporary directory under `cleanup_dir`, and return it.

        Args:
            prefix: Prepended to the directory's name, to say what it is for.

        In test mode nothing is created on disk and the name is numbered per
        *prefix* (`<cleanup_dir>/<prefix>_tmp_1`, `_2`, ...) rather than
        randomized, so expectations stay stable across runs.
        """
        return self._fs.mkdtemp(self.cleanup_dir, prefix)

    def abs(self, path: str | Path) -> Path:
        """Return an absolute, `~`-expanded, normalized `Path`.

        Replaces `Path(p).expanduser().resolve()`. In test mode `~` expands to
        the simulated home and the path is normalized lexically, never
        resolved against the real cwd or filesystem (so it stays deterministic).
        """
        return self._fs.abs(path)

    def home(self) -> Path:
        """The user's home directory (simulated in test mode)."""
        return self._fs.home()
