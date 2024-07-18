#!/usr/bin/env python3

# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
This script invokes macOS command-line tools such as `codesign` remotely. It
works by forwarding the commands via SSH. In order for this to work, the
Chromium src/ directory must be mounted on the remote macOS machine.

For example: Suppose you have a symlink called "codesign" to this script on your
PATH. You also have a remote macOS machine that you can SSH into via the command
`ssh ventura`. On that remote machine, your Chromium src/ directory is mounted
at /MyMount.

When you now execute `codesign --sign out/Brave.app`, then this script runs and
executes, essentially, `ssh ventura "codesign --sign /MyMount/out/Brave.app"`.
The net effect is that src/out/Brave.app on your local machine gets codesigned.

This script requires several environment variables to be set. Please see below.
"""

from os.path import basename, exists, dirname, relpath, join
from shlex import quote
from subprocess import run
from tempfile import gettempdir
from time import sleep
from uuid import uuid4

import os
import sys

MACOS_HOST = os.environ['MACOS_HOST']
SRC_DIR_LOCAL = dirname(dirname(dirname(dirname(dirname(dirname(__file__))))))
SRC_DIR_MOUNT_IN_MACOS = os.environ['MACOS_SRC_DIR_MOUNT']

tmp_dir = gettempdir()

assert tmp_dir.startswith(SRC_DIR_LOCAL), \
    f'The root temporary directory {tmp_dir} must be a subdirectory of ' \
    f'{SRC_DIR_LOCAL} - otherwise, the macOS host cannot access it. Consider ' \
    f'setting the TMPDIR environment variable.'

cmd = basename(sys.argv[0])
cwd = os.getcwd()
assert cwd.startswith(SRC_DIR_LOCAL)

cwd_path_in_macos = join(SRC_DIR_MOUNT_IN_MACOS, relpath(cwd, SRC_DIR_LOCAL))
steps = [('cd', quote(cwd_path_in_macos))]

if cmd in ('codesign', 'productsign'):
    steps += [('security', 'unlock-keychain', '-p',
               quote(os.environ['KEYCHAIN_PASSWORD']))]

args = [cmd]

for arg in sys.argv[1:]:
    if exists(arg) or (exists(dirname(arg)) and dirname(arg) != '/'):
        arg = relpath(arg, cwd)
    args.append(quote(arg))

pkg_dest_on_host = None
if cmd == 'pkgbuild' and '--analyze' not in args:
    # We get errors without this delay:
    sleep(1)
    # When SRC_DIR_MOUNT_IN_MACOS is mounted via mount_9p, then pkgbuild fails
    # to write to it. So write to a known-writeable location. Further below,
    # we then `mv` to the correct destination.
    pkg_dest_on_host = args[-1]
    pkg_dest_on_guest = '~/pkgbuild-pkg-' + uuid4().hex
    args[-1] = pkg_dest_on_guest

steps.append(args)

if pkg_dest_on_host:
    steps.append(('mv', pkg_dest_on_guest, pkg_dest_on_host))

remote_command = ' && '.join(map(' '.join, steps))

cp = run(['ssh', MACOS_HOST, remote_command])  # pylint: disable=subprocess-run-check

exit_code = cp.returncode

if cmd == 'xcodebuild' and sys.argv[1:] == ['-version']:
    # Upstream runs this command line purely for informational purposes. If only
    # the command line tools of Xcode are installed, then the command exits with
    # an error (code 1). But the build goes through just fine. So ignore the
    # non-zero exit code:
    exit_code = 0

sys.exit(exit_code)
