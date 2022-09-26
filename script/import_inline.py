# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import importlib.util
import os.path


def get_src_dir():
    """Searches for src/ dir which includes brave/ dir."""
    current_file = globals().get('__file__')
    if current_file and os.path.isabs(current_file):
        path = os.path.dirname(current_file)
    else:
        path = os.getcwd()
    while True:
        if os.path.basename(path) == 'src' and os.path.isdir(
                os.path.join(path, 'brave')):
            return path
        parent_dir = os.path.dirname(path)
        if parent_dir == path:
            # We hit the system root directory.
            raise RuntimeError("Can't find src/ directory")
        path = parent_dir


def join_src_dir(*args):
    return os.path.join(get_src_dir(), *args)


def _inline_file(location, _globals, _locals):
    """Inlines file by executing it using passed scopes."""
    with open(location, "r") as f:
        # pylint: disable=exec-used
        exec(f.read(), _globals, _locals)


def inline_module(module_name, _globals, _locals):
    """Finds module and inlines it by executing using passed scopes."""
    module_spec = importlib.util.find_spec(module_name)
    if not module_spec:
        raise ModuleNotFoundError(
            f"Can't find module to inline: {module_name}")
    # pylint: disable=exec-used
    exec(module_spec.loader.get_data(module_spec.loader.path), _globals,
         _locals)


def inline_file_from_src(location, _globals, _locals):
    """Locates src/ dir and inlines relative file by executing it using passed
    scopes."""
    _inline_file(join_src_dir(location), _globals, _locals)
