#!/usr/bin/env python

# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import argparse
import os
import subprocess
import sys

"""
The sign_chrome.py script does notarization of the app, but we don't use it to
create our .dmg and .pkg files. Instead we do our own signing and packaging,
but must perform notarization after those processes are complete. This script
performs notarizing and stapling of those files.
"""

# Our CWD is the packaging directory (i.e.
# src/out/Release/Brave_Browser_CHANNEL_Packaging), the signing directory is
# relative to that
packaging_signing_path = os.path.realpath(os.path.dirname(os.path.realpath(__file__)))
sys.path.append(packaging_signing_path)

# Import the entire module to avoid circular dependencies in the functions
from signing import chromium_config, config, commands, model, notarize, pipeline, signing  # noqa: E402
from signing_helper import GetBraveSigningConfig

def run_command(args, **kwargs):
    print('Running command: {}'.format(args))
    subprocess.check_call(args, **kwargs)


def create_config(config_args, development, mac_provisioning_profile):
    """Creates the |model.CodeSignConfig| for the signing operations.

    If |development| is True, the config will be modified to not require
    restricted internal assets, nor will the products be required to match
    specific certificate hashes.

    Args:
        config_args: List of args to expand to the config class's constructor.
        development: Boolean indicating whether or not to modify the chosen
            config for development testing.

    Returns:
        An instance of |model.CodeSignConfig|.
    """
    config_class = chromium_config.ChromiumCodeSignConfig

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

    config_class = GetBraveSigningConfig(config_class, development, mac_provisioning_profile)
    return config_class(*config_args)


def NotarizeBraveDmgPkg(paths, config, dmg, pkg, outdir, signed):
    """
    Notarize Brave .dmg and .pkg files.
    """
    uuids_to_path_map = {}
    for dist in config.distributions:
        dist_config = dist.to_config(config)
        uuid = notarize.submit(dmg, dist_config)
        uuids_to_path_map[uuid] = dmg
        uuid1 = notarize.submit(pkg, dist_config)
        uuids_to_path_map[uuid1] = pkg
        for result in notarize.wait_for_results(
                uuids_to_path_map.keys(), config):
            brave_path = uuids_to_path_map[result]
            notarize.staple(brave_path)
    for item in uuids_to_path_map.values():
        commands.copy_files(os.path.join(signed, item), outdir)
    return 0


def main():

    args = parse_args()

    if args.mac_provisioning_profile and args.development is not True:
        config = create_config((args.identity, None, args.notary_user,
                               args.notary_password, args.notary_asc_provider),
                               args.development, args.mac_provisioning_profile)
    else:
        config = create_config((args.identity, None, args.notary_user,
                               args.notary_password, args.notary_asc_provider),
                               args.development)
    paths = model.Paths(args.pkgdir, args.outdir, None)

    rc = NotarizeBraveDmgPkg(paths, config, args.dmg, args.pkg, args.outdir, args.signed)
    return rc


def parse_args():
    parser = argparse.ArgumentParser(description='Notarize Mac DMG and PKG')
    parser.add_argument(
        '--identity', required=True, help='The identity to sign with.')
    parser.add_argument(
        '--notary-user',
        help='The username used to authenticate to the Apple notary service.')
    parser.add_argument(
        '--notary-password',
        help='The password or password reference (e.g. @keychain, see '
        '`xcrun altool -h`) used to authenticate to the Apple notary service.')
    parser.add_argument(
        '--notary-asc-provider',
        help='The ASC provider string to be used as the `--asc-provider` '
        'argument to `xcrun altool`, to be used when --notary-user is '
        'associated with multiple Apple developer teams. See `xcrun altool -h. '
        'Run `iTMSTransporter -m provider -account_type itunes_connect -v off '
        '-u USERNAME -p PASSWORD` to list valid providers.')
    parser.add_argument(
        '--development',
        action='store_true',
        help='The specified identity is for development. Certain codesign '
        'requirements will be omitted.')
    parser.add_argument('-d', '--dmg', help='Path to the dmg to notarize',
                        required=True)
    parser.add_argument('-o', '--outdir', help='Output directory',
                        required=True)
    parser.add_argument('--pkgdir', help='Packaging directory',
                        required=True)
    parser.add_argument('-s', '--signed', help='Directory with signed DMG and PKG',
                        required=True)
    parser.add_argument('-p', '--pkg', help='Path to the pkg to notarize',
                        required=True)
    parser.add_argument('--mac_provisioning_profile', help='Provisioning profile(optional)')
    return parser.parse_args()


if __name__ == '__main__':
    import sys
    sys.exit(main())
