#!/usr/bin/env python3

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import importlib.util
import os
from pathlib import Path
from types import ModuleType, SimpleNamespace
import unittest
from unittest import mock

_INTERNAL_CONFIG = Path(__file__).with_name('internal_config.py')


class _FakeChromiumCodeSignConfig:

    @property
    def invoker(self):
        return self._invoker


class _FakeDistribution:

    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)


def _load_internal_config():
    signing = ModuleType('signing')
    chromium_config = ModuleType('signing.chromium_config')
    chromium_config.ChromiumCodeSignConfig = _FakeChromiumCodeSignConfig
    model = ModuleType('signing.model')
    model.Distribution = _FakeDistribution
    model.NotarizeAndStapleLevel = SimpleNamespace(STAPLE='staple')

    spec = importlib.util.spec_from_file_location('internal_config_under_test',
                                                  _INTERNAL_CONFIG)
    module = importlib.util.module_from_spec(spec)
    modules = {
        'signing': signing,
        'signing.chromium_config': chromium_config,
        'signing.model': model,
    }
    with mock.patch.dict(os.environ, {'BRAVE_CHANNEL': 'nightly'}), \
         mock.patch.dict('sys.modules', modules):
        spec.loader.exec_module(module)
    return module


class InternalCodeSignConfigTest(unittest.TestCase):

    def _distribution(self, skip_signing):
        module = _load_internal_config()
        config = object.__new__(module.InternalCodeSignConfig)
        config._invoker = SimpleNamespace(args=SimpleNamespace(
            skip_signing=skip_signing))
        return config.distributions[0]

    def test_unsigned_packaging_uses_channel_from_built_app(self):
        distribution = self._distribution(skip_signing=True)

        self.assertIsNone(distribution.channel)

    def test_signed_packaging_adds_channel(self):
        distribution = self._distribution(skip_signing=False)

        self.assertEqual('nightly', distribution.channel)


if __name__ == '__main__':
    unittest.main()
