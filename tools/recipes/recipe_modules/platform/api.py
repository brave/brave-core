# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Mockable system platform identity functions.
"""

from __future__ import annotations

import platform
import sys

import psutil

from recipe_api import RecipeApi


def norm_bits(arch: str | int) -> int:
    """Normalise a machine/bitness string to either 32 or 64."""
    return 64 if '64' in str(arch) else 32


def get_arch() -> str:
    """The current CPU architecture, as either `'arm'` or `'intel'`."""
    arch = platform.machine()
    return 'arm' if ('arm' in arch or 'aarch' in arch) else 'intel'


class PlatformApi(RecipeApi):
    """Provides host-platform-detection properties.

    Mocks:
        * name (str): One of `'win'`, `'mac'`, or `'linux'`.
        * bits (int): Either 32 or 64.
        * arch (str): Either `'intel'` or `'arm'`.
        * cpu_count (int): Logical CPU cores, via `capacity()`.
        * total_memory (int): Physical memory in MiB, via `capacity()`.
    """

    def __init__(self) -> None:
        super().__init__()
        self._name = 'linux'
        self._bits = 64
        self._arch = 'intel'
        self._num_logical_cores = 0
        self._memory_bytes = 0

    def initialise(self) -> None:
        self._name = PlatformApi.normalize_platform_name(sys.platform)
        self._arch = get_arch()
        self._bits = norm_bits(platform.machine())

        if self._test is not None:
            # Default to linux/64, unless the test case says otherwise.
            self._name = PlatformApi.normalize_platform_name(
                self._test.platform)
            self._bits = norm_bits(self._test.bits)
            self._arch = self._test.arch

            self._num_logical_cores = self._test.cpu_count
            self._memory_bytes = self._test.total_memory * (1024**2)
        else:  # pragma: no cover - production host probe.
            # platform.machine is based on the running kernel. It is possible
            # to use a 64-bit kernel with a 32-bit userland, e.g. to give the
            # linker slightly more memory. Distinguish between different
            # userland bitness by querying the python binary.
            if (self._name == 'linux' and self._bits == 64
                    and platform.architecture()[0] == '32bit'):
                self._bits = 32
            # On Mac the inverse of the linux 64-bit-kernel case is true: the
            # kernel is 32-bit but the CPU and userspace are both capable of
            # running 64-bit programs.
            elif (self._name == 'mac' and self._bits == 32
                  and platform.architecture()[0] == '64bit'):
                self._bits = 64

            self._num_logical_cores = psutil.cpu_count(True)
            self._memory_bytes = psutil.virtual_memory().total

    @property
    def is_win(self) -> bool:
        """Whether the recipe is running on Windows."""
        return self.name == 'win'

    @property
    def is_mac(self) -> bool:
        """Whether the recipe is running on macOS."""
        return self.name == 'mac'

    @property
    def is_linux(self) -> bool:
        """Whether the recipe is running on Linux."""
        return self.name == 'linux'

    @property
    def name(self) -> str:
        """The current platform name, which will be one of:
            * win
            * mac
            * linux
        """
        return self._name

    @property
    def bits(self) -> int:
        """The bitness of the userland for the current system, either 32 or 64.

        If anyone needs to query for the kernel bitness, another accessor
        should be added.
        """
        return self._bits

    @property
    def arch(self) -> str:
        """The current CPU architecture, either `'arm'` or `'intel'`."""
        return self._arch

    @property
    def total_memory(self) -> int:
        """The total physical memory in MiB.

        Equivalent to `psutil.virtual_memory().total / (1024 ** 2)`.
        """
        return self._memory_bytes // (1024**2)

    @property
    def cpu_count(self) -> int:
        """The number of logical CPU cores (i.e. including hyper-threaded
        cores), according to `psutil.cpu_count(True)`."""
        return self._num_logical_cores

    @staticmethod
    def normalize_platform_name(plat: str) -> str:
        """One of python's `sys.platform` values -> 'win', 'linux' or 'mac'."""
        if plat.startswith('linux'):
            return 'linux'
        if plat.startswith(('win', 'cygwin')):
            return 'win'
        if plat.startswith(('darwin', 'mac')):
            return 'mac'
        # Unreachable from a `GenTests` case, since the test API only accepts
        # the three known names; this guards an unrecognised real host.
        raise ValueError(  # pragma: no cover
            f"Don't understand platform {plat!r}")
