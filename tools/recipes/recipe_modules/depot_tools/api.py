# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `depot_tools` module API."""

from __future__ import annotations

import logging
import os
from pathlib import Path
import shutil
import sys

from recipe_api import RecipeApi

# the urls we clone from
DEPOT_TOOLS_URL = 'https://chromium.googlesource.com/chromium/tools/depot_tools'

# vpython3 entry point.
VPYTHON3 = 'vpython3.bat' if sys.platform == 'win32' else 'vpython3'

# the relative path for depot_tools in the chromium checkout.
DEPOT_TOOLS_PATH = Path('third_party') / 'depot_tools'


class DepotToolsApi(RecipeApi):
    """Deploys depot_tools so `gclient`/`fetch` are available on PATH.

    Extracted from `build_rust_toolchain.py::_bootstrap_depot_tools`.
    """

    def __init__(self) -> None:
        super().__init__()
        # Resolved path to depot_tools, or None if no `depot_tools` has been
        # deployed yet.
        self._depot_tools_path: Path | None = None

    def ensure_on_path(self) -> None:
        """Deploy depot_tools and put it on PATH. Successive calls are no-ops.
        """
        if self._depot_tools_path is not None:
            return  # Already deployed this run.

        if shutil.which('gclient') is not None:
            logging.debug('depot_tools already on PATH, skipping clone')
            # Using whatever depot_tools is already on PATH.
            self._depot_tools_path = Path(shutil.which('gclient')).parent
            return

        # Checking for a standalone depot_tools under what would be a supposed
        # Chromium checkout.
        depot_tools_path = (self.m.path.chromium_src.parent /
                            DEPOT_TOOLS_PATH).resolve()
        if (depot_tools_path / 'gclient').is_file():
            # If Chromium has already been deployed, we just use whatever
            # is in place.
            logging.info('depot_tools already present at %s, adding to PATH.',
                         depot_tools_path)
        else:
            logging.info('Installing depot_tools under %s', depot_tools_path)
            depot_tools_path.parent.mkdir(parents=True, exist_ok=True)
            self.m.step('clone depot_tools', [
                'git', 'clone', '--depth', '1', DEPOT_TOOLS_URL,
                str(depot_tools_path)
            ])

        os.environ['PATH'] = os.pathsep.join(
            [str(depot_tools_path), os.environ['PATH']])
        # Run once so depot_tools bootstraps itself (downloads its own deps).
        self.m.step('verify gclient', ['gclient'])
        self._depot_tools_path = depot_tools_path

    def path(self) -> Path:
        """Return the deployed depot_tools directory.

        This function ensures that depot_tools is deployed and on PATH, if that
        hasn't already been done.

        Returns:
            The absolute `Path` to the depot_tools checkout.
        """
        self.ensure_on_path()
        assert self._depot_tools_path is not None
        return self._depot_tools_path

    def vpython3(self) -> Path:
        """Return depot_tools' `vpython3`.

        This function ensures that depot_tools is deployed and on PATH, if that
        hasn't already been done.

        Returns:
            The absolute `Path` to depot_tools' `vpython3` entry point.
        """
        return self.path() / VPYTHON3
