# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""An advisory file lock, held by the kernel, so `kill -9` releases it.

A fresh descriptor is opened per acquisition, so one lock serializes threads of
a single process as well as separate processes: flock belongs to the open file
description, and two descriptions of one file exclude each other no matter who
opened them.

Always lock a file that is never replaced. flock lives on the inode, so a
lock taken on a file that is then rewritten via `os.replace` guards nothing.
Where the guarded file is written atomically, lock a sentinel beside it
instead (`<name>.lock`), which is what `locked_json_update` does.
"""

import errno
import fcntl
import json
import os
import tempfile
import time
from contextlib import contextmanager

# Long enough for a cold fetch of a large repo to finish ahead of us, short
# enough that a wedged holder surfaces as an error instead of a hung run.
DEFAULT_TIMEOUT_S = 600


class FileLockTimeout(RuntimeError):
    """Nobody released the lock within the timeout."""


@contextmanager
def file_lock(path, timeout=DEFAULT_TIMEOUT_S, what=None):
    """Hold an exclusive lock on `path`, waiting up to `timeout` seconds."""
    os.makedirs(os.path.dirname(os.path.abspath(path)) or ".", exist_ok=True)
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o644)
    deadline = time.monotonic() + timeout
    try:
        while True:
            try:
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                break
            except OSError as exc:
                if exc.errno not in (errno.EAGAIN, errno.EACCES):
                    raise
                if time.monotonic() >= deadline:
                    raise FileLockTimeout(
                        f"Timed out after {timeout}s waiting for the lock on "
                        f"{what or path}. Another review-prs run may be "
                        "wedged.") from exc
                time.sleep(0.2)
        try:
            yield path
        finally:
            fcntl.flock(fd, fcntl.LOCK_UN)
    finally:
        os.close(fd)


@contextmanager
def locked_json_update(path, default=None, timeout=DEFAULT_TIMEOUT_S):
    """Read a JSON file, yield it for mutation, write it back atomically.

    The whole read-modify-write is one critical section, so two runs finishing
    moments apart cannot drop each other's entries. The write is a rename, so
    a process killed mid-write leaves the previous contents, not half a file.
    """
    lock_file = f"{path}.lock"
    with file_lock(lock_file, timeout=timeout, what=path):
        try:
            with open(path) as f:
                data = json.load(f)
        except (FileNotFoundError, json.JSONDecodeError):
            data = {} if default is None else default

        yield data

        directory = os.path.dirname(os.path.abspath(path))
        fd, tmp = tempfile.mkstemp(dir=directory, prefix=".json-update-")
        try:
            with os.fdopen(fd, "w") as f:
                json.dump(data, f, indent=2)
                f.write("\n")
            os.replace(tmp, path)
        except BaseException:
            os.unlink(tmp)
            raise
