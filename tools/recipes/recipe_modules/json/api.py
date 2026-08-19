# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `json` module API: JSON in and out of steps.

Return codes are a poor channel for anything richer than "did it work", so the
usual way for a step to report back is to write JSON. `api.json.output()` gives
the step a path to write, and hands the parsed result back on the step's result
as `step_result.json.output`; `api.json.input(data)` goes the other way.

Deliberately not supported: `add_json_log` (mirroring the parsed JSON into a
step log), which needs step presentation.
"""

from __future__ import annotations

import functools
import json
from pathlib import Path
from typing import Any

from google.protobuf import json_format as jsonpb
from google.protobuf import struct_pb2

import config_types
from recipe_api import OutputPlaceholder, RecipeApi, returns_placeholder

# JSON is meant to be read by whoever debugs a build, so anything encoded here
# gets the same treatment: stable key order and a real indent.
_INDENT = 2

# Beyond this magnitude a JSON number can't round-trip through a float, so a
# whole float in range is safe to hand back as the int it plainly is.
MIN_SAFE_INTEGER = -((2**53) - 1)
MAX_SAFE_INTEGER = (2**53) - 1


def _default_serializer(obj: Any):
    """Make the types recipes pass around routinely JSON-serializable."""
    if isinstance(obj, (Path, config_types.Path)):
        return str(obj)
    if isinstance(obj, struct_pb2.Struct):
        # A proto Struct has exactly one sensible JSON form, so coercing it
        # keeps code that serializes plain dicts working for Structs too. Other
        # message types are deliberately left out: how a message becomes JSON
        # is a choice (field name capitalization, and so on), so those go
        # through the `proto` module instead.
        return jsonpb.MessageToDict(obj)
    raise TypeError(f'{obj!r} is not JSON serializable')


@functools.wraps(json.dumps)
def dumps(*args, **kwargs) -> str:
    """Works like `json.dumps`, sorting keys and handling recipe types."""
    kwargs.setdefault('sort_keys', True)
    kwargs.setdefault('default', _default_serializer)
    return json.dumps(*args, **kwargs)


def fix_json_object(obj: Any) -> Any:
    """Replace whole, in-range floats in *obj* with the ints they represent.

    JSON has one number type, so a producer's `1` may arrive as `1.0`. Turning
    those back into ints keeps a recipe from having to defend against it (and
    keeps expectations from depending on which side of the wire wrote them).
    """
    if isinstance(obj, list):
        return [fix_json_object(item) for item in obj]
    if isinstance(obj, float):
        if obj.is_integer() and MIN_SAFE_INTEGER <= obj <= MAX_SAFE_INTEGER:
            return int(obj)
        return obj
    if isinstance(obj, dict):
        return {
            fix_json_object(key): fix_json_object(value)
            for key, value in obj.items()
        }
    return obj


@functools.wraps(json.loads)
def loads(data: str, **kwargs) -> Any:
    """Works like `json.loads`, but see `fix_json_object` for the difference."""
    return fix_json_object(json.loads(data, **kwargs))


class JsonOutputPlaceholder(OutputPlaceholder):
    """Renders to a path the step writes JSON to, then parses it back.

    This is a `raw_io.output_text` placeholder with a `json.loads` on the end:
    the step sees an ordinary file path (e.g. `/tmp/tmp4lp1qM`), and by the time
    the recipe sees the result it is a parsed Python value. A file the step
    never wrote, or wrote something that isn't JSON to, results in `None` rather
    than an exception, so a recipe can decide for itself how much it cares.
    """

    def __init__(self,
                 api,
                 name: str | None = None,
                 leak_to: str | Path | None = None) -> None:
        self.raw = api.m.raw_io.output_text('.json', leak_to=leak_to)
        super().__init__(name=name)

    @property
    def backing_file(self) -> str | None:
        return self.raw.backing_file

    def render(self, test) -> list[str]:
        return self.raw.render(test)

    def result(self, test) -> Any:
        raw_data = self.raw.result(test)
        if raw_data is None:
            return None
        try:
            return loads(raw_data)
        except ValueError:
            return None


class JsonApi(RecipeApi):
    """Encode and decode JSON, and carry it in and out of steps."""

    @staticmethod
    def dumps(*args, **kwargs) -> str:
        """Works like `json.dumps`.

        Dictionary keys are sorted by default (pass `sort_keys=False` to keep
        insertion order), and `Path`/proto `Struct` values serialize sensibly.
        """
        return dumps(*args, **kwargs)

    @staticmethod
    def loads(data: str, **kwargs) -> Any:
        """Works like `json.loads`, but whole in-range floats become ints."""
        return loads(data, **kwargs)

    @returns_placeholder
    def input(self, data: Any, sort_keys: bool = True):
        """A placeholder expanding to the path of a file holding *data* as JSON.

        Args:
            data: A JSON-serializable value to hand the step.
            sort_keys: Sort dictionary keys. On by default so the rendered JSON
                (and any expectation quoting it) is stable; turn it off when the
                step cares about the original order.
        """
        return self.m.raw_io.input_text(
            self.dumps(data, indent=_INDENT, sort_keys=sort_keys), '.json')

    @returns_placeholder
    def output(self,
               name: str | None = None,
               leak_to: str | Path | None = None) -> JsonOutputPlaceholder:
        """A placeholder expanding to a path the step writes JSON to.

        Once the step is done, the engine parses that file and files the result
        onto the step's result as `step_result.json.output`. Also usable as a
        step's `stdout`/`stderr`, in which case the parsed value is the result's
        `stdout`/`stderr`.

        Args:
            name: Distinguishes this placeholder from others produced by the
                same method on one step. With a name, the result is also at
                `step_result.json.outputs[name]`.
            leak_to: Write to this path instead of a temporary file, and do not
                delete it afterwards (i.e. "leak" it), so a later step can pick
                it up.
        """
        return JsonOutputPlaceholder(self, name=name, leak_to=leak_to)
