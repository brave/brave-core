# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import contextlib
import inspect
import types

from typing import Any


def override_function(scope, name=None, condition=True):
    """Replaces an existing function in the scope."""

    def decorator(new_function):
        is_dict_scope = isinstance(scope, dict)
        function_name = name or new_function.__name__
        if is_dict_scope:
            original_function = scope.get(function_name, None)
        else:
            original_function = getattr(scope, function_name, None)

        if not callable(original_function):
            raise NameError(f'Failed to override function: '
                            f'{function_name} not found or not callable')

        def wrapped_function(*args, **kwargs):
            return new_function(original_function, *args, **kwargs)

        if not condition:
            wrapped_function = original_function

        if is_dict_scope:
            scope[function_name] = wrapped_function
        else:
            setattr(scope, function_name, wrapped_function)

        return wrapped_function

    return decorator


def override_method(scope, name=None, condition=True):
    """Replaces an existing method in the class scope."""

    def decorator(new_method):
        assert not isinstance(scope, dict)
        method_name = name or new_method.__name__
        original_method: Any = getattr(scope, method_name, None)

        if not condition:
            wrapped_method = original_method
        else:

            def wrapped_method(self, *args, **kwargs):
                return new_method(self, original_method, *args, **kwargs)

            if inspect.ismethod(original_method):
                setattr(scope, method_name,
                        types.MethodType(wrapped_method, scope))
            else:
                assert inspect.isfunction(original_method)
                setattr(scope, method_name, wrapped_method)

        return wrapped_method

    return decorator


@contextlib.contextmanager
def override_scope_function(scope, new_function, name=None, condition=True):
    """Scoped function override helper. Can override a scope function or a class
    method."""
    if not condition:
        yield
        return

    function_name = name or new_function.__name__
    original_function = getattr(scope, function_name, None)
    try:
        if not callable(original_function):
            raise NameError(f'Failed to override scope function: '
                            f'{function_name} not found or not callable')

        if inspect.ismethod(original_function):

            def wrapped_method(self, *args, **kwargs):
                return new_function(self, original_function, *args, **kwargs)

            setattr(scope, function_name,
                    types.MethodType(wrapped_method, scope))
        else:

            def wrapped_function(*args, **kwargs):
                return new_function(original_function, *args, **kwargs)

            setattr(scope, function_name, wrapped_function)

        yield
    finally:
        if condition and original_function:
            setattr(scope, function_name, original_function)


@contextlib.contextmanager
def override_scope_variable(scope,
                            name,
                            value,
                            fail_if_not_found=True,
                            condition=True):
    """Scoped variable override helper."""
    if not condition:
        yield
        return

    var_exist = hasattr(scope, name)
    if fail_if_not_found and not var_exist:
        raise NameError(f'Failed to override scope variable: {name} not found')
    original_value = getattr(scope, name) if var_exist else None
    try:
        setattr(scope, name, value)
        yield
    finally:
        if var_exist:
            setattr(scope, name, original_value)
        else:
            delattr(scope, name)
