# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for the `platform` module: choose the simulated host."""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData


class PlatformTestApi(RecipeTestApi):
    """Select the host a `GenTests` case simulates."""

    def name(self, name: str) -> TestData:
        """Set the platform 'name' for the current test.

        The only three values currently allowed are 'win', 'linux', and 'mac'.
        """
        assert name in ('win', 'linux', 'mac'), f'unknown platform {name!r}'
        return self._mod_data(name=name)

    def bits(self, bits: int) -> TestData:
        """Set the bitness for the current test.

        The only two values currently allowed are 32 and 64.
        """
        assert bits in (32, 64), f'unknown bitness {bits!r}'
        return self._mod_data(bits=bits)

    def arch(self, arch: str) -> TestData:
        """Set the architecture for the current test.

        The only two values currently allowed are 'intel' and 'arm'.
        """
        assert arch in ('intel', 'arm'), f'unknown arch {arch!r}'
        return self._mod_data(arch=arch)

    def capacity(self,
                 cpu_count: int | None = None,
                 total_memory: int | None = None) -> TestData:
        """Set the host's capacity, as the step scheduler sees it.

        This has no upstream counterpart: upstream fixes a simulated host at 8
        cores and 16 GiB with no override, and covers the scheduler with engine
        level tests instead. Ours is reachable from a `GenTests` case so a
        recipe can exercise a step that has to queue for resources.

        A simulated host reports those same 8 cores and 16 GiB by default, so a
        `ResourceCost` schedules identically wherever the tests run.

        Args:
            cpu_count: Logical cores to report, or None to keep the default.
            total_memory: Physical memory in MiB, or None to keep the default.
        """
        data = {}
        if cpu_count is not None:
            data['cpu_count'] = cpu_count
        if total_memory is not None:
            data['total_memory'] = total_memory
        return self._mod_data(**data)

    def __call__(self, name: str, bits: int, arch: str = 'intel') -> TestData:
        """Set name, bitness and architecture together."""
        return self.name(name) + self.bits(bits) + self.arch(arch)
