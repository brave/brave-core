# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the `proto` module's rejection of bad codecs and message types.

A seeded `MODE` env var selects which mistake to make; each one aborts the
recipe.
"""

from __future__ import annotations

from dataclasses import dataclass

from PB.recipe_modules.brave.proto.examples.full import Config

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    env,
    proto,
    step,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    env: env.API
    proto: proto.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    env: env.TEST_API


MODES = (
    'bad_codec',
    'output_not_a_message_class',
    'input_not_a_message',
    'output_test_data_not_a_message',
)


def RunSteps(api: DEPS):
    mode = api.env.get('MODE')
    if mode == 'bad_codec':
        # Only the three named codecs exist.
        api.step('read', ['read', api.proto.output(Config, 'YAMLPB')])
    elif mode == 'output_not_a_message_class':
        # `output` decodes into a message type, not an arbitrary class.
        api.step('read', ['read', api.proto.output(dict, api.proto.JSONPB)])
    elif mode == 'input_not_a_message':
        # ... and `input` encodes a message instance.
        api.step('cat', ['cat', api.proto.input({'a': 1}, api.proto.JSONPB)])
    else:
        api.step(
            'read',
            ['read', api.proto.output(Config, api.proto.JSONPB)],
            step_test_data=lambda: api.proto.test_api.output('not a msg'),
        )


def GenTests(api: TEST_DEPS):
    for mode in MODES:
        yield api.test(
            mode,
            api.env.set('MODE', mode),
            api.post_process(post_process.StatusException),
            api.post_process(post_process.DropExpectation),
            status='EXCEPTION',
        )
