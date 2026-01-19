#!/usr/bin/env python

# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

""" This helper is a collection of functions used for signing on MacOS """

import collections
import os
import re

from lib.widevine import can_generate_sig_file, generate_sig_file


def GenerateBraveWidevineSigFile(paths, config, part):
    """ Generates Widevine .sig file """
    if can_generate_sig_file():
        # Framework needs to be signed before generating Widevine signature
        # file. The calling script will re-sign it after Widevine signature
        # file has been added (see signing.py from where this function is
        # called).
        from signing.signing import sign_part  # pylint: disable=import-error, import-outside-toplevel
        sign_part(paths, config, part)
        # Generate signature file
        chrome_framework_name = config.app_product + ' Framework'
        chrome_framework_version_path = os.path.join(paths.work, part.path,
                                                     'Versions', config.version)
        sig_source_file = os.path.join(
            chrome_framework_version_path, chrome_framework_name)
        sig_target_file = os.path.join(chrome_framework_version_path,
                                       'Resources',
                                       chrome_framework_name + '.sig')

        generate_sig_file(sig_source_file, sig_target_file, '1')


def BraveModifyPartsForSigning(parts, config):
    """ Inserts Brave specific parts that need to be signed """
    parts = collections.OrderedDict(parts)
    from signing.model import CodeSignedProduct, VerifyOptions, CodeSignOptions  # pylint: disable=import-error, import-outside-toplevel

    # We inherit this from upstream because our config sets is_chrome_branded()
    # to True.
    del parts['liboptimization_guide_internal.dylib']

    development = (config.provisioning_profile_basename is None)

    full_hardened_runtime_options = (
        CodeSignOptions.HARDENED_RUNTIME | CodeSignOptions.RESTRICT
        | CodeSignOptions.LIBRARY_VALIDATION | CodeSignOptions.KILL)

    # Add Sparkle
    if not development:
        # Add Sparkle binaries
        parts['sparkle-framework-fileop'] = CodeSignedProduct(
            '{0.framework_dir}/Versions/{0.version}/Frameworks/Sparkle.framework/Versions/A/Resources/Autoupdate.app/Contents/MacOS/fileop'  # pylint: disable=line-too-long
            .format(config),
            'fileop',
            verify_options=VerifyOptions.DEEP | VerifyOptions.NO_STRICT)
        parts['sparkle-framework-fileop'].options = (
            full_hardened_runtime_options
        )

        parts['sparkle-framework-autoupdate'] = CodeSignedProduct(
            '{0.framework_dir}/Versions/{0.version}/Frameworks/Sparkle.framework/Versions/A/Resources/Autoupdate.app/Contents/MacOS/Autoupdate'  # pylint: disable=line-too-long
            .format(config),
            'org.sparkle-project.Sparkle.Autoupdate',
            verify_options=VerifyOptions.DEEP | VerifyOptions.NO_STRICT)
        parts['sparkle-framework-autoupdate'].options = (
            full_hardened_runtime_options
        )

        parts['sparkle-framework'] = CodeSignedProduct(
            '{.framework_dir}/Frameworks/Sparkle.framework'.format(config),
            'org.sparkle-project.Sparkle',
            verify_options=VerifyOptions.DEEP | VerifyOptions.NO_STRICT)
        parts['sparkle-framework'].options = full_hardened_runtime_options

    # Overwrite to avoid TeamID mismatch with widevine dylib.
    parts['helper-app'].entitlements = 'helper-entitlements.plist'
    parts['helper-app'].options = (CodeSignOptions.RESTRICT
                                   | CodeSignOptions.KILL
                                   | CodeSignOptions.HARDENED_RUNTIME)

    if config.enable_updater:
        # The privileged helper is com.brave.Browser.UpdaterPrivilegedHelper.
        # But the value here is
        # com.brave.Browser.<channel>.UpdaterPrivilegedHelper. This is because
        # our current branding logic treats each channel as a separate product.
        # We should instead use upstream's channel_customize mechanism.
        # See https://github.com/brave/brave-browser/issues/39347.
        privileged_helper = parts['privileged-helper']
        channel_re = r'com\.brave\.Browser(\.origin)?(.*)\.UpdaterPrivilegedHelper'
        replacement = r'com.brave.Browser\1.UpdaterPrivilegedHelper'
        privileged_helper.path = re.sub(channel_re, replacement,
                                        privileged_helper.path)
        privileged_helper.identifier = re.sub(channel_re, replacement,
                                              privileged_helper.identifier)

    return parts
