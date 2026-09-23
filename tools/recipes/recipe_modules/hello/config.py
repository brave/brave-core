# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Configuration schema and items for the `hello` module.
"""

from __future__ import annotations

from config import BadConf, ConfigGroup, Single, Static, config_item_context


def BaseConfig(TARGET='Bob'):
    # The schema for the 'config blobs' the hello module deals with. A blob is
    # not complete() until every required entry has a value. Schema factory
    # arguments are ALL_CAPS by convention and are threaded in as Static (input)
    # data via set_config(..., TARGET=...).
    return ConfigGroup(
        verb=Single(str),
        tool=Single(str, required=True),
        TARGET=Static(str(TARGET)),
    )


config_ctx = config_item_context(BaseConfig)


# `is_root` means every config item applies this one first.
@config_ctx(is_root=True)
def BASE(c):
    if c.TARGET == 'DarthVader':
        c.verb = 'Die in a fire %s!'
    else:
        c.verb = 'Hello %s'


@config_ctx(group='tool')  # Items in the same group are mutually exclusive.
def super_tool(c):
    if c.TARGET != 'Charlie':
        raise BadConf('Can only use super tool for Charlie!')
    c.tool = 'unicorn.py'


@config_ctx(group='tool')
def default_tool(c):
    c.tool = 'echo'
