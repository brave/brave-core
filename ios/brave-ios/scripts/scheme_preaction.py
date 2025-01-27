#!/usr/bin/env python3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Actions to run before every build in the brave-ios Xcode project
"""

import argparse
import os
import subprocess
import platform
import sys
import inspect

this_dir = os.path.dirname(os.path.abspath(__file__))
brave_root_dir = os.path.join(
    os.path.join(os.path.join(this_dir, os.pardir), os.pardir), os.pardir)
src_dir = os.path.join(brave_root_dir, os.pardir)


def main():
    description = 'Runs actions before each build'
    parser = argparse.ArgumentParser(description=description)

    parser.add_argument('--configuration',
                        nargs='?',
                        default='Debug',
                        help='Specify which configuration to build.')
    parser.add_argument('--platform_name',
                        nargs='?',
                        help='Specify which platform to build.')
    parser.add_argument('--only_update_symlink',
                        action='store_true',
                        default=False,
                        help='Only update the symlink')

    options = parser.parse_args()

    # Passed in configuration is going to be based on Xcode configurations which
    # is based on channels, so use Release for all non-Debug configs.
    config = 'Release' if 'Debug' not in options.configuration else 'Debug'
    output_dir = BuildOutputDirectory(config, options.platform_name)
    target_arch = 'arm64' if platform.processor(
    ) == 'arm' or options.platform_name == 'iphoneos' else 'x64'
    target_environment = 'simulator' if (options.platform_name
                                         == 'iphonesimulator') else 'device'

    if options.only_update_symlink:
        # If we're choosing to only update the symlink we should validate
        # that the resulting symlink will point to a valid folder
        if not os.path.exists(output_dir):
            err = inspect.cleandoc(f'''
            Expected out directory for chosen build doesn't exist:

            {output_dir}

            Ensure you run the correct `npm run build` command prior to building
            ''')
            raise Exception(err)
    else:
        BuildCore(config, target_arch, target_environment)
        CallNpm(['npm', 'run', 'ios_pack_js'])
    UpdateSymlink(config, target_arch, target_environment)


def BuildOutputDirectory(config, platform_name):
    directory_name = 'ios_%s' % config
    if platform.processor() == 'arm' or platform_name == 'iphoneos':
        directory_name += '_arm64'
    if platform_name == 'iphonesimulator':
        directory_name += '_simulator'
    return os.path.normpath(os.path.join(src_dir, 'out', directory_name))


def UpdateSymlink(config, target_arch, target_environment):
    """Updates the 'ios_current_link' symlink"""
    cmd_args = [
        'npm', 'run', 'update_symlink', '--', config, '--symlink_dir',
        os.path.join(src_dir, 'out/ios_current_link'), '--target_os', 'ios',
        '--target_arch', target_arch, '--target_environment',
        target_environment
    ]
    CallNpm(cmd_args)


def BuildCore(config, target_arch, target_environment):
    """Generates and builds the BraveCore.framework"""
    cmd_args = [
        'npm', 'run', 'build', '--', config, '--target_os', 'ios',
        '--target_arch', target_arch, '--target_environment',
        target_environment
    ]
    CallNpm(cmd_args)


def CallNpm(cmd):
    retcode = subprocess.call(cmd, cwd=brave_root_dir, stderr=subprocess.STDOUT)
    if retcode:
        raise subprocess.CalledProcessError(retcode, cmd)


if __name__ == '__main__':
    sys.exit(main())
