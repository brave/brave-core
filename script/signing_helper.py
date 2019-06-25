#!/usr/bin/env python

# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import os
import subprocess
import sys

sign_widevine_cert = os.environ.get('SIGN_WIDEVINE_CERT')
sign_widevine_key = os.environ.get('SIGN_WIDEVINE_KEY')
sign_widevine_passwd = os.environ.get('SIGN_WIDEVINE_PASSPHRASE')
sig_generator_path = os.path.realpath(os.path.dirname(os.path.realpath(__file__)))
sig_generator_path = os.path.realpath(
    os.path.join(sig_generator_path, os.pardir, os.pardir, 'third_party',
                 'widevine', 'scripts', 'signature_generator.py'))


def file_exists(path):
    return os.path.exists(path)


def run_command(args, **kwargs):
    print('Running command: {}'.format(args))
    subprocess.check_call(args, **kwargs)


def GenerateWidevineSigFile(paths, config, part):
    if sign_widevine_key and sign_widevine_key and sign_widevine_passwd and file_exists(sig_generator_path):
        chrome_framework_name = config.app_product + ' Framework'
        chrome_framework_version_path = os.path.join(paths.work, part.path, 'Versions', config.version)
        sig_source_file = os.path.join(chrome_framework_version_path, chrome_framework_name)
        sig_target_file = os.path.join(chrome_framework_version_path, 'Resources', chrome_framework_name + '.sig')
        assert file_exists(sig_source_file), 'Wrong source path for sig generation'

        command = ['python', sig_generator_path, '--input_file', sig_source_file,
                   '--output_file', sig_target_file, '--flags', '1',
                   '--certificate', sign_widevine_cert,
                   '--private_key', sign_widevine_key,
                   '--private_key_passphrase', sign_widevine_passwd]

        run_command(command)
        assert file_exists(sig_target_file), 'No sig file'


def GetBraveLibraries():
    return ('libchallenge_bypass_ristretto.dylib', 'libadblock.dylib',)
