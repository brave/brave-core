#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import argparse
import os.path
import sys
import tempfile

from lib.l10n.crowdin.common import should_use_crowdin_for_file
from lib.l10n.crowdin.pull import pull_source_file_from_crowdin
from lib.l10n.grd_utils import (combine_override_xtb_into_original,
                                get_override_file_path, update_xtbs_locally)


BRAVE_SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def parse_args():
    parser = argparse.ArgumentParser(description='Pull strings from Crowdin')
    parser.add_argument('--source_string_path',
                        nargs=1,
                        help='path to the source file (GRD or JSON)')
    parser.add_argument('--channel',
                        choices=['Release', 'Beta', 'Nightly'],
                        nargs='?',
                        default='Release',
                        const='Release')
    parser.add_argument('--lang', nargs=1, help='only download this language')
    parser.add_argument('--debug', dest='debug', action='store_true',
                        help='dump downloaded content for the current ' \
                             'language to the CrowdinCurrent.txt file ' \
                             'in the temp directory')
    return parser.parse_args()


def main():
    args = parse_args()
    channel = args.channel
    # TODO(https://github.com/brave/brave-browser/issues/51497): Generate
    # separate XTB files for brave_origin brand instead of sharing with brave.
    # For now, always use 'brave' brand.
    brand = 'brave'
    print(f'[pull-l10n] Channel: {channel}')
    dump_path = None
    if args.debug:
        dump_path = os.path.join(tempfile.gettempdir(), 'CrowdinCurrent.txt')
        print(f'DEBUG: Content dump file = {dump_path}')

    source_string_path = os.path.join(
        BRAVE_SOURCE_ROOT, args.source_string_path[0])
    filename = os.path.basename(source_string_path).split('.')[0]
    if filename in ('android_webapps_strings', 'locale_settings_linux',
                    'locale_settings_mac', 'locale_settings_win'):
        print(f'Skipping {filename}.grd - no translations needed.')
        return

    lang = args.lang[0] if args.lang else None
    if lang:
        print(f'Downloading only for language = {lang}')

    should_use_service_for_file = should_use_crowdin_for_file(
        source_string_path, filename)

    if should_use_service_for_file:
        print(f'Crowdin: {source_string_path}')
        pull_source_file_from_crowdin(channel, source_string_path, filename,
                                      lang, dump_path)
    else:
        print('Local: ', source_string_path)
        override_path = get_override_file_path(source_string_path)
        override_exists = os.path.exists(override_path)
        if override_exists:
            print(f'Crowdin override: {override_path}')
            override_filename = os.path.basename(override_path).split('.')[0]
            pull_source_file_from_crowdin(channel, override_path,
                                          override_filename, lang, dump_path)
        else:
            print('No Crowdin override.')

        update_xtbs_locally(source_string_path, BRAVE_SOURCE_ROOT, lang, brand)
        if override_exists:
            combine_override_xtb_into_original(source_string_path, lang)


if __name__ == '__main__':
    sys.exit(main())
