# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `osx_sdk`: seed its two steps' JSON output.

Exposes the module's simulated preconditions as helpers so a recipe that
merely depends on `osx_sdk` can arrange them without itself depending on
`json`/`step`.
"""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData


class OSXSDKTestApi(RecipeTestApi):
    """Seed `ensure()`'s two steps: reading the gni pin, then installing."""

    def mac_sdk_gni(self,
                    sdk_version: str = '26.5',
                    sdk_build_version: str = '25F70') -> TestData:
        """Seed `read mac_sdk.gni`'s parsed SDK version/build pin."""
        return self.step_data(
            'read mac_sdk.gni',
            self.m.json.output({
                'sdk_version': sdk_version,
                'sdk_build_version': sdk_build_version,
            }))

    def installed(self,
                  app: str = '/Applications/xcode_25f70.app',
                  xcode_version: str = '26.5',
                  xcode_build: str = '17F42',
                  sdk_version: str = '26.5',
                  sdk_build_version: str = '25F70') -> TestData:
        """Seed `install xcode`'s resolved Xcode info, and the gni pin it was
        resolved from (via `mac_sdk_gni()`), so a test needs only this one
        call to exercise the happy path end to end.
        """
        return self.mac_sdk_gni(
            sdk_version=sdk_version,
            sdk_build_version=sdk_build_version) + self.step_data(
                'install xcode',
                self.m.json.output({
                    'app': app,
                    'xcode_version': xcode_version,
                    'xcode_build': xcode_build,
                    'sdk_version': sdk_version,
                    'sdk_build_version': sdk_build_version,
                }))
