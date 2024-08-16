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
"""

from os.path import basename, exists, dirname, relpath, join
from shlex import quote
from subprocess import run
from tempfile import gettempdir
from time import sleep

import os
import sys

SRC_DIR = dirname(dirname(dirname(dirname(dirname(dirname(__file__))))))


def main(argv):
    check_tmp_dir()
    tool, args = basename(argv[0]), argv[1:]
    host, src_dir_on_host, keychain_pw, keychain_path = read_env_vars(tool)
    remote_commands = get_remote_commands(tool, args, os.getcwd(),
                                          src_dir_on_host, keychain_pw,
                                          keychain_path)
    exit_code = run_via_ssh(host, remote_commands)
    return get_outer_exit_code(tool, args, exit_code)


def check_tmp_dir():
    tmp_dir = gettempdir()
    assert tmp_dir.startswith(SRC_DIR), \
        f'The root temporary directory {tmp_dir} must be a subdirectory of ' \
        f'{SRC_DIR} - otherwise, the macOS host cannot access it. ' \
        f'Consider setting the TMPDIR environment variable.'


def read_env_vars(tool):
    host = require_env_var('MACOS_HOST')
    src_dir_on_host = require_env_var('MACOS_SRC_DIR_MOUNT')
    keychain_pw = \
        require_env_var('KEYCHAIN_PASSWORD') if requires_keychain(tool) else ''
    keychain_path = os.getenv('KEYCHAIN_PATH', '')
    return host, src_dir_on_host, keychain_pw, keychain_path


def get_remote_commands(tool, args, cwd, src_dir_on_host, keychain_pw,
                        keychain_path):
    result = []
    cwd_on_host = join(src_dir_on_host, relpath(cwd, SRC_DIR))
    result.append(['cd', quote(cwd_on_host)])
    if requires_keychain(tool):
        unlock_keychain_cmd = [
            'security', 'unlock-keychain', '-p',
            quote(keychain_pw)
        ]
        if keychain_path:
            unlock_keychain_cmd.append(quote(keychain_path))
        result.append(unlock_keychain_cmd)
    args_on_host = make_relative(args, cwd)
    if tool == 'pkgbuild' and '--analyze' not in args:
        # We get errors without this delay:
        result.append(['sleep', '1'])
        result.extend(get_commands_via_tmpfile(tool, args_on_host))
    elif tool == 'productsign':
        result.extend(get_commands_via_tmpfile(tool, args_on_host))
    else:
        result.append([tool] + args_on_host)
    return result


def get_commands_via_tmpfile(tool, args):
    # When src_dir_on_host is mounted via mount_9p, then some tools fail to
    # write to it. So write to a known-writeable location. Then `mv` to the
    # correct destination.
    result = [['tempfile=$(mktemp)']]
    dest, dest_index = get_destination_arg(args)
    new_args = args[:dest_index] + ['$tempfile'] + args[dest_index + 1:]
    result.append([tool] + new_args)
    result.append(['mv', '$tempfile', dest])
    return result


def get_destination_arg(args):
    result = None
    for i, arg in enumerate(args):
        if arg.endswith(".pkg") or arg.endswith(".pkg'"):
            result = arg, i
    return result


def get_outer_exit_code(tool, args, exit_code_on_host):
    if tool == 'xcodebuild' and args == ['-version']:
        # Upstream runs this command line purely for informational purposes. If
        # only the command line tools of Xcode are installed, then the command
        # exits with an error (code 1). But the build goes through just fine. So
        # ignore the non-zero exit code:
        return 0
    return exit_code_on_host


def make_relative(args, cwd):
    result = []
    for arg in args:
        if exists(arg) or (exists(dirname(arg)) and dirname(arg) != '/'):
            arg = relpath(arg, cwd)
        result.append(quote(arg))
    return result


def requires_keychain(tool):
    return tool in ('codesign', 'productsign', 'pkgbuild')


def run_via_ssh(host, commands):
    command_str = ' && '.join(map(' '.join, commands))
    # pylint: disable=subprocess-run-check
    cp = run(['ssh', host, command_str])
    return cp.returncode


def require_env_var(name):
    assert name in os.environ, f'Please set environment variable {name}.'
    return os.environ[name]


if __name__ == '__main__':
    sys.exit(main(sys.argv))
