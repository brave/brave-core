#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Utility exporting basic filesystem operations, run as a recipe step.

Every operation reports a shared `{ok, errno_name, message}` result as JSON to
the file named by `--json-output`, leaving stdout free for the operations that
have an actual value to return (`glob`, `listdir`, ...). Reading and writing
file content needs no operation of its own: `copy` does both, given a path that
the recipe engine has filled in (or will read back) for it -- see the
"Getting data back from a step" section of ../../../README.md.
"""

from __future__ import annotations

import argparse
import errno
import glob
import hashlib
import itertools
import json
import mmap
import os
import shutil
import sys
import tempfile


def _copy(source: str, dest: str) -> None:
    shutil.copy(source, dest)


def _copy_for_copytree(src: str, dest: str, hardlink: bool) -> None:
    if hardlink:
        if os.path.isfile(dest):
            os.remove(dest)
        os.link(src, dest)
    else:
        shutil.copy2(src, dest)


def _copytree(source: str, dest: str, symlinks: bool, hardlink: bool,
              allow_override: bool) -> None:
    shutil.copytree(
        source,
        dest,
        symlinks=symlinks,
        dirs_exist_ok=allow_override,
        copy_function=lambda src, dst: _copy_for_copytree(src, dst, hardlink))


def _move(source: str, dest: str) -> None:
    shutil.move(source, dest)


def _chmod(path: str, mode: int, recursive: bool) -> None:
    if not recursive:
        os.chmod(path, mode)
        return
    for dirpath, _dirnames, filenames in os.walk(path):
        os.chmod(dirpath, mode)
        for filename in filenames:
            os.chmod(os.path.join(dirpath, filename), mode)


def _remove(path: str) -> None:
    try:
        os.remove(path)
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise


def _rmtree(path: str) -> None:
    if not os.path.exists(path):
        return
    if os.path.isfile(path) or os.path.islink(path):
        os.remove(path)
        return
    # Make everything writable first so a read-only tree doesn't fail to
    # delete.
    for dirpath, _dirnames, _filenames in os.walk(path):
        os.chmod(dirpath, 0o770)
    shutil.rmtree(path)


def _rmcontents(path: str) -> None:
    if not os.path.exists(path):
        return
    os.chmod(path, 0o770)
    for name in os.listdir(path):
        full = os.path.join(path, name)
        if os.path.isdir(full) and not os.path.islink(full):
            _rmtree(full)
        else:
            os.remove(full)


def _rmglob(root: str, wildcard: str, include_hidden: bool) -> None:
    pattern = os.path.join(os.path.realpath(root), wildcard)
    # `include_hidden` was added to glob.glob() in Python 3.11; the pinned
    # pylint's stdlib stubs predate it.
    for item in glob.glob(  # pylint: disable=unexpected-keyword-arg
            pattern,
            recursive=True,
            include_hidden=include_hidden):
        try:
            os.remove(item)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise


def _glob(base: str, pattern: str, include_hidden: bool) -> str:
    base = os.path.realpath(base)
    hits = glob.glob(  # pylint: disable=unexpected-keyword-arg
        os.path.join(base, pattern),
        recursive=True,
        include_hidden=include_hidden)
    return '\n'.join(sorted(os.path.relpath(hit, start=base) for hit in hits))


def _listdir(source: str, recursive: bool) -> str:
    if recursive:
        out = []
        for dirpath, _dirnames, files in os.walk(source):
            out.extend(
                os.path.relpath(os.path.join(dirpath, f), source)
                for f in files)
    else:
        out = os.listdir(source)
    return '\n'.join(sorted(out))


def _ensure_directory(dest: str, mode: int) -> None:
    if not os.path.isdir(dest):
        if os.path.exists(dest):
            raise OSError(errno.EEXIST, os.strerror(errno.EEXIST))
        os.makedirs(dest, mode)


def _filesizes(files: list[str]) -> str:
    return '\n'.join(str(os.stat(f).st_size) for f in files)


def _symlink(source: str, linkname: str) -> None:
    os.symlink(source, linkname)


def _truncate(path: str, size_mb: int) -> None:
    with open(path, 'w', encoding='utf-8') as f:
        f.truncate(size_mb * 1024 * 1024)


def _flatten_single_directories(path: str) -> None:
    first_single_dir = None
    for root, dirs, files in os.walk(path):
        # If it's a single dir, keep walking.
        if len(dirs) == 1 and not files:
            if not first_single_dir:
                first_single_dir = os.path.join(path, dirs[0])
            continue

        # Otherwise we found some stuff.
        if not first_single_dir:
            # Still in the base directory; nothing to flatten.
            return

        # Move the first_single_dir out of the way first, in case there's a
        # file/folder we need to move that has a conflicting name.
        tmpname = tempfile.mktemp(dir=path)
        os.rename(first_single_dir, tmpname)
        for name in itertools.chain(dirs, files):
            fullname = os.path.join(root,
                                    name).replace(first_single_dir, tmpname)
            os.rename(fullname, os.path.join(path, name))
        shutil.rmtree(tmpname)
        return


def _hash_file_into(sha, rel_path: str, base_path: str) -> None:
    path = os.path.join(base_path, rel_path)
    sha.update(str(len(rel_path)).encode())
    sha.update(rel_path.encode())
    with open(path, 'rb') as f:
        size = os.fstat(f.fileno()).st_size
        sha.update(str(size).encode())
        if size:
            with mmap.mmap(f.fileno(), size, access=mmap.ACCESS_READ) as mm:
                sha.update(mm)


def _compute_hash(base_path: str, rel_paths: list[str]) -> str:
    sha = hashlib.sha256()
    for rel_path in rel_paths:
        path = os.path.join(base_path, rel_path)
        if os.path.isfile(path):
            _hash_file_into(sha, rel_path, base_path)
        elif os.path.isdir(path):
            for root, dirs, files in os.walk(path, topdown=True):
                dirs.sort()
                files.sort()
                for f_name in files:
                    rel_file_path = os.path.relpath(os.path.join(root, f_name),
                                                    base_path)
                    _hash_file_into(sha, rel_file_path, base_path)
    return sha.hexdigest()


def _file_hash(path: str) -> str:
    sha = hashlib.sha256()
    with open(path, 'rb') as f:
        size = os.fstat(f.fileno()).st_size
        if size:
            with mmap.mmap(f.fileno(), size, access=mmap.ACCESS_READ) as mm:
                sha.update(mm)
    return sha.hexdigest()


def _is_executable(path: str) -> str:
    return str(os.access(path, os.X_OK))


# Maps a subcommand name to the function implementing it; each takes the
# parsed `argparse.Namespace` and returns the string to print to stdout (or
# None, for operations with nothing to report beyond ok/errno_name/message).
_OPERATIONS = {
    'copy': lambda o: _copy(o.source, o.dest),
    'copytree': lambda o: _copytree(o.source, o.dest, o.symlinks, o.hardlink, o
                                    .allow_override),
    'move': lambda o: _move(o.source, o.dest),
    'chmod': lambda o: _chmod(o.path, o.mode, o.recursive),
    'remove': lambda o: _remove(o.source),
    'rmtree': lambda o: _rmtree(o.source),
    'rmcontents': lambda o: _rmcontents(o.source),
    'rmglob': lambda o: _rmglob(o.root, o.wildcard, o.hidden),
    'glob': lambda o: _glob(o.base, o.pattern, o.hidden),
    'listdir': lambda o: _listdir(o.source, o.recursive),
    'ensure_directory': lambda o: _ensure_directory(o.dest, o.mode),
    'filesizes': lambda o: _filesizes(o.file),
    'symlink': lambda o: _symlink(o.source, o.linkname),
    'truncate': lambda o: _truncate(o.path, o.size_mb),
    'flatten_single_directories': lambda o: _flatten_single_directories(o.path
                                                                        ),
    'compute_hash': lambda o: _compute_hash(o.base_path, o.rel_paths),
    'file_hash': lambda o: _file_hash(o.file_path),
    'is_executable': lambda o: _is_executable(o.path),
}


def _octal(value: str) -> int:
    return int(value, 8)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--json-output',
                        required=True,
                        type=argparse.FileType('w'),
                        help='Where to write the {ok, errno_name, message} '
                        'result of the operation.')
    subparsers = parser.add_subparsers(dest='command', required=True)

    p = subparsers.add_parser('copy',
                              help='Copy a file. Behaves like shutil.copy().')
    p.add_argument('source')
    p.add_argument('dest')

    p = subparsers.add_parser(
        'copytree',
        help='Recursively copy a tree. Behaves like shutil.copytree().')
    p.add_argument('--symlinks', action='store_true')
    p.add_argument('--hardlink', action='store_true')
    p.add_argument('--allow-override', action='store_true')
    p.add_argument('source')
    p.add_argument('dest')

    p = subparsers.add_parser(
        'move', help='Move/rename a file. Behaves like shutil.move().')
    p.add_argument('source')
    p.add_argument('dest')

    p = subparsers.add_parser('chmod', help='Run chmod on a file.')
    p.add_argument('path')
    p.add_argument('--mode', required=True, type=_octal)
    p.add_argument('--recursive', action='store_true')

    p = subparsers.add_parser('remove', help='Remove a file.')
    p.add_argument('source')

    p = subparsers.add_parser('rmtree', help='Recursively remove a directory.')
    p.add_argument('source')

    p = subparsers.add_parser(
        'rmcontents', help='Recursively remove the contents of a directory.')
    p.add_argument('source')

    p = subparsers.add_parser(
        'rmglob', help='Remove entries under a directory matching a glob.')
    p.add_argument('root')
    p.add_argument('wildcard')
    p.add_argument('--hidden', action='store_true')

    p = subparsers.add_parser(
        'glob', help='Print paths under a directory matching a glob.')
    p.add_argument('base')
    p.add_argument('pattern')
    p.add_argument('--hidden', action='store_true')

    p = subparsers.add_parser('listdir',
                              help='Print all entries in the given directory.')
    p.add_argument('source')
    p.add_argument('--recursive', action='store_true')

    p = subparsers.add_parser('ensure_directory',
                              help='Ensure that a path is a directory.')
    p.add_argument('dest')
    p.add_argument('--mode', required=True, type=_octal)

    p = subparsers.add_parser('filesizes',
                              help='Print each given file\'s size, in bytes.')
    p.add_argument('file', nargs='+')

    p = subparsers.add_parser(
        'symlink', help='Create a symlink. Behaves like os.symlink.')
    p.add_argument('source')
    p.add_argument('linkname')

    p = subparsers.add_parser('truncate',
                              help='Create an empty file of the given size.')
    p.add_argument('path')
    p.add_argument('size_mb', type=int)

    p = subparsers.add_parser(
        'flatten_single_directories',
        help='Move contents of nested singular directories to the top.')
    p.add_argument('path')

    p = subparsers.add_parser(
        'compute_hash',
        help='Hash the given directories/files, relative to a base path.')
    p.add_argument('base_path')
    p.add_argument('rel_paths', nargs='+')

    p = subparsers.add_parser('file_hash', help='Hash a single file.')
    p.add_argument('file_path')

    p = subparsers.add_parser('is_executable',
                              help='Check whether a file is executable.')
    p.add_argument('path')

    opts = parser.parse_args(argv)

    result = {'ok': False, 'errno_name': '', 'message': ''}
    output = ''
    try:
        output = _OPERATIONS[opts.command](opts) or ''
        result['ok'] = True
    except shutil.Error as e:
        # `shutil.Error`'s message can sometimes be a tuple; render the whole
        # exception as a string to be safe.
        result['message'] = str(e)
    except OSError as e:
        if e.errno:
            result['errno_name'] = errno.errorcode[e.errno]
        result['message'] = str(e)
    except Exception as e:  # pylint: disable=broad-except
        result['message'] = f'UNKNOWN: {e}'

    sys.stdout.write(output)
    with opts.json_output as json_output:
        json.dump(result, json_output)
    # Success/failure is reported in `result`, not the exit code: `file/api.py`
    # inspects `result['ok']` itself.
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
