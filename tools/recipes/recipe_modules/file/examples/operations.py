# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising every `file` operation beyond `read_text`
(covered separately in `examples/full.py`, alongside `_run`'s core
success/failure handling that every operation here shares).
"""

from __future__ import annotations

import post_process

DEPS = ['file', 'step']


def RunSteps(api):
    data = api.file.read_json('read config', '/etc/config.json')
    api.step('echo json', ['echo', str(data)])

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

    paths = api.file.glob_paths('glob', '/src', '*.py', include_hidden=True)
    api.step('echo glob', ['echo', str(paths)])

    entries = api.file.listdir('listdir', '/src', recursive=True)
    api.step('echo listdir', ['echo', str(entries)])

    api.file.ensure_directory('ensure dir', '/newdir', mode=0o755)

    sizes = api.file.filesizes('filesizes', ['/a.txt', '/b.txt'])
    api.step('echo sizes', ['echo', str(sizes)])

    api.file.symlink('symlink', '/target', '/link')
    api.file.truncate('truncate', '/big.bin', size_mb=10)
    api.file.flatten_single_directories('flatten', '/nested')

    sha = api.file.compute_hash('compute hash', ['/base/a.txt', '/base/dir'],
                                '/base')
    api.step('echo hash', ['echo', sha])

    file_sha = api.file.file_hash('file hash', '/a.txt')
    api.step('echo file hash', ['echo', file_sha])

    executable = api.file.is_executable('is executable', '/a.txt')
    api.step('echo executable', ['echo', str(executable)])


def GenTests(api):
    yield api.test(
        'basic',
        api.file.read_json('read config', {'key': 'value'}),
        api.file.glob_paths('glob', ['a.py', 'sub/b.py']),
        api.file.listdir('listdir', ['a.txt', 'sub/b.txt']),
        api.file.filesizes('filesizes', [111, 222]),
        api.file.compute_hash('compute hash', 'deadbeef'),
        api.file.file_hash('file hash', 'c0ffee'),
        api.file.is_executable('is executable', True),
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
        api.post_process(post_process.MustRun, 'echo json'),
        api.post_process(post_process.MustRun, 'ensure dir'),
        api.post_process(post_process.MustRun, 'symlink'),
        api.post_process(post_process.MustRun, 'truncate'),
        api.post_process(post_process.MustRun, 'flatten'),
        api.post_process(post_process.StatusSuccess),
    )
