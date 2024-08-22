#!/usr/bin/env python3

# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from argparse import ArgumentParser
from os import mkdir, makedirs, symlink
from os.path import join, isdir, basename, dirname, exists
from pathlib import Path
from shutil import copy, copytree
from subprocess import run, PIPE, STDOUT, CalledProcessError
from tempfile import TemporaryDirectory

import re

DMG_TOOL_PATH = join(dirname(dirname(dirname(dirname(__file__)))),
                     'third_party', 'libdmg-hfsplus', 'build', 'dmg', 'dmg')


def main():
    assert exists(DMG_TOOL_PATH), DMG_TOOL_PATH + \
        ' does not exist. Please make sure that checkout_dmg_tool is set to' \
        ' True in the "custom_vars" section of the "src/brave" section of' \
        ' your .gclient file, then `npm run sync` (without `--target_os=mac`)' \
        ' to download and build it.'
    args = parse_args()
    assert args.source == '/var/empty', 'Only --source=/var/empty is supported.'
    to_copy = list(map(parse_copy_arg, args.copy))
    required_size = sum(get_size(tpl[0]) for tpl in to_copy)
    safe_size_mb = required_size // 2**20 + 25
    with TemporaryDirectory(dir=args.tempdir) as tmp:
        image = join(tmp, 'image.hfs')
        run_with_output([
            'dd', 'if=/dev/zero', f'of={image}', 'bs=1M',
            f'count={safe_size_mb}'
        ])
        run_with_output(['/usr/sbin/mkfs.hfsplus', '-v', args.volname, image])
        output = run_with_output(['udisksctl', 'loop-setup', '-f', image])
        match = re.search(r'/dev/loop\d+', output)
        assert match, output
        loop_device = match.group(0)
        try:
            output = run_with_output(['udisksctl', 'mount', '-b', loop_device])
            try:
                prefix = f'Mounted {loop_device} at '
                assert output.startswith(prefix), prefix
                mount_path = output[len(prefix):].rstrip('\n')
                for d in args.mkdir or []:
                    makedirs(join(mount_path, d))
                for src, dst_rel in to_copy:
                    dst = join(mount_path, dst_rel)
                    if isdir(src):
                        copytree(src, dst, symlinks=True)
                    else:
                        copy(src, dst)
                for arg in args.symlink or []:
                    src, dst_rel = parse_copy_arg(arg)
                    symlink(src, join(mount_path, dst_rel))
            finally:
                run_with_output(['udisksctl', 'unmount', '-b', loop_device])
        finally:
            run_with_output(['udisksctl', 'loop-delete', '-b', loop_device])
        run_with_output([DMG_TOOL_PATH, image, args.target])


def parse_copy_arg(arg):
    try:
        src, dst_rel_leading_slash = arg.split(':', 1)
    except ValueError:
        return arg, basename(arg)
    else:
        assert dst_rel_leading_slash.startswith('/'), dst_rel_leading_slash
        return src, dst_rel_leading_slash[1:]


def get_size(dir_or_file):
    path = Path(dir_or_file)
    if path.is_dir():
        return sum(p.lstat().st_size for p in Path(dir_or_file).rglob('*'))
    return path.lstat().st_size


def run_with_output(args):
    # pylint: disable=subprocess-run-check
    cp = run(args, stdout=PIPE, stderr=STDOUT, text=True)
    if cp.returncode:
        raise RuntimeError(
            f'Command {args} returned error {cp.returncode}. Output:\n\n'
            f'{cp.stdout}')
    return cp.stdout


def parse_args():
    parser = ArgumentParser(description="A Linux implementation of pkg-dmg")

    parser.add_argument("--source", default="/var/empty")
    parser.add_argument("--target", required=True)
    parser.add_argument("--format", required=True)
    parser.add_argument("--verbosity", type=int, help='Currently ignored.')
    parser.add_argument("--volname", required=True)
    parser.add_argument("--copy", action='append')
    parser.add_argument("--tempdir")
    parser.add_argument("--mkdir", action="append")
    parser.add_argument("--icon", help='Currently ignored.')
    parser.add_argument("--symlink", action="append", help='Currently ignored.')

    return parser.parse_args()


if __name__ == '__main__':
    main()
