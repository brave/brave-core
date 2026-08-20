# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `osx_sdk` module API."""

from __future__ import annotations

import contextlib
from collections.abc import Iterator
from typing import Any

from recipe_api import RecipeApi

# Path to `EphemeralXcode`.
EPHEMERAL_XCODE_SCRIPT = 'tools/cr/toolchains/ephemeral_xcode.py'

# Path for Chromium's macOS SDK pin.
MAC_SDK_GNI_PATH = 'build/config/mac/mac_sdk.gni'


class OSXSDKApi(RecipeApi):
    """Installs and selects the exact Xcode that Chromium is using.
    """

    @contextlib.contextmanager
    def ensure(
        self,
        chromium_src: str | Path | None = None
    ) -> Iterator[dict[str, Any] | None]:
        """Install + select the Xcode in `mac_sdk.gni` pins.

        Args:
            chromium_src: Chromium `src/` checkout to read the SDK pin from.
                Defaults to the `path` module's `chromium_src`.

        Yields:
            A dict with `app`, `xcode_version`, `xcode_build`, `sdk_version`,
            `sdk_build_version`.
            `None` on non-mac platforms, where this is a no-op.
        """
        if not self.m.platform.is_mac:
            yield None
            return

        if chromium_src is None:
            chromium_src = self.m.path.chromium_src
        mac_sdk_gni = self.m.path.abs(chromium_src) / MAC_SDK_GNI_PATH

        vpython3 = self.m.depot_tools.vpython3()
        gni_result = self.m.step('read mac_sdk.gni', [
            vpython3, '-u',
            self.resource('read_mac_sdk_gni.py'), mac_sdk_gni, '--json-output',
            self.m.json.output()
        ])
        sdk_version = gni_result.json.output['sdk_version']
        sdk_build_version = gni_result.json.output['sdk_build_version']

        brave_core_root = self.m.brave_core_checkout.deploy(
            'tools/cr/toolchains')
        script = brave_core_root / EPHEMERAL_XCODE_SCRIPT

        try:
            result = self.m.step('install xcode', [
                vpython3, script, '--sdk-version', sdk_version, '--sdk-build',
                sdk_build_version, '--json-output',
                self.m.json.output(), '--no-developer-mode-check'
            ])
            yield result.json.output
        finally:
            self.m.step('reset xcode',
                        ['sudo', '/usr/bin/xcode-select', '--reset'])
