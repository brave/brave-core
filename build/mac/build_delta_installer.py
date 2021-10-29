#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import optparse
import os
import re
import subprocess
import sys


def unmount(volume_path):
    print('-> unmount ' + volume_path)
    command = ['hdiutil', 'detach', volume_path]
    try:
        subprocess.check_call(command)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def mount_dmg(dmg_path, mount_point):
    print('-> mounting ' + dmg_path + ' to ' + mount_point)

    command = ['hdiutil', 'attach', dmg_path, '-mountpoint', mount_point]
    try:
        output = subprocess.check_output(command)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e


def Main(argv):
    parser = optparse.OptionParser('%prog [options]')
    parser.add_option('--binary-delta', dest='binary_delta_path', action='store',
        type='string', default=None, help='The path of BinaryDelta binary.')
    parser.add_option('--root-out-dir', dest='root_out_dir_path', action='store',
        type='string', default=None, help='The path of root output dir.')
    parser.add_option('--old-dmg', dest='old_dmg_path', action='store',
        type='string', default=None, help='The path of old dmg.')
    parser.add_option('--new-dmg', dest='new_dmg_path', action='store',
        type='string', default=None, help='The path of new dmg.')
    parser.add_option('--delta-output', dest='delta_output_path', action='store',
        type='string', default=None, help='The path of generated delta file.')
    (options, args) = parser.parse_args(argv)

    if len(args) > 0:
        print >> sys.stderr, parser.get_usage()
        return 1

    old_dmg_mount_point = os.path.join(options.root_out_dir_path, 'old_dmg_mount_for_delta')
    mount_dmg(options.old_dmg_path, old_dmg_mount_point)
    old_app_path = os.path.join(old_dmg_mount_point, os.path.splitext(os.path.basename(options.old_dmg_path))[0] + '.app')

    new_dmg_mount_point = os.path.join(options.root_out_dir_path, 'new_dmg_mount_for_delta')
    mount_dmg(options.new_dmg_path, new_dmg_mount_point)
    new_app_path = os.path.join(new_dmg_mount_point, os.path.splitext(os.path.basename(options.new_dmg_path))[0] + '.app')

    # generate delta file
    print('-> generate delta file from ' + old_app_path + ' and ' + new_app_path)
    command = [options.binary_delta_path, 'create', old_app_path, new_app_path, options.delta_output_path]
    try:
        subprocess.check_call(command)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e

    unmount(old_dmg_mount_point)
    unmount(new_dmg_mount_point)

    return 0


if __name__ == '__main__':
    sys.exit(Main(sys.argv[1:]))
