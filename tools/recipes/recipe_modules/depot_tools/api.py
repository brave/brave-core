# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `depot_tools` module API."""

from __future__ import annotations

import logging
from pathlib import Path

from recipe_api import RecipeApi

# the urls we clone from
DEPOT_TOOLS_URL = 'https://chromium.googlesource.com/chromium/tools/depot_tools'

# the relative path for depot_tools in the chromium checkout.
DEPOT_TOOLS_PATH = Path('third_party') / 'depot_tools'


class DepotToolsApi(RecipeApi):
    """Deploys depot_tools so `gclient`/`fetch` are available on PATH."""

    def __init__(self) -> None:
        super().__init__()
        # Resolved path to depot_tools, or None if no `depot_tools` has been
        # deployed yet.
        self._depot_tools_path: Path | None = None
        # vpython3 entry point; resolved from the platform in initialise().
        self._vpython3 = 'vpython3'

    def initialise(self) -> None:
        # `.bat` on Windows; resolved via the platform seam so a test can
        # simulate either host (and so this isn't fixed at import time).
        self._vpython3 = ('vpython3.bat'
                          if self.m.platform.is_win else 'vpython3')

    def ensure_on_path(self) -> None:
        """Deploy depot_tools and put it on PATH. Successive calls are no-ops.
        """
        if self._depot_tools_path is not None:
            return  # Already deployed this run.

        resolved = self.m.env.which('gclient')
        if resolved is not None:
            logging.debug('depot_tools already on PATH, skipping clone')
            # Using whatever depot_tools is already on PATH.
            self._depot_tools_path = Path(resolved).parent
            return

        # Checking for a standalone depot_tools under what would be a supposed
        # Chromium checkout.
        depot_tools_path = self.m.path.abs(self.m.path.chromium_src.parent /
                                           DEPOT_TOOLS_PATH)
        if self.m.path.is_file(depot_tools_path / 'gclient'):
            # If Chromium has already been deployed, we just use whatever
            # is in place.
            logging.info('depot_tools already present at %s, adding to PATH.',
                         depot_tools_path)
        else:
            logging.info('Installing depot_tools under %s', depot_tools_path)
            self.m.path.mkdir(depot_tools_path.parent)
            self.m.step('clone depot_tools', [
                'git', 'clone', '--depth', '1', DEPOT_TOOLS_URL,
                str(depot_tools_path)
            ])

        self.m.env.prepend_path(depot_tools_path)
        # Run once so depot_tools bootstraps itself (downloads its own deps).
        self.m.step('verify gclient', ['gclient'])
        self._depot_tools_path = depot_tools_path

    def vpython3(self) -> Path:
        """Return depot_tools' `vpython3`.

        This function ensures that depot_tools is deployed and on PATH, if that
        hasn't already been done.

        Returns:
            The absolute `Path` to depot_tools' `vpython3` entry point.
        """
        self.ensure_on_path()
        assert self._depot_tools_path is not None
        return self._depot_tools_path / self._vpython3
