#!/usr/bin/env python
# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

# This script is a modified version of chrome/installer/mac/sign_chrome.py
# that allows to configure provisioning profile on the fly and also adds
# sparkle to optional parts for signing.

import argparse
import os.path
import shutil
import sys

sys.path.append(os.path.dirname(__file__))

from signing import config, model, pipeline


def create_config(identity, keychain, development, provisioning_profile):
    """Creates the |model.CodeSignConfig| for the signing operations.

    If |development| is True, the config will be modified to not require
    restricted internal assets, nor will the products be required to match
    specific certificate hashes.

    Args:
        identity: The code signing identity to use.
        keychain: Optional path to the keychain file, in which |identity|
            will be searched for.
        development: Boolean indicating whether or not to modify the chosen
            config for development testing.
        provisioning_profile: The path to provisioning profile file.

    Returns:
        An instance of |model.CodeSignConfig|.
    """
    config_class = config.CodeSignConfig

    if development:

        class DevelopmentCodeSignConfig(config_class):

            @property
            def codesign_requirements_basic(self):
                return ''

            @property
            def provisioning_profile_basename(self):
                return None

            @property
            def run_spctl_assess(self):
                return False

        config_class = DevelopmentCodeSignConfig

    else:

        class ProvisioningProfileCodeSignConfig(config_class):

            @property
            def provisioning_profile_basename(self):
                return os.path.splitext(
                    os.path.basename(provisioning_profile))[0]

            @property
            def optional_parts(self):
                return set(('libwidevinecdm.dylib',
                            'sparkle-framework',))

            @property
            def run_spctl_assess(self):
                return True

        config_class = ProvisioningProfileCodeSignConfig

    return config_class(identity, keychain)


def main():
    parser = argparse.ArgumentParser(
        description='Code sign and package Brave for channel distribution.')
    parser.add_argument(
        '--keychain', help='The keychain to load the identity from.')
    parser.add_argument(
        '--identity', required=True, help='The identity to sign with.')
    parser.add_argument('--development', action='store_true',
            help='The specified identity is for development. ' \
                 'Certain codesign requirements will be omitted.')
    parser.add_argument('--input', required=True,
            help='Path to the input directory. The input directory should ' \
                    'contain the products to sign, as well as the Packaging ' \
                    'directory.')
    parser.add_argument('--output', required=True,
            help='Path to the output directory. The signed DMG products and ' \
                    'installer tools will be placed here.')
    parser.add_argument(
        '--no-dmg',
        action='store_true',
        help='Only sign Brave and do not package the bundle into a DMG.')
    parser.add_argument('--provisioning-profile',
                        help='The path to the provisioning profile file')
    args = parser.parse_args()

    config = create_config(args.identity, args.keychain, args.development,
                           args.provisioning_profile)
    paths = model.Paths(args.input, args.output, None)

    if not os.path.exists(paths.output):
        os.mkdir(paths.output)
    else:
        if args.no_dmg:
            dest_dir = os.path.join(paths.output, config.dmg_basename)
            if os.path.exists(dest_dir):
                shutil.rmtree(dest_dir)

    pipeline.sign_all(paths, config, package_dmg=not args.no_dmg)


if __name__ == '__main__':
    main()
