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
from lib.l10n.transifex.common import should_use_transifex_for_file
from lib.l10n.transifex.push import (
    check_for_chromium_upgrade,
    check_missing_source_grd_strings_to_transifex,
    upload_grd_translations_to_transifex,
    upload_json_translations_to_transifex,
    upload_source_files_to_transifex,
    upload_source_strings_desc)
from lib.l10n.grd_utils import get_override_file_path


BRAVE_SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SOURCE_ROOT = os.path.dirname(BRAVE_SOURCE_ROOT)


def parse_args():
    parser = argparse.ArgumentParser(description='Push strings to Transifex')
    parser.add_argument('--source_string_path', nargs=1, required=True)
    parser.add_argument('--service',
                        nargs=1,
                        choices=['Transifex', 'Crowdin'],
                        default='Transifex')
    parser.add_argument('--channel',
                        nargs=1,
                        choices=['Release', 'Beta', 'Nightly'],
                        default='Release')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--with_translations',
                       dest='with_translations',
                       action='store_true',
                       help='Uploads translations from the local .xtb and ' \
                            '.json files to Transifex. WARNING: This will ' \
                            'overwrite the Transifex translations with the ' \
                            'local values')
    group.add_argument('--with_missing_translations',
                       dest='with_missing_translations', action='store_true',
                       help='Uploads translations from the local .xtb and ' \
                            '.json files to Transifex, but only for strings '\
                            'that are not translated in Transifex.')
    return parser.parse_args()


def main():
    args = parse_args()
    service = args.service[0]
    channel = args.channel[0]
    print(f'[push-l10n] Service: {service}, Channel: {channel}')
    if service == 'Transifex' and channel != 'Release':
        raise Exception('Only Release channel is supported with Transifex')

    source_string_path = os.path.join(BRAVE_SOURCE_ROOT,
                                      args.source_string_path[0])
    filename = os.path.basename(source_string_path).split('.')[0]

    use_crowdin = service == 'Crowdin'
    should_use_service_for_file = (should_use_crowdin_for_file(
        source_string_path, filename) if use_crowdin else
                                   should_use_transifex_for_file(
                                       source_string_path, filename))

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
        print(f'Handled locally, sending only overrides to {service}: '
              f'{override_string_path} filename: {filename}')
        upload_source_file(use_crowdin, channel, override_string_path,
                           filename)
        upload_source_strings_descriptions(use_crowdin, override_string_path,
                                           filename)
        # Upload local translations if requested
        if args.with_translations or args.with_missing_translations:
            upload_grd_translations(
                use_crowdin,
                channel,
                override_string_path,
                filename,
                missing_only=args.with_missing_translations,
                is_override=True)
        return

    print(f'[{service}]: {source_string_path}')
    upload_source_file(use_crowdin, channel, source_string_path, filename)
    ext = os.path.splitext(source_string_path)[1]
    if ext == '.grd':
        check_for_chromium_upgrade(SOURCE_ROOT, source_string_path)
        check_source_grd_strings_parity_with_service(use_crowdin, channel,
                                                     source_string_path)
    upload_source_strings_descriptions(use_crowdin, source_string_path,
                                       filename)

    # Upload local translations if requested
    if (args.with_translations or args.with_missing_translations):
        if ext == '.grd':
            upload_grd_translations(
                use_crowdin,
                channel,
                source_string_path,
                filename,
                missing_only=args.with_missing_translations)
        else:
            upload_json_translations(
                use_crowdin,
                channel,
                source_string_path,
                missing_only=args.with_missing_translations)


def upload_source_file(use_crowdin, channel, source_string_path, filename):
    if use_crowdin:
        upload_source_file_to_crowdin(channel, source_string_path, filename)
    else:
        upload_source_files_to_transifex(source_string_path, filename)


def upload_source_strings_descriptions(use_crowdin, source_string_path,
                                       filename):
    # Crowdin descriptions are uploaded with the source file
    if not use_crowdin:
        upload_source_strings_desc(source_string_path, filename)


def upload_grd_translations(use_crowdin,
                            channel,
                            source_string_path,
                            filename,
                            missing_only,
                            is_override=False):
    if use_crowdin:
        # String by string upload is too slow, so only use it for missing
        # translations.
        if missing_only:
            upload_grd_translations_to_crowdin(channel, source_string_path,
                                               filename, missing_only,
                                               is_override)
        else:
            upload_translation_strings_xml_for_grd(channel, source_string_path,
                                                   filename, is_override)
    else:
        upload_grd_translations_to_transifex(source_string_path, filename,
                                             missing_only, is_override)


def upload_json_translations(use_crowdin, channel, source_string_path,
                             missing_only):
    if use_crowdin:
        upload_json_translations_to_crowdin(channel, source_string_path,
                                            missing_only)
    else:
        upload_json_translations_to_transifex(source_string_path, missing_only)


def check_source_grd_strings_parity_with_service(use_crowdin, channel,
                                                 source_string_path):
    if use_crowdin:
        check_source_grd_strings_parity_with_crowdin(channel,
                                                     source_string_path)
    else:
        check_missing_source_grd_strings_to_transifex(source_string_path)


if __name__ == '__main__':
    sys.exit(main())
