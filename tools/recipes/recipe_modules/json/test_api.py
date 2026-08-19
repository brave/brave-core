# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `json`: seed what a JSON output placeholder hands back.

`api.json.output({'passed': 791})` is a fragment for `api.step_data`, either as
one of its positional arguments (seeding an `api.json.output()` in the step's
command) or as its `stdout=`/`stderr=` (seeding the step's std handle):

    api.step_data('run tests', api.json.output({'passed': 791}))
    api.step_data('run tests', stdout=api.json.output({'passed': 791}))

`output_stream` builds the latter in one call, for a step's own
`step_test_data=` default.
"""

from __future__ import annotations

import json
from typing import Any

from recipe_test_api import (RecipeTestApi, StepTestData,
                             placeholder_step_data)

from .api import dumps, loads


class JsonTestApi(RecipeTestApi):
    """Seed the simulated data of a `json` output placeholder."""

    @staticmethod
    def dumps(*args, **kwargs) -> str:
        """Same as `api.json.dumps`."""
        return dumps(*args, **kwargs)

    @staticmethod
    def loads(data: str, **kwargs) -> Any:
        """Same as `api.json.loads`."""
        return loads(data, **kwargs)

    @placeholder_step_data
    def output(self,
               data: Any,
               retcode: int | None = None,
               name: str | None = None):
        """Seed an `api.json.output()` placeholder to return *data*.

        *data* is an ordinary Python value (dict, list, str, int, ...); the step
        is simulated as having written it out as JSON.
        """
        return json.dumps(data, indent=2, sort_keys=True), retcode, name

    @placeholder_step_data('output')
    def invalid(self,
                raw_data: str,
                retcode: int | None = None,
                name: str | None = None):
        """Seed an `api.json.output()` placeholder with text that isn't JSON.

        For exercising what a recipe does when a step writes garbage: the
        placeholder's result is then `None`.
        """
        return raw_data, retcode, name

    @placeholder_step_data('output')
    def backing_file_missing(self,
                             retcode: int | None = None,
                             name: str | None = None):
        """Seed an output placeholder as if the step never wrote its file.

        Only meaningful for a placeholder created with `leak_to`; without it the
        engine creates the backing file itself, so it is always there.
        """
        return None, retcode, name

    def output_stream(self,
                      data: Any,
                      stream: str = 'stdout',
                      retcode: int | None = None,
                      name: str | None = None) -> StepTestData:
        """Seed *data* as the JSON on the step's `stdout` (or `stderr`)."""
        assert stream in ('stdout', 'stderr')
        fragment = StepTestData()
        # `functools.wraps` (in `placeholder_step_data`) leaves pylint
        # inferring `output`'s return type from its undecorated body (a plain
        # tuple), rather than the `StepTestData` the decorator actually
        # returns.
        placeholder_data = self.output(data, retcode=retcode, name=name)
        setattr(fragment, stream, placeholder_data.unwrap_placeholder())  # pylint: disable=no-member
        if retcode:
            fragment.retcode = retcode
        return fragment
