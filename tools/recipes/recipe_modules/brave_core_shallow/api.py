# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `brave_core_shallow` module API."""

from __future__ import annotations

from collections.abc import Iterable
import logging
from pathlib import Path

from recipe_api import RecipeApi

# Default SSH remote for the brave-core repository.
REPO_URL = 'git@github.com:brave/brave-core.git'


class BraveCoreShallowApi(RecipeApi):
    """Deploys subpaths of brave-core via a shallow, sparse checkout.

    Instead of cloning the whole (large) repository, this fetches just enough
    history and only the requested subtrees, so a recipe can use a handful of
    paths (scripts, configs, ...) without paying for a full checkout. The flow
    matches a manual:

        git clone --depth 2 --filter=blob:none --sparse <url> <dest>
        git -C <dest> sparse-checkout set <path>...

    `--filter=blob:none` defers blob downloads until checkout, `--sparse` starts
    the working tree with only top-level files (cone mode), and
    `sparse-checkout set` then materialises exactly the requested directories.
    """

    def deploy(self,
               paths: str | Path | Iterable[str | Path],
               *,
               dest: str | Path | None = None,
               url: str = REPO_URL,
               ref: str | None = None,
               depth: int = 2) -> Path:
        """Ensure *paths* from brave-core are checked out under *dest*.

        Clones brave-core into *dest* (shallow + sparse) when it is not already
        a checkout, then restricts the working tree to *paths*. Re-running is
        cheap: an existing checkout is reused, fetched to *ref*, and only the
        sparse set is re-applied -- so a checkout left on a different ref by a
        prior run is brought to the requested one rather than trusted as-is.

        Every requested path keeps its brave-core-relative layout beneath the
        returned root (a path `tools/a` lands at `<root>/tools/a`), so scripts
        deployed in one path can reference sibling paths exactly as they do in a
        full checkout, with no rewriting. Paths accumulate: each call extends
        the working tree via `sparse-checkout add`, so subtrees checked out by
        an earlier call -- or by an external bootstrap sharing this same
        checkout -- coexist rather than being replaced.

        Args:
            paths: A single repo-relative path, or an iterable of them, to
                materialise (e.g. `'tools/cr/toolchains'`).
            dest: Directory the repo lives in (created if missing); the
                brave-core root the returned tree is anchored at. Defaults to
                the `path` module's `brave_core`, the standard job layout.
            url: Git remote to clone from; defaults to brave-core over SSH.
            ref: Branch or tag to check out. Defaults to the engine-provided
                brave-core ref (`master`, or the `--brave-core-ref` override).
                Applied whether the checkout is freshly cloned or reused (an
                existing checkout is fetched and hard-checked-out to it). A
                shallow clone cannot target a bare commit SHA.
            depth: History depth for the shallow clone/fetch.

        Returns:
            The absolute `Path` to the brave-core checkout root. Requested paths
            live beneath it at their original repo-relative locations.

        Raises:
            ValueError: If *paths* is empty.
            RuntimeError: If a requested path is absent after the checkout
                (e.g. a typo or a path that does not exist on *ref*).
        """
        if dest is None:
            dest = self.m.path.brave_core
        single = isinstance(paths, (str, Path))
        rel_paths = [str(paths)] if single else [str(p) for p in paths]
        if not rel_paths:
            raise ValueError('deploy() requires at least one path')

        # Fall back to the engine-provided ref when the caller doesn't specify.
        ref = ref if ref is not None else self._brave_core_ref

        dest = Path(dest).expanduser().resolve()

        if (dest / '.git').is_dir():
            # Reuse the existing checkout, but bring it to *ref* -- it may have
            # been cloned (here or by a prior run) at a different ref that lacks
            # the requested paths, so fetch and hard-checkout rather than trust
            # its current state.
            logging.info('brave-core checkout present at %s; updating to %s',
                         dest, ref)
            self.m.step('fetch brave-core ref', [
                'git', '-C',
                str(dest), 'fetch', '--depth',
                str(depth), 'origin', ref
            ])
            self.m.step(
                'checkout brave-core ref',
                ['git', '-C',
                 str(dest), 'checkout', '--force', 'FETCH_HEAD'])
        else:
            dest.parent.mkdir(parents=True, exist_ok=True)
            clone_cmd = [
                'git', 'clone', '--depth',
                str(depth), '--filter=blob:none', '--sparse', '--branch', ref,
                url,
                str(dest)
            ]
            self.m.step('clone brave-core (shallow, sparse)', clone_cmd)

        # Extend the sparse working tree with the requested subtrees. `add`
        # (rather than `set`) accumulates, so subtrees checked out by a prior
        # call or an external bootstrap into this same checkout survive.
        self.m.step(
            'sparse-checkout add',
            ['git', '-C',
             str(dest), 'sparse-checkout', 'add', *rel_paths])

        for rel in rel_paths:
            if not (dest / rel).exists():
                raise RuntimeError(
                    f'brave-core path not found after sparse checkout: {rel!r} '
                    f'(looked under {dest})')

        return dest
