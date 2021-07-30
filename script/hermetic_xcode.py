#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

"""Script to put a hermetic Xcode toolchain into the source tree."""

import os
import subprocess
import sys


BRAVE_CORE_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir))

CHROMIUM_SRC_ROOT = os.path.abspath(
    os.path.join(BRAVE_CORE_ROOT, os.pardir))

MAC_FILES = os.path.join(CHROMIUM_SRC_ROOT, 'build', 'mac_files')


def _run(*args, workdir=None, extra_env={}):
    # Set environment variables for subprocess
    env = os.environ.copy()
    env.update(extra_env)

    try:
        result = subprocess.run(
            args,
            cwd=workdir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True)
        return result.stdout, result.stderr
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def _normalize_xcode_version(version):
    parts = version.split('.')
    while len(parts) < 3:
        parts.append('0')
    return '.'.join(parts)


def _get_xcode_version():
    # stdout should be of format
    #   Xcode x.y.z
    #   Build version abc
    stdout, _ = _run('xcodebuild', '-version')

    version = []
    for line in stdout.split(b'\n'):
        if line.startswith(b'Xcode '):
            xcode = _normalize_xcode_version(
                str(line[len(b'Xcode '):], 'utf8'))
            version.append(xcode)
        elif line.startswith(b'Build version '):
            build = str(line[len(b'Build version '):], 'utf8')
            version.append(build)
    
    return '.'.join(version)


def _get_install_path():
    xcode_version = _get_xcode_version()

    # TODO(yannic): Use xcode-locator to ensure it's the right version?
    stdout, _ = _run('xcode-select', '--print-path')

    developer_dir = str(stdout, 'utf8')
    return {
        'version': xcode_version,
        'path': os.path.abspath(
            os.path.join(developer_dir, os.pardir, os.pardir))        
    }


def main():
    if os.path.exists(MAC_FILES):
        _run('rm', '-rf', MAC_FILES)
    _run('mkdir', '-p', MAC_FILES)

    xcode = _get_install_path()
    _run('ln', '-s', xcode['path'], 'xcode_binaries', workdir=MAC_FILES)

    return 0


if __name__ == '__main__':
    sys.exit(main())
