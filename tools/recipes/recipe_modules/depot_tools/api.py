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
        # Resolved depot_tools directory, set by ensure_on_path() and memoised
        # so repeat calls skip the deploy. None until the first call deploys it.
        self._depot_tools_path: Path | None = None

    def ensure_on_path(self) -> None:
        """Deploy depot_tools and put it on PATH. Successive calls are no-ops.
        """
        if self._depot_tools_path is not None:
            return  # Already deployed this run.

        if shutil.which('gclient') is not None:
            logging.debug('depot_tools already on PATH, skipping clone')
            # Reuse it as-is: depot_tools is the directory holding gclient.
            self._depot_tools_path = Path(shutil.which('gclient')).parent
            return

        # No checkout to borrow from: install a standalone clone beside src/.
        depot_tools_path = (self.m.path.chromium_src.parent /
                            DEPOT_TOOLS_PATH).resolve()
        if (depot_tools_path / 'gclient').is_file():
            # Already present at the expected install path; just add to PATH.
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

    def vpython3(self) -> Path:
        """Return depot_tools' `vpython3`, deploying depot_tools on first use.
        """
        self.ensure_on_path()
        assert self._depot_tools_path is not None
        return self._depot_tools_path / VPYTHON3
