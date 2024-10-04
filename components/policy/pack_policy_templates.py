#!/usr/bin/env python3
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Script that prepares a Brave-specific version of the `policy_templates.zip`
# file that folks expect for administering Brave via Windows group policy.
#
# For more info, see:
# https://support.brave.com/hc/en-us/articles/360039248271-Group-Policy
# and
# https://github.com/brave/brave-browser/issues/26502
#
# NOTE: There are assets on other platforms but we don't currently use them.
#
# - macOS should have two assets:
#     - $root_out_dir/mac/app-Manifest.plist
#     - $root_out_dir/mac/jamf.json
#
# - Linux has one asset:
#     - $root_out_dir/linux/examples/chrome.json
#
# For more info, see:
# https://source.chromium.org/chromium/chromium/src/+/main:components/policy/resources/policy_templates.gni # pylint: disable=line-too-long
"""
Create a Zip file of Windows Group Policy templates similar to Chrome's.
"""

import argparse
import os

from os.path import join, exists, relpath
from tempfile import TemporaryDirectory
from zipfile import ZipFile, ZIP_DEFLATED

def main():
    chrome_policy_zip, dest_zip = _get_args()
    _pack_policy_templates(chrome_policy_zip, dest_zip)

def _get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('chrome_policy_zip',
                        help="Path to Chrome's policy_templates.zip")
    parser.add_argument('dest_zip', help="Path to the Zip file to be created")
    args = parser.parse_args()
    return args.chrome_policy_zip, args.dest_zip

def _pack_policy_templates(chrome_policy_zip, dest_zip):
    with TemporaryDirectory() as tmp_dir:
        with ZipFile(chrome_policy_zip) as src_zip:
            src_zip.extract('VERSION', tmp_dir)
            namelist = src_zip.namelist()
            for dir_ in ('windows/adm/', 'windows/admx/', 'windows/examples/'):
                src_zip.extractall(tmp_dir,
                                   (n for n in namelist if n.startswith(dir_)))

        # Some sanity checks:
        assert exists(join(tmp_dir, 'windows/adm/en-US/chrome.adm'))
        assert exists(join(tmp_dir, 'windows/admx/chrome.admx'))
        assert exists(join(tmp_dir, 'windows/admx/en-US/chrome.adml'))

        with ZipFile(dest_zip, 'w', ZIP_DEFLATED) as dest_zipfile:
            for dirpath, _, filenames in os.walk(tmp_dir):
                for filename in filenames:
                    filepath = join(dirpath, filename)
                    arcname = relpath(filepath,
                                      tmp_dir).replace('chrome', 'brave')
                    dest_zipfile.write(filepath, arcname=arcname)

if __name__ == '__main__':
    main()
