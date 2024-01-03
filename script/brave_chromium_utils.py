# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import contextlib
import functools
import os.path
import sys


@functools.lru_cache(maxsize=None)
def get_src_dir() -> str:
    """Searches for src/ dir which includes brave/ dir."""
    current_file = globals().get('__file__')
    if current_file:
        current_dir = os.path.dirname(os.path.abspath(current_file))
    else:
        current_dir = os.getcwd()
    while True:
        if os.path.basename(current_dir) == 'src' and os.path.isdir(
                os.path.join(current_dir, 'brave')):
            return current_dir
        parent_dir = os.path.dirname(current_dir)
        if parent_dir == current_dir:
            # We hit the system root directory.
            raise RuntimeError("Can't find src/ directory")
        current_dir = parent_dir


# Returns OS path from workspace path (//brave/path/file.py).
def wspath(path: str) -> str:
    assert isinstance(path, str)

    if path.startswith('//'):
        path = os.path.join(get_src_dir(), path[2:])

    # Normalize path separators.
    return os.path.normpath(path)


# Inline file by executing it using passed scopes.
def inline_file(path: str, _globals, _locals):
    path = wspath(path)
    with open(path, "r") as f:
        # Compile first to set the location explicitly. This makes stacktrace to
        # show the actual filename instead of '<string>'.
        code = compile(f.read(), path, 'exec')
        # pylint: disable=exec-used
        exec(code, _globals, _locals)


# Locate src/ dir and inline relative file by executing it using passed scopes.
def inline_file_from_src(path: str, _globals, _locals):
    inline_file(f"//{path}", _globals, _locals)


@contextlib.contextmanager
def sys_path(path: str, position=None):
    path = wspath(path)
    path_exists = path in sys.path
    if not path_exists:
        if position is None:
            sys.path.append(path)
        else:
            sys.path.insert(position, path)
    try:
        yield
    finally:
        if not path_exists:
            if sys.path[-1] == path:
                sys.path.pop()
            else:
                sys.path.remove(path)
