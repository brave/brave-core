# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for exporting output properties through `step.presentation`."""

from __future__ import annotations

from dataclasses import dataclass

from PB.recipe_modules.brave.step.tests.output_properties import (
    Artefact,
    InputProperties,
)
import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import step
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    step: step.TEST_API


PROPERTIES = InputProperties


def RunSteps(api: DEPS, properties: InputProperties) -> None:
    # A plain JSON value, and a proto message exported in its JSONPB form.
    result = api.step('upload', None)
    result.presentation.properties['got_revision'] = 'deadbeef'
    result.presentation.properties['artefact'] = Artefact(
        url='https://example.com/a.tar.xz',
        sha256='abc123',
        platforms=['linux', 'mac'],
    )

    # The step finalised last wins a key.
    api.step('first', None).presentation.properties['winner'] = 'first'
    api.step('second', None).presentation.properties['winner'] = 'second'

    # Once the next step has started, a step's properties are read-only.
    late = api.step('late', None)
    api.step('after late', None)
    late.presentation.properties['too_late'] = True

    # A failing step still exports what it set before the recipe failed.
    try:
        api.step('might fail', ['do-thing'])
    except api.step.StepFailure:
        api.step.active_result.presentation.properties['failed'] = True
        raise
    finally:
        # An artefact fed back in as input arrives as the same message type.
        if properties.HasField('artefact'):
            api.step('received', None).presentation.properties['received'] = (
                properties.artefact
            )


def GenTests(api: TEST_DEPS):
    yield api.test(
        'basic',
        api.post_process(
            post_process.PropertyEquals, 'got_revision', 'deadbeef'
        ),
        api.post_process(
            post_process.PropertyMatchesCallable,
            'artefact',
            lambda check, value: check(value['platforms'] == ['linux', 'mac']),
        ),
        api.post_process(post_process.PropertyEquals, 'winner', 'second'),
        api.post_process(post_process.PropertiesDoNotContain, 'too_late'),
        api.post_process(post_process.PropertiesDoNotContain, 'failed'),
        api.post_process(post_process.PropertiesDoNotContain, 'received'),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'step fails',
        api.step.data('might fail', retcode=1),
        api.post_process(post_process.PropertyEquals, 'failed', True),
        api.post_process(
            post_process.SummaryMarkdown, "Step('might fail') (retcode: 1)"
        ),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )
    yield api.test(
        'artefact as input',
        api.properties(
            InputProperties(artefact=Artefact(url='https://example.com/b'))
        ),
        api.post_process(
            post_process.PropertyEquals,
            'received',
            {'url': 'https://example.com/b'},
        ),
        api.post_process(post_process.PropertyMatchesRE, 'got_revision', '^d'),
        api.post_process(post_process.PropertiesContain, 'received'),
        api.post_process(post_process.DropExpectation),
    )
