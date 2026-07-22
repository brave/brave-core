# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Edge case: a config item raising `BadConf` surfaces as an EXCEPTION."""

from __future__ import annotations

from post_process import DropExpectation, StatusException

DEPS = ['hello']


def RunSteps(api):
    # super_tool only accepts TARGET 'Charlie'; anything else raises BadConf.
    api.hello.set_config('super_tool', TARGET='Not Charlie')


def GenTests(api):
    yield api.test(
        'badconf',
        api.post_process(StatusException),
        api.post_process(DropExpectation),
        status='EXCEPTION',
    )
