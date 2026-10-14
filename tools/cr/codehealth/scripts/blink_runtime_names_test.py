#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for blink_runtime_names.py."""

from __future__ import annotations

from pathlib import Path
import subprocess
import tempfile
import unittest

from blink_runtime_names import Use, brave_uses, check, json5_names

_JSON5 = """\
{
  // A comment, and unquoted keys and trailing commas, as json5 allows.
  parameters: {
    name: {},
    status: {},
  },
  data: [
    {
      name: "Kept",
      status: "stable",
    },
    {
      // name: "Commented",
      name: "AlsoKept",
      depends_on: ["Kept"],
      status: {"Android": "", "default": "stable"},
    },
  ],
}
"""

_SOURCE = """\
void Disable() {
  blink::WebRuntimeFeatures::EnableFeatureFromString("Kept", false);
  blink::WebRuntimeFeatures::EnableFeatureFromString(
      "Gone", false);
  blink::WebRuntimeFeatures::EnableFledge(false);
  Log("NotAFeature");
}
"""


class Json5NamesTest(unittest.TestCase):
    """Real ast-grep runs."""

    def test_entry_names(self) -> None:
        self.assertEqual({'Kept', 'AlsoKept'}, json5_names(_JSON5))


class BraveUsesTest(unittest.TestCase):
    """Real ast-grep runs, on a git repository."""

    def test_first_arguments_of_the_calls(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'renderer').mkdir()
            (root / 'renderer/client.cc').write_text(_SOURCE, encoding='utf-8')
            (root / 'other.cc').write_text('int x;\n', encoding='utf-8')
            for args in (['init', '-q'], ['add', '.']):
                subprocess.run(['git', '-C', tmp, *args], check=True)
            uses = brave_uses(root)
        self.assertEqual(
            [
                Use('Kept', 'renderer/client.cc:2'),
                Use('Gone', 'renderer/client.cc:4'),
            ],
            uses,
        )


class CheckTest(unittest.TestCase):
    def test_removed_in_range_and_missing_at_both(self) -> None:
        uses = [
            Use('Kept', 'a.cc:1'),
            Use('Gone', 'a.cc:2'),
            Use('NeverThere', 'a.cc:3'),
        ]
        self.assertEqual(
            (['removed\tGone\ta.cc:2'], 1),
            check(uses, {'Kept', 'Gone'}, {'Kept'}),
        )


if __name__ == '__main__':
    unittest.main()
