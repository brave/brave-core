#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import argparse
import os
import sys

from lib.l10n.crowdin.common import should_use_crowdin_for_file
from lib.l10n.crowdin.push import (
    upload_grd_translations_to_crowdin, upload_json_translations_to_crowdin,
    upload_source_file_to_crowdin, upload_translation_strings_xml_for_grd,
    check_source_grd_strings_parity_with_crowdin)
from lib.l10n.grd_utils import (get_grd_languages, get_original_grd,
                                get_override_file_path)


BRAVE_SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SOURCE_ROOT = os.path.dirname(BRAVE_SOURCE_ROOT)


def parse_args():
    parser = argparse.ArgumentParser(description='Push strings to Crowdin')
    parser.add_argument('--source_string_path', nargs=1, required=True)
    parser.add_argument('--channel',
                        choices=['Release', 'Beta', 'Nightly'],
                        nargs='?',
                        default='Release',
                        const='Release')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--with_translations',
                       dest='with_translations',
                       action='store_true',
                       help='Uploads translations from the local .xtb and ' \
                            '.json files to Crowdin. WARNING: This will ' \
                            'overwrite the service translations with the ' \
                            'local values')
    group.add_argument('--with_missing_translations',
                       dest='with_missing_translations', action='store_true',
                       help='Uploads translations from the local .xtb and ' \
                            '.json files to Crowdin, but only for strings ' \
                            ' that are not translated in Crowdin.')
    return parser.parse_args()


def main():
    args = parse_args()
    channel = args.channel
    print(f'[push-l10n] Channel: {channel}')

    source_string_path = os.path.join(BRAVE_SOURCE_ROOT,
                                      args.source_string_path[0])
    filename = os.path.basename(source_string_path).split('.')[0]

    should_use_service_for_file = should_use_crowdin_for_file(
        source_string_path, filename)

    if not should_use_service_for_file:
        override_string_path = get_override_file_path(source_string_path)
        filename = os.path.basename(override_string_path).split('.')[0]
        # This check is needed because some files that we process have no
        # replacements needed so in that case we don't even upload the override
        # file to the l10n service.
        if not os.path.exists(override_string_path):
            print('Skipping fully locally handled, override not present: '
                  f'{override_string_path} filename: {filename}')
            return
        print('Handled locally, sending only overrides to Crowdin: '
              f'{override_string_path} filename: {filename}')
        upload_source_file_to_crowdin(channel, override_string_path, filename)
        # Upload local translations if requested
        if args.with_translations or args.with_missing_translations:
            upload_grd_translations(
                channel,
                override_string_path,
                filename,
                missing_only=args.with_missing_translations,
                is_override=True)
        return

    print(f'[Crowdin]: {source_string_path}')
    upload_source_file_to_crowdin(channel, source_string_path, filename)
    ext = os.path.splitext(source_string_path)[1]
    if ext == '.grd':
        check_for_chromium_upgrade_extra_langs(SOURCE_ROOT, source_string_path)
        check_source_grd_strings_parity_with_crowdin(channel,
                                                     source_string_path)

    # Upload local translations if requested
    if (args.with_translations or args.with_missing_translations):
        if ext == '.grd':
            upload_grd_translations(
                channel,
                source_string_path,
                filename,
                missing_only=args.with_missing_translations)
        else:
            upload_json_translations_to_crowdin(
                channel,
                source_string_path,
                missing_only=args.with_missing_translations)


def upload_grd_translations(channel,
                            source_string_path,
                            filename,
                            missing_only,
                            is_override=False):
    # String by string upload is too slow, so only use it for missing
    # translations.
    if missing_only:
        upload_grd_translations_to_crowdin(channel, source_string_path,
                                           filename, missing_only, is_override)
    else:
        upload_translation_strings_xml_for_grd(channel, source_string_path,
                                               filename, is_override)


def check_for_chromium_upgrade_extra_langs(src_root, grd_file_path):
    """Checks the Brave GRD file vs the Chromium GRD file for extra
       languages."""
    chromium_grd_file_path = get_original_grd(src_root, grd_file_path)
    if not chromium_grd_file_path:
        return
    brave_langs = get_grd_languages(grd_file_path)
    chromium_langs = get_grd_languages(chromium_grd_file_path)
    x_brave_extra_langs = brave_langs - chromium_langs
    assert len(x_brave_extra_langs) == 0, \
        f'Brave GRD {grd_file_path} has extra languages ' \
            f'{list(x_brave_extra_langs)} over Chromium GRD ' \
            f'{chromium_grd_file_path}'
    x_chromium_extra_langs = chromium_langs - brave_langs
    assert len(x_chromium_extra_langs) == 0, \
        f'Chromium GRD {chromium_grd_file_path} has extra languages ' \
            f'{list(x_chromium_extra_langs)} over Brave GRD {grd_file_path}'


if __name__ == '__main__':
    sys.exit(main())
