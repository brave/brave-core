# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import lib.override_utils as override_utils


def override_check(scope, name=None):
    """Replaces existing PRESUBMIT check. Can be used with globals() scope or a
    class scope (such as input_api.canned_checks). Doesn't fail if the Check is
    not found, only prints a warning message."""

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

        return override_utils.override_function(scope, name=name)(new_func)

    return decorator


def get_first_check_name(scope):
    """Returns first Check* method from the scope."""
    assert isinstance(scope, dict)
    for key, value in scope.items():
        if key.startswith('Check') and callable(value):
            return key
    raise LookupError('Check* method not found in scope')
