# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import contextlib
import inspect
import types


def override_check(scope, name=None):
    """Replaces existing PRESUBMIT check. Can be used with globals() scope or a
    class scope (such as input_api.canned_checks)."""
    def decorator(new_func):
        is_dict_scope = isinstance(scope, dict)
        check_name = name or new_func.__name__
        if is_dict_scope:
            original_check = scope.get(check_name, None)
        else:
            original_check = getattr(scope, check_name, None)

        if not callable(original_check):
            print(f'WARNING: {check_name} check to override not found.\n'
                  'Please update chromium_presubmit_overrides.py!')

            def noop_check(*_, **__):
                return []

            return noop_check

        def wrapped_f(*args, **kwargs):
            return new_func(original_check, *args, **kwargs)

        if is_dict_scope:
            scope[check_name] = wrapped_f
        else:
            setattr(scope, check_name, wrapped_f)

        return wrapped_f

    return decorator


@contextlib.contextmanager
def override_scope_function(scope, new_function, name=None):
    """Scoped function override helper. Can override a scope method or a class
    function."""
    function_name = name or new_function.__name__
    original_function = getattr(scope, function_name, None)
    try:
        if not callable(original_function):
            raise NameError(f'Failed to override scope function: '
                            f'{function_name} not found or not callable')

        def wrapped_method(self, *args, **kwargs):
            return new_function(self, original_function, *args, **kwargs)

        def wrapped_function(*args, **kwargs):
            return new_function(original_function, *args, **kwargs)

        if inspect.ismethod(original_function):
            setattr(scope, function_name,
                    types.MethodType(wrapped_method, scope))
        else:
            setattr(scope, function_name, wrapped_function)

        yield
    finally:
        if original_function:
            setattr(scope, function_name, original_function)


@contextlib.contextmanager
def override_scope_variable(scope, name, value):
    """Scoped variable override helper."""
    if not hasattr(scope, name):
        raise NameError(f'Failed to override scope variable: {name} not found')
    original_value = getattr(scope, name)
    try:
        setattr(scope, name, value)
        yield
    finally:
        setattr(scope, name, original_value)


def get_first_check_name(scope):
    """Returns first Check* method from the scope."""
    assert isinstance(scope, dict)
    for key, value in scope.items():
        if key.startswith('Check') and callable(value):
            return key
    raise LookupError('Check* method not found in scope')
