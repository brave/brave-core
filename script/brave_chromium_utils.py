# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import contextlib
import functools
import json
import os.path
import sys

from typing import Any, Dict, Optional


def get_additional_extensions():
    """Additional extensions which can be appended to the name of the file."""
    return [
        '.lit_mangler.ts',
    ]


@functools.lru_cache(maxsize=None)
def get_src_dir() -> str:
    """Searches for src/ dir which includes brave/ dir."""
    current_file = globals().get('__file__')
    if current_file:
        current_dir = os.path.dirname(os.path.abspath(current_file))
    else:
        current_dir = os.getcwd()
    while True:
        if os.path.exists(
                os.path.join(current_dir,
                             'brave/script/brave_chromium_utils.py')):
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
    src_path = os.path.relpath(path, src_dir)
    assert not src_path.startswith('..'), (path, src_dir)
    override_path = wspath(f'//brave/chromium_src/{src_path}')
    if not os.path.exists(override_path):
        for override_extension in get_additional_extensions():
            alt_path = override_path + override_extension
            if os.path.exists(alt_path):
                override_path = alt_path
                break
    return override_path


def to_wspath(path: str) -> str:
    """Convert path into workspace path. Examples:

    `/home/user/brave_checkout/src/chrome/file.txt` -> `//chrome/file.txt`
    `C:\\brave_checkout\\src\\chrome\\file.txt` -> `//chrome/file.txt`
    """
    assert path, path
    if not os.path.isabs(path):
        path = os.path.abspath(path)
    assert os.path.exists(path), path
    src_dir = get_src_dir()
    return '//' + os.path.relpath(path, src_dir).replace(os.path.sep, '/')


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


def get_webui_overriden_file_name(file_name):
    """Gets the name of an upstream file which is being overridden (but still
       referenced)
       for example `foo.ts` ==> `foo-chromium.ts`
                   `bar.css` ==> `bar-chromium.css`
                   `bar.css.js` ==> `bar-chromium.css.js`
    """
    name_bits = file_name.split('.')
    return "".join([name_bits[0], "-chromium.", '.'.join(name_bits[1:])])


def get_webui_overridden_but_referenced_files(folder, in_files):
    """Returns a list of files which are overridden by chromium_src but are still referenced."""
    for file in in_files:
        overridden_name = get_webui_overriden_file_name(file)
        override = os.path.join(folder, overridden_name)
        if os.path.exists(override):
            yield overridden_name


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


@functools.lru_cache(maxsize=None)
def parse_json_file_cached(file_path: str) -> Dict[str, Any]:
    """Return parsed `file_path`."""
    with open(file_path, "r") as f:
        return json.load(f)


def get_json_value(file_path: str, key: str) -> Any:
    """Return value from `file_path`."""

    value = parse_json_file_cached(file_path).get(key)
    if value is None:
        raise RuntimeError(
            "Python-checked value should be explicitly set during gn gen: "
            f"{key} value not found in {file_path}")

    return value
