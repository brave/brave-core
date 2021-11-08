#!/usr/bin/env python

# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

""" This helper is a collection of functions used for signing on MacOS """

import signing.signing  # pylint: disable=import-error, wrong-import-position, unused-import
import signing.model    # pylint: disable=import-error, reimported, wrong-import-position, unused-import
import collections
import os
import subprocess
import sys

from signing import model  # pylint: disable=import-error

# Construct path to signing modules in chrome/installer/mac/signing
signing_path = os.path.realpath(os.path.dirname(os.path.realpath(__file__)))
signing_path = os.path.realpath(os.path.join(
    signing_path, os.pardir, os.pardir, "chrome", "installer", "mac"))
sys.path.append(signing_path)

# Import the entire module to avoid circular dependencies in the functions

brave_channel = os.environ.get('BRAVE_CHANNEL')
sign_widevine_cert = os.environ.get('SIGN_WIDEVINE_CERT')
sign_widevine_key = os.environ.get('SIGN_WIDEVINE_KEY')
sign_widevine_passwd = os.environ.get('SIGN_WIDEVINE_PASSPHRASE')
sig_generator_path = os.path.realpath(
    os.path.dirname(os.path.realpath(__file__)))
sig_generator_path = os.path.realpath(
    os.path.join(sig_generator_path, os.pardir, os.pardir, 'third_party',
                 'widevine', 'scripts', 'signature_generator.py'))


def file_exists(path):
    """ Checks if file with the given path exists """
    return os.path.exists(path)


def run_command(args, **kwargs):
    """ Runs the given executable with the given arguments """
    print('Running command: {}'.format(args)
          )  # pylint: disable=superfluous-parens
    subprocess.check_call(args, **kwargs)


def GenerateBraveWidevineSigFile(paths, config, part):
    """ Generates Widevine .sig file """
    if sign_widevine_key and sign_widevine_passwd and file_exists(sig_generator_path):
        # Framework needs to be signed before generating Widevine signature
        # file. The calling script will re-sign it after Widevine signature
        # file has been added (see signing.py from where this function is
        # called).
        from signing.signing import sign_part  # pylint: disable=import-error
        sign_part(paths, config, part)
        # Generate signature file
        chrome_framework_name = config.app_product + ' Framework'
        chrome_framework_version_path = os.path.join(paths.work, part.path,
                                                     'Versions', config.version)
        sig_source_file = os.path.join(
            chrome_framework_version_path, chrome_framework_name)
        sig_target_file = os.path.join(chrome_framework_version_path, 'Resources',
                                       chrome_framework_name + '.sig')
        assert file_exists(
            sig_source_file), 'Wrong source path for sig generation'

        command = ['python', sig_generator_path, '--input_file', sig_source_file,
                   '--output_file', sig_target_file, '--flags', '1',
                   '--certificate', sign_widevine_cert,
                   '--private_key', sign_widevine_key,
                   '--private_key_passphrase', sign_widevine_passwd]

        run_command(command)
        assert file_exists(sig_target_file), 'No sig file'


def AddBravePartsForSigning(parts, config):
    """ Inserts Brave specific parts that need to be signed """
    parts = collections.OrderedDict(parts)
    from signing.model import CodeSignedProduct, VerifyOptions, CodeSignOptions  # pylint: disable=import-error

    development = True if config.provisioning_profile_basename is None else False

    full_hardened_runtime_options = (
        CodeSignOptions.HARDENED_RUNTIME + CodeSignOptions.RESTRICT +
        CodeSignOptions.LIBRARY_VALIDATION + CodeSignOptions.KILL)

    # Add Sparkle
    if not development:
        # Add Sparkle binaries
        parts['sparkle-framework-fileop'] = CodeSignedProduct(
            '{0.framework_dir}/Versions/{1.version}/Frameworks/Sparkle.framework/Versions/A/Resources/Autoupdate.app/Contents/MacOS/fileop'  # pylint: disable=line-too-long
            .format(config, config),
            'fileop',
            verify_options=VerifyOptions.DEEP + VerifyOptions.NO_STRICT)
        parts['sparkle-framework-fileop'].options = full_hardened_runtime_options

        parts['sparkle-framework-autoupdate'] = CodeSignedProduct(
            '{0.framework_dir}/Versions/{1.version}/Frameworks/Sparkle.framework/Versions/A/Resources/Autoupdate.app/Contents/MacOS/Autoupdate'  # pylint: disable=line-too-long
            .format(config, config),
            'org.sparkle-project.Sparkle.Autoupdate',
            verify_options=VerifyOptions.DEEP + VerifyOptions.NO_STRICT)
        parts['sparkle-framework-autoupdate'].options = full_hardened_runtime_options

        parts['sparkle-framework'] = CodeSignedProduct(
            '{.framework_dir}/Frameworks/Sparkle.framework'.format(config),
            'org.sparkle-project.Sparkle',
            verify_options=VerifyOptions.DEEP + VerifyOptions.NO_STRICT)
        parts['sparkle-framework'].options = full_hardened_runtime_options

    # Overwrite to avoid TeamID mismatch with widevine dylib.
    parts['helper-app'].entitlements = 'helper-entitlements.plist'
    parts['helper-app'].options = (CodeSignOptions.RESTRICT
                                   + CodeSignOptions.KILL
                                   + CodeSignOptions.HARDENED_RUNTIME)

    return parts


def GetBraveSigningConfig(config_class, mac_provisioning_profile=None):
    """ Creates Brave specific config used for signing """
    class ConfigNonChromeBranded(config_class):  # pylint: disable=too-few-public-methods
        """ Config that overrides is_chrome_branded """

        @staticmethod
        def is_chrome_branded():
            """ Not chrome branded """
            return False

        @property
        def distributions(self):
            """ Brave distribution """
            return [model.Distribution(channel=brave_channel)]

    config_class = ConfigNonChromeBranded

    if mac_provisioning_profile is not None:
        provisioning_profile = mac_provisioning_profile
    else:
        # Retrieve provisioning profile exported by build/mac/sign_app.sh
        provisioning_profile = os.environ['MAC_PROVISIONING_PROFILE']

    # If provisioning_profile is not set, then it's development config.
    if not provisioning_profile:
        return config_class

    class ProvisioningProfileCodeSignConfig(config_class):
        """ Config with provisioning profile """

        @property
        def provisioning_profile_basename(self):
            """ Provisioning profile base name """
            return os.path.splitext(os.path.basename(
                provisioning_profile))[0]

        @property
        def run_spctl_assess(self):
            """ Run spctl check """
            return True

    return ProvisioningProfileCodeSignConfig
