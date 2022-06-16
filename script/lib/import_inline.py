# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import importlib.util


def inline_module(module_name, _globals, _locals):
    """Finds module and inlines it by executing using passed scopes."""
    module_spec = importlib.util.find_spec(module_name)
    if not module_spec:
        raise ModuleNotFoundError(
            f"Can't find module to inline: {module_name}")
    # pylint: disable=exec-used
    exec(module_spec.loader.get_data(module_spec.loader.path), _globals,
         _locals)


def inline_file(location, _globals, _locals):
    """Inlines file by executing using passed scopes."""
    with open(location, "r") as f:
        # pylint: disable=exec-used
        exec(f.read(), _globals, _locals)
