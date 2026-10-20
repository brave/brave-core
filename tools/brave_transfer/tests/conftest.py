# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
import sys
from pathlib import Path

import pytest

TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

# pylint: disable=wrong-import-position
import gen_vectors


@pytest.fixture
def vectors():

    def load(name):
        return json.loads((gen_vectors.DATA_DIR / name).read_text())

    return load
