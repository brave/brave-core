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

# Latest Chromium depot_tools bundle.
DEPOT_TOOLS_URL = 'https://chromium.googlesource.com/chromium/tools/depot_tools'

# vpython3 selected by depot_tools from `$PATH`, relative to the Chromium src/
# root. On Windows the `.bat` shim is used (needed when calling from git bash).
VPYTHON_PATH = Path('third_party/depot_tools') / (
    'vpython3.bat' if sys.platform == 'win32' else 'vpython3')


class DepotToolsApi(RecipeApi):
    """Deploys depot_tools so `gclient`/`fetch` are available on PATH.

    Extracted from `build_rust_toolchain.py::_bootstrap_depot_tools`.
    """

    def ensure_on_path(self, chromium_src: str | Path) -> None:
        """Put depot_tools on PATH, cloning it beside src/ if necessary.

        If `gclient` already resolves on PATH, this is a no-op. Otherwise it
        reuses an existing checkout at `<chromium_src>/../depot_tools` or clones
        a fresh one there, then prepends it to PATH and verifies `gclient` runs.

        Args:
            chromium_src: Path to the Chromium `src/` directory; depot_tools is
                installed as its sibling.
        """
        chromium_src = Path(chromium_src).expanduser().resolve()

        if shutil.which('gclient') is not None:
            logging.debug('depot_tools already on PATH, skipping clone')
            return

        depot_tools_path = chromium_src.parent / 'depot_tools'
        if (depot_tools_path / 'gclient').is_file():
            # Already present at the expected install path; just add to PATH.
            logging.info('depot_tools already present at %s, adding to PATH.',
                         depot_tools_path)
        else:
            logging.info('Installing depot_tools under %s', depot_tools_path)
            depot_tools_path.parent.mkdir(parents=True, exist_ok=True)
            self.m.step(
                'clone depot_tools',
                ['git', 'clone', DEPOT_TOOLS_URL,
                 str(depot_tools_path)])

        os.environ['PATH'] = os.pathsep.join(
            [str(depot_tools_path), os.environ['PATH']])
        # Run once so depot_tools bootstraps itself (downloads its own deps).
        self.m.step('verify gclient', ['gclient'])

    def vpython3(self, chromium_src: str | Path) -> Path:
        """Return the path to depot_tools' vpython3 inside *chromium_src*."""
        return Path(chromium_src).expanduser().resolve() / VPYTHON_PATH
