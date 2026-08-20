# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising every `file` operation beyond reading and writing
content (covered separately in `examples/full.py`, alongside `_run`'s core
success/failure handling that every operation here shares).
"""

from __future__ import annotations

import post_process

DEPS = ['file', 'path', 'step']


def RunSteps(api):
    api.file.copy('copy file', '/src/a.txt', '/dst/a.txt')
    api.file.copytree('copy tree',
                      '/src/dir',
                      '/dst/dir',
                      symlinks=True,
                      hardlink=True,
                      allow_override=True)
    api.file.move('move file', '/src/b.txt', '/dst/b.txt')
    api.file.chmod('chmod file', '/dst/a.txt', 0o755, recursive=True)
    api.file.remove('remove file', '/dst/a.txt')
    api.file.rmtree('rmtree dir', '/dst/dir')
    api.file.rmcontents('rmcontents dir', '/dst')
    api.file.rmglob('rmglob',
                    '/dst',
                    '*.txt',
                    recursive=True,
                    include_hidden=True)

    # An already-Path `source` (not just a plain string) must not be
    # re-wrapped in a fresh native `pathlib.Path` -- see `_as_path`.
    src = api.path.abs('/src')
    paths = api.file.glob_paths('glob',
                                src,
                                '*.py',
                                include_hidden=True,
                                test_data=['a.py', 'sub/b.py'])
    api.step('echo glob', ['echo', str(paths)])

    # A plain string `source` (not yet a Path) must still be wrapped.
    entries = api.file.listdir('listdir',
                               '/src',
                               recursive=True,
                               test_data=['a.txt', 'sub/b.txt'])
    api.step('echo listdir', ['echo', str(entries)])

    api.file.ensure_directory('ensure dir', '/newdir', mode=0o755)

    sizes = api.file.filesizes('filesizes', ['/a.txt', '/b.txt'],
                               test_data=[111, 222])
    api.step('echo sizes', ['echo', str(sizes)])

    api.file.symlink('symlink', '/target', '/link')
    api.file.truncate('truncate', '/big.bin', size_mb=10)
    api.file.flatten_single_directories('flatten', '/nested')

    base = api.path.abs('/base')
    sha = api.file.compute_hash('compute hash',
                                [str(base / 'a.txt'), base / 'dir'],
                                base,
                                test_data='deadbeef')
    api.step('echo hash', ['echo', sha])

    file_sha = api.file.file_hash('file hash', '/a.txt', test_data='c0ffee')
    api.step('echo file hash', ['echo', file_sha])

    executable = api.file.is_executable('is executable',
                                        '/a.txt',
                                        test_data=True)
    api.step('echo executable', ['echo', str(executable)])


def GenTests(api):
    yield api.test(
        'basic',
        api.post_process(post_process.StepCommandContains, 'copy tree',
                         ['--symlinks', '--hardlink', '--allow-override']),
        api.post_process(post_process.StepCommandContains, 'chmod file',
                         ['--recursive']),
        api.post_process(post_process.StepCommandContains, 'rmglob',
                         ['**/*.txt', '--hidden']),
        api.post_process(post_process.StepCommandContains, 'glob',
                         ['--hidden']),
        api.post_process(post_process.StepCommandContains, 'listdir',
                         ['--recursive']),
        api.post_process(post_process.MustRun, 'ensure dir'),
        api.post_process(post_process.MustRun, 'symlink'),
        api.post_process(post_process.MustRun, 'truncate'),
        api.post_process(post_process.MustRun, 'flatten'),
        api.post_process(post_process.StatusSuccess),
    )
    # Each value-returning operation's test API helper overrides the
    # `test_data` the recipe passed.
    yield api.test(
        'seeded results',
        api.step_data('glob', api.file.glob_paths(['seeded.py'])),
        api.step_data('listdir', api.file.listdir(['seeded.txt'])),
        api.step_data('filesizes', api.file.filesizes([7])),
        api.step_data('compute hash', api.file.compute_hash('f00d')),
        api.step_data('file hash', api.file.file_hash('bead')),
        api.step_data('is executable', api.file.is_executable(False)),
        api.post_process(post_process.StepCommandContains, 'echo glob',
                         ['echo', "[Path(, 'src', 'seeded.py')]"]),
        api.post_process(post_process.StepCommandContains, 'echo sizes',
                         ['echo', '[7]']),
        api.post_process(post_process.StepCommandContains, 'echo hash',
                         ['echo', 'f00d']),
        api.post_process(post_process.StepCommandContains, 'echo executable',
                         ['echo', 'False']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
