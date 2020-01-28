#!/usr/bin/env python

# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import os
import shutil

CHROME_DIR = "Chrome-bin"


def SignAndCopyPreSignedBinaries(skip_signing, output_dir, staging_dir, current_version):
    if not skip_signing:
        from sign_binaries import sign_binaries
        sign_binaries(staging_dir)
        """Copies already signed three binaries - brave.exe and chrome.dll
        These files are signed during the build phase to create widevine sig files.
        """
        src_dir = os.path.join(output_dir, 'signed_binaries')
        chrome_dir = os.path.join(staging_dir, CHROME_DIR)
        version_dir = os.path.join(chrome_dir, current_version)
        shutil.copy(os.path.join(src_dir, 'brave.exe'), chrome_dir)
        shutil.copy(os.path.join(src_dir, 'chrome.dll'), version_dir)


def BraveCopyAllFilesToStagingDir(config, staging_dir, g_archive_inputs):
    current_dir = os.path.realpath(os.path.dirname(os.path.realpath(__file__)))

    brave_extension_locales_src_dir_path = os.path.realpath(
        os.path.join(current_dir, os.pardir, 'components',
                     'brave_extension', 'extension', 'brave_extension', '_locales'))
    CopyExtensionLocalization('brave_extension', brave_extension_locales_src_dir_path,
                              config, staging_dir, g_archive_inputs)

    brave_rewards_locales_src_dir_path = os.path.realpath(
        os.path.join(current_dir, os.pardir, 'components',
                     'brave_rewards', 'resources', 'extension', 'brave_rewards', '_locales'))
    CopyExtensionLocalization('brave_rewards', brave_rewards_locales_src_dir_path,
                              config, staging_dir, g_archive_inputs)


def CopyExtensionLocalization(extension_name, locales_src_dir_path, config, staging_dir, g_archive_inputs):
    """Copies extension localization files from locales_src_dir_path to
    \\<out_gen_dir>\\chrome\\installer\\mini_installer\\mini_installer\\temp_installer_archive
        \\Chrome-bin\\<version>\\resources\\extension_name\\_locales
    """
    locales_dest_path = staging_dir
    locales_dest_path = os.path.join(locales_dest_path, config.get('GENERAL', 'brave_resources.pak'),
                                     'resources', extension_name, '_locales')
    locales_dest_path = os.path.realpath(locales_dest_path)
    try:
        shutil.rmtree(locales_dest_path)
    except Exception as e:
        pass
    shutil.copytree(locales_src_dir_path, locales_dest_path)
    # Files are copied, but we need to inform g_archive_inputs about that
    for root, dirs, files in os.walk(locales_dest_path):
        for name in files:
            rel_dir = os.path.relpath(root, locales_dest_path)
            rel_file = os.path.join(rel_dir, name)
            candidate = os.path.join('.', 'resources', extension_name, '_locales', rel_file)
            g_archive_inputs.append(candidate)
