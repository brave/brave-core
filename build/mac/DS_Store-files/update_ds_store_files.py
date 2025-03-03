# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# This script updates the DS_Store files in this directory. (Cf. README.md.)
# It is not used in the build process. The motivation for having it here is that
# DS_Store is a proprietary format that is notoriously hard to edit. This script
# gives us a way to edit the files in a more convenient way, should we need to
# do so in the future.

from ds_store import DSStore
from mac_alias import Alias
from os import makedirs, symlink
from os.path import join, basename
from shutil import copyfile
from subprocess import check_call
from tempfile import TemporaryDirectory
from uuid import uuid4


def main():
    for channel in ('', 'Nightly', 'Beta', 'Release'):
        app_name = 'Brave Browser' + (f' {channel}' if channel else '')
        ds_store = 'DS_Store' + (f'.{channel.lower()}' if channel else '')
        create_ds_store(app_name, '../dmg-background.png', f'{ds_store}')


def create_ds_store(app_name, bg_file, ds_store_path):
    with TemporaryDirectory() as temp_dir:
        # We create and mount a temporary DMG because this creates an HFS+ file
        # system, which is required for at least Alias.for_file(...) below. See
        # github.com/dmgbuild/mac_alias/issues/22#issuecomment-1350790003.
        dmg_path = join(temp_dir, f'{app_name}.dmg')
        with TemporaryDirectory() as empty:
            check_call([
                'hdiutil', 'create', '-fs', 'HFS+', '-format', 'UDRW',
                '-sectors',
                str(8 * 1024 * 1024), '-volname', app_name, '-srcfolder',
                empty, '-quiet', dmg_path
            ])
        mount_path = join('/Volumes', basename(__file__) + str(uuid4()))
        check_call([
            'hdiutil', 'attach', '-mountpoint', mount_path, '-noautoopen',
            '-quiet', dmg_path
        ])
        try:
            symlink('/Applications', join(mount_path, 'Applications'))
            background_dir = join(mount_path, '.background')
            makedirs(background_dir)
            copyfile(bg_file, join(background_dir, 'background.png'))
            ds_store_tmp = join(mount_path, '.DS_Store')
            with DSStore.open(ds_store_tmp, 'w+') as d:
                d[app_name + '.app']['Iloc'] = (150, 155)
                d[' ']['Iloc'] = (430, 150)
                d['.']['bwsp'] = {
                    'ContainerShowSidebar': False,
                    'ShowPathbar': False,
                    'ShowSidebar': False,
                    'ShowStatusBar': False,
                    'ShowTabView': False,
                    'ShowToolbar': False,
                    'WindowBounds': '{{548, 815}, {602, 350}}'
                }
                bg_alias = Alias.for_file(
                    join(background_dir, 'background.png'))
                d['.']['icvp'] = {
                    'arrangeBy': 'none',
                    'backgroundColorBlue': 1.0,
                    'backgroundColorGreen': 1.0,
                    'backgroundColorRed': 1.0,
                    'backgroundType': 2,
                    'backgroundImageAlias': bg_alias.to_bytes(),
                    'gridOffsetX': 0.0,
                    'gridOffsetY': 0.0,
                    'gridSpacing': 100.0,
                    'iconSize': 128.0,
                    'labelOnBottom': True,
                    'showIconPreview': True,
                    'showItemInfo': False,
                    'textSize': 16.0,
                    'viewOptionsVersion': 1
                }
            copyfile(ds_store_tmp, ds_store_path)
        finally:
            check_call(['hdiutil', 'detach', '-quiet', mount_path])


if __name__ == '__main__':
    main()
