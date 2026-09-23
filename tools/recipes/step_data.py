# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`StepData`: what `api.step(...)` hands back.

Besides the step's `retcode` (and the results of its `stdout`/`stderr`
placeholders, if it had any), a `StepData` grows an attribute per output
placeholder the step's command contained, filed under that placeholder's
namespaces. So a step run as:

    step_result = api.step('run tests', [script, api.json.output()])

comes back with `step_result.json.output` holding the parsed JSON the script
wrote.
"""

from __future__ import annotations

from typing import Any

from recipe_api import Placeholder

# Distinguishes "no result staged" from a placeholder whose result is None.
_UNSET = object()


class _PlaceholderNamespace:
    """One tier of a placeholder namespace on a `StepData` (e.g. `.json`).

    Holds the results filed under it as plain attributes, and raises a helpful
    `AttributeError` for anything else -- a step that didn't pass
    `api.json.output()` has no `step_result.json.output` to read.
    """

    def __init__(self, step_name: str, namespace: str) -> None:
        # Set through __dict__ because __setattr__ below consults `_frozen`,
        # which doesn't exist yet.
        self.__dict__['_step_name'] = step_name
        self.__dict__['_namespace'] = namespace
        self.__dict__['_frozen'] = False

    def __getattr__(self, name: str):
        raise AttributeError(
            f'StepData({self._step_name!r}).{self._namespace} has no attribute '
            f'{name!r}.')

    def __setattr__(self, name: str, value: Any) -> None:
        if self.__dict__.get('_frozen'):
            raise AttributeError(
                f'Cannot assign to StepData({self._step_name!r})'
                f'.{self._namespace}.{name}.')
        self.__dict__[name] = value

    def _freeze(self) -> None:
        self.__dict__['_frozen'] = True


class StepData:
    """The result of running one step.

    The always-present members are:

      * `name`: the step's name.
      * `retcode`: the command's exit code.
      * `stdout` / `stderr`: the result of the step's `stdout`/`stderr`
        placeholder, or `None` when the step didn't redirect that handle.

    On top of those, each output placeholder in the step's command contributes
    one attribute, addressed by the placeholder's namespaces (its module and
    method name) and, optionally, the caller-chosen placeholder `name`:

      * An unnamed placeholder is filed at `<module>.<method>`. Two unnamed
        placeholders from the same method on one step is an error.
      * A named placeholder is filed at `<module>.<method>s[name]` (note the
        plural).
      * If exactly one placeholder from a method has a name, and none is
        unnamed, its result is *also* filed at `<module>.<method>`.

    So `api.step('...', [script, api.json.output()])` yields
    `step_result.json.output`, while
    `api.step('...', [script, api.json.output(name='a'),
    api.json.output(name='b')])` yields `step_result.json.outputs['a']` and
    `['b']`.
    """

    def __init__(self, name: str, retcode: int) -> None:
        self.name = name
        self.retcode = retcode
        # Results of the step's std handle placeholders, if it had any. Set by
        # the `step` module once the step is done.
        self.stdout: Any = None
        self.stderr: Any = None
        # namespaces -> {placeholder name or None: result}, filled in by
        # `assign_placeholder` and turned into real attributes by `finalize`.
        self._staged: dict[tuple[str, str], dict[str | None, Any]] = {}
        self._frozen = False

    def assign_placeholder(self, placeholder: Placeholder,
                           result: Any) -> None:
        """Stage one output placeholder's *result*, to be filed by `finalize`."""
        if self._frozen:
            raise ValueError(
                f'Cannot assign placeholder {placeholder.label!r} on the '
                f'finalized result of step {self.name!r}')
        by_name = self._staged.setdefault(placeholder.namespaces, {})
        if placeholder.name in by_name:
            raise ValueError(
                f'Step {self.name!r} has two {placeholder.label!r} '
                'placeholders. Give them distinct names.')
        by_name[placeholder.name] = result

    def finalize(self) -> None:
        """File every staged placeholder result, then stop accepting writes."""
        if self._frozen:
            return
        for namespace, by_name in self._staged.items():
            by_name = dict(by_name)
            default = by_name.pop(None, _UNSET)
            if default is _UNSET and len(by_name) == 1:
                # Exactly one named placeholder, and nothing unnamed: it is
                # also reachable without its name.
                default = next(iter(by_name.values()))
            if default is not _UNSET:
                self._set(namespace, default)
            if by_name:
                module, method = namespace
                self._set((module, method + 's'), by_name)
        self._staged = {}
        for value in self.__dict__.values():
            if isinstance(value, _PlaceholderNamespace):
                value._freeze()  # pylint: disable=protected-access
        self._frozen = True

    def _set(self, namespace: tuple[str, str], value: Any) -> None:
        """Set `self.<module>.<method> = value`, creating the tier as needed."""
        module, method = namespace
        tier = self.__dict__.get(module)
        if not isinstance(tier, _PlaceholderNamespace):
            tier = _PlaceholderNamespace(self.name, module)
            setattr(self, module, tier)
        setattr(tier, method, value)

    def __setattr__(self, name: str, value: Any) -> None:
        if self.__dict__.get('_frozen'):
            raise ValueError(
                f'Cannot assign to {name!r} on the finalized result of step '
                f'{self.name!r}')
        object.__setattr__(self, name, value)

    def __getattr__(self, name: str):
        step_name = self.__dict__.get('name')
        raise AttributeError(
            f'The result of step {step_name!r} has no attribute {name!r}.')
