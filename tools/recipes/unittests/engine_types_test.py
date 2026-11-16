#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the engine-level value types."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
import engine
from engine_types import StepPresentation

engine._ensure_protos()  # pylint: disable=protected-access
# pylint: disable-next=import-error,wrong-import-order
from PB.recipe_modules.brave.step.tests.output_properties import Artefact


class StepPresentationTest(unittest.TestCase):
    def test_finalize_exports_json_values(self):
        presentation = StepPresentation('step')
        presentation.properties['b'] = ('x', 1)
        presentation.properties['a'] = Artefact(url='u', platforms=['linux'])
        self.assertEqual(
            presentation.finalize(),
            {'a': {'url': 'u', 'platforms': ['linux']}, 'b': ['x', 1]},
        )

    def test_properties_are_write_only_once_finalised(self):
        presentation = StepPresentation('step')
        presentation.properties['k'] = 'v'
        presentation.finalize()
        presentation.properties['k'] = 'changed'
        self.assertEqual(presentation.properties, {'k': 'v'})
        with self.assertRaises(AssertionError):
            presentation.properties = {}

    def test_properties_can_be_replaced_before_finalising(self):
        presentation = StepPresentation('step')
        presentation.properties = {'k': 'v'}
        self.assertEqual(presentation.finalize(), {'k': 'v'})

    def test_non_json_value_is_rejected(self):
        presentation = StepPresentation('step')
        presentation.properties['k'] = object()
        with self.assertRaises(TypeError):
            presentation.finalize()


if __name__ == '__main__':
    unittest.main()
