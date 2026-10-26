#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Runs one shard of a unittest file: run_shard.py <test file> <index> <count>.

Tests are dealt out round-robin, so shards of one file can run in parallel.
"""

import importlib.util
import sys
import unittest
from pathlib import Path


def _flatten(suite):
    for test in suite:
        if isinstance(test, unittest.TestSuite):
            yield from _flatten(test)
        else:
            yield test


def main() -> int:
    path = Path(sys.argv[1]).resolve()
    index, count = int(sys.argv[2]), int(sys.argv[3])
    sys.path.insert(0, str(path.parent))
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[path.stem] = module
    spec.loader.exec_module(module)
    tests = list(
        _flatten(unittest.defaultTestLoader.loadTestsFromModule(module))
    )
    result = unittest.TextTestRunner().run(
        unittest.TestSuite(tests[index::count])
    )
    return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
    sys.exit(main())
