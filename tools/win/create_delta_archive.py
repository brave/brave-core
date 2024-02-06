# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from argparse import ArgumentParser
from os import getcwd
from os.path import basename, dirname
from subprocess import check_call

from brave_chromium_utils import sys_path

with sys_path('//chrome/tools/build/win'):
    import create_installer_archive as upstream_impl


def main():
    args = parse_args()
    create_patch(args.zucchini, args.old_chrome_7z, args.new_chrome_7z,
                 args.chrome_7z_patch, args.chrome_7z_patch_packed)
    create_patch(args.zucchini, args.old_setup_exe, args.new_setup_exe,
                 args.setup_exe_patch, args.setup_exe_patch_packed)
    write_rc_file(args.chrome_7z_patch_packed, args.setup_exe_patch_packed,
                  args.rc_file, args.is_component_build)


def parse_args():
    parser = ArgumentParser(
        description=
        'Compute a Windows delta update. The resulting files can be used to '
        'create a mini_installer that applies the delta.')
    parser.add_argument('zucchini',
                        help=('the binary computing the delta, usually '
                              'zucchini64.exe.'))
    parser.add_argument('old_chrome_7z',
                        help='path to chrome.7z for the previous version.')
    parser.add_argument('new_chrome_7z',
                        help='path to chrome.7z for this  version.')
    parser.add_argument('old_setup_exe',
                        help='path to setup.exe for the previous version.')
    parser.add_argument('new_setup_exe',
                        help='path to setup.exe for this version.')
    parser.add_argument('chrome_7z_patch',
                        help=('output path for the .diff file for chrome.7z. '
                              'Eg. chrome_patch.diff'))
    parser.add_argument('chrome_7z_patch_packed',
                        help=('output path for the packed .diff file for '
                              'chrome.7z. Eg. chrome_patch.packed.7z.'))
    parser.add_argument('setup_exe_patch',
                        help=('output path for the .diff file for setup.exe. '
                              'Eg. setup_patch.diff.'))
    parser.add_argument('setup_exe_patch_packed',
                        help=('output path for the packed .diff file for '
                              'setup.exe. Eg. setup_patch.packed.7z.'))
    parser.add_argument('rc_file',
                        help=('output path for the .rc file that makes the '
                              'packed patch files accessible to '
                              'mini_installer.'))
    parser.add_argument('--is_component_build', action='store_true')
    return parser.parse_args()


def create_patch(zucchini, old_file, new_file, patch_file, packed_patch_file):
    check_call([zucchini, '-gen', old_file, new_file, patch_file])
    compress_with_7z(patch_file, packed_patch_file)


def write_rc_file(chrome_7z_patch, setup_exe_patch, rc_file,
                  is_component_build):
    if is_component_build:
        raise NotImplementedError(
            "This Python script doesn't yet suport component builds.")
    output_dir = dirname(chrome_7z_patch)
    assert dirname(setup_exe_patch) == output_dir
    # pylint: disable=no-member
    upstream_impl.CreateResourceInputFile(output_dir, 'DIFF',
                                          basename(chrome_7z_patch),
                                          basename(setup_exe_patch), rc_file,
                                          False, None, None)


def compress_with_7z(src_file, dst_file):
    # Use upstream's implementation because it heavily optimizes for size.
    # We assume that we are in the build directory.
    build_dir = getcwd()
    # pylint: disable=no-member
    upstream_impl.CompressUsingLZMA(build_dir,
                                    dst_file,
                                    src_file,
                                    verbose=False,
                                    fast=False)


if __name__ == '__main__':
    main()
