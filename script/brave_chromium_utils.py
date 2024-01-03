# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import contextlib
import functools
import os.path
import sys

from typing import Any, Dict, Optional


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


def wspath(path: str) -> str:
    """Convert workspace path to OS path. Examples:

    `//chrome/file.txt` -> `/home/user/brave_checkout/src/chrome/file.txt`
    `//chrome/file.txt` -> `C:\\brave_checkout\\src\\chrome\\file.txt`
    """
    assert isinstance(path, str)

    if path.startswith('//'):
        path = os.path.join(get_src_dir(), path[2:])

    # Normalize path separators.
    return os.path.normpath(path)


def get_chromium_src_override(path: str) -> str:
    """Convert path into `//brave/chromium_src` override path."""
    assert path, path
    if not os.path.isabs(path):
        path = os.path.abspath(path)
    assert os.path.exists(path), path
    src_dir = get_src_dir()
    assert path.startswith(src_dir), (path, src_dir)
    src_path = path[len(src_dir) + 1:]
    return wspath(f'//brave/chromium_src/{src_path}')


def inline_file(path: str, _globals: Dict[str, Any],
                _locals: Dict[str, Any]) -> None:
    """Inline file from `path` by executing it using `_globals` and `_locals`
    scopes."""
    path = wspath(path)
    with open(path, "r") as f:
        # Compile first to set the location explicitly. This makes stacktrace to
        # show the actual filename instead of '<string>'.
        code = compile(f.read(), path, 'exec')
        # pylint: disable=exec-used
        exec(code, _globals, _locals)


def inline_chromium_src_override(_globals: Dict[str, Any],
                                 _locals: Dict[str, Any]) -> None:
    """Inline `__file__` override from `//brave/chromium_src`."""
    orig_file = _globals.get('__file__')
    if not orig_file:
        raise RuntimeError(
            '__file__ is not set to inline from //brave/chromium_src. '
            'Use inline_file() with full path instead.')
    chromium_src_override = get_chromium_src_override(orig_file)
    inline_file(chromium_src_override, _globals, _locals)


@contextlib.contextmanager
def sys_path(path: str, position: Optional[int] = None):
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
