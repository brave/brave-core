# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Recipe property declarations with optional environment-variable defaults.

Environment properties allow the pipeline to pass property values via the
environment.

Recipes declare properties as a frozen dataclass whose fields use `Property()`:

    @dataclass(frozen=True)
    class InputProperties:
        chromium_ref: str = Property(from_environ='CHROMIUM_REF')
        git_cache: str | None = Property(default=None)

Environment values are strings, so `from_environ` best suits string-typed
properties. `int` and `bool` fields are coerced from their string form.
"""

from __future__ import annotations

from collections.abc import Mapping
import dataclasses
from typing import Any

# Metadata key under which a field stores the env var it may be sourced from.
_FROM_ENVIRON = 'from_environ'


def Property(*,
             default: Any = dataclasses.MISSING,
             from_environ: str | None = None) -> Any:
    """Declare a recipe property field, optionally sourced from an env var.

    Args:
        default: The value used when the property is neither passed explicitly
            nor found in the environment. Omit to make the property required.
        from_environ: Name of an environment variable to read the value from
            when it is not passed explicitly.

    Returns:
        A `dataclasses.field` carrying the `from_environ` metadata. A field
        without a `default` stays required: if neither the properties payload
        nor the environment supplies it, constructing the dataclass raises.
    """
    metadata = {_FROM_ENVIRON: from_environ} if from_environ else {}
    if default is dataclasses.MISSING:
        return dataclasses.field(metadata=metadata)
    return dataclasses.field(default=default, metadata=metadata)


def apply_environ(properties_def: type | None, values: Mapping[str, Any],
                  environ: Mapping[str, str]) -> dict[str, Any]:
    """Return *values* augmented with env-sourced properties.

    For each dataclass field declaring `from_environ`, if the property was not
    given explicitly in *values* but its env var is set, take (and coerce) the
    value from *environ*. Explicit values always win over the environment.
    """
    merged = dict(values)
    if not dataclasses.is_dataclass(properties_def):
        return merged
    for field in dataclasses.fields(properties_def):
        env = field.metadata.get(_FROM_ENVIRON)
        if env and field.name not in merged and env in environ:
            merged[field.name] = _coerce(environ[env], field.type)
    return merged


def _coerce(value: str, field_type: Any) -> Any:
    """Coerce an environment string to a field's simple scalar type.

    `field_type` is the field's annotation. It may be a string (under
    `from __future__ import annotations`, e.g. `'int'`) or the actual type
    object (e.g. `int`), so match on the type name to handle both.
    """
    name = getattr(field_type, '__name__', field_type)
    if name == 'int':
        return int(value)
    if name == 'bool':
        return value.strip().lower() in ('1', 'true', 'yes', 'on')
    return value
