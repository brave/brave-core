# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `cipd` module API."""

from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING

from recipe_api import RecipeApi

if TYPE_CHECKING:
    from pathlib import Path

    import config_types
    from recipe_modules import cipd


class CipdApi(RecipeApi):
    """Installs CIPD packages at pinned versions, with depot_tools' `cipd`."""

    m: cipd.DEPS

    def __init__(self) -> None:
        super().__init__()
        # Each tool `ensure_tool()` has installed, by package and version.
        self._tools: dict[tuple[str, str, str], Path | config_types.Path] = {}

    def ensure(
        self,
        step_name: str,
        root: str | Path | config_types.Path,
        packages: Mapping[str, str],
    ) -> None:
        """Installs *packages* into *root*, and removes anything else there.

        Args:
            step_name: The step's name.
            root: The directory the packages go into.
            packages: Each package's version, by its name. Names may hold
                `${platform}`, which `cipd` fills in for the host.
        """
        ensure_file = ''.join(
            f'{package} {version}\n' for package, version in packages.items()
        )
        self.m.step(
            step_name,
            [
                self.m.depot_tools.root
                / ('cipd.bat' if self.m.platform.is_win else 'cipd'),
                'ensure',
                '-root',
                root,
                '-ensure-file',
                self.m.raw_io.input_text(ensure_file),
            ],
        )

    def ensure_tool(
        self, package: str, version: str, executable: str
    ) -> Path | config_types.Path:
        """Installs *package* at *version* once per run, under the workspace.

        Args:
            package: The package's name, e.g.
                `infra/rbe/client/${platform}`.
            version: Its version, as a tag, ref or instance ID.
            executable: The tool's path inside the package, without `.exe`.

        Returns:
            The tool's path.
        """
        key = (package, version, executable)
        if key not in self._tools:
            root = (
                self.m.path.workspace
                / 'cipd'
                / package.replace('/${platform}', '').replace('/', '_')
                / version.replace(':', '_')
            )
            self.ensure(f'install {package}', root, {package: version})
            if self.m.platform.is_win:
                executable += '.exe'
            self._tools[key] = root / executable
        return self._tools[key]
