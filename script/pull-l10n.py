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
from lib.l10n.grd_utils import (get_override_file_path, update_xtbs_locally)
from lib.l10n.transifex.common import should_use_transifex_for_file
from lib.l10n.transifex.pull import (combine_override_xtb_into_original,
                                     pull_source_files_from_transifex)


BRAVE_SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def parse_args():
    parser = argparse.ArgumentParser(description='Pull strings from Transifex')
    parser.add_argument('--source_string_path', nargs=1,
                        help='path to the source file (GRD(P) or JSON)')
    parser.add_argument('--service',
                        nargs=1,
                        choices=['Transifex', 'Crowdin'],
                        default='Transifex')
    parser.add_argument('--channel',
                        nargs=1,
                        choices=['Release', 'Beta', 'Nightly'],
                        default='Release')
    parser.add_argument('--debug', dest='debug', action='store_true',
                        help='dump downloaded content for the current ' \
                             'language to the TransifexCurrent.txt file in ' \
                             'the temp directory')
    return parser.parse_args()


def main():
    args = parse_args()
    if args.service == 'Transifex' and args.channel != 'Release':
        raise Exception('Only Release channel is supported with Transifex')
    service = args.service[0]
    channel = args.channel[0]
    print(f'[pull-l10n] Service: {service}, Channel: {channel}')
    dump_path = None
    if args.debug:
        dump_path = os.path.join(tempfile.gettempdir(),
                                 f'{service}Current.txt')
        print(f'DEBUG: Content dump file = {dump_path}')

    source_string_path = os.path.join(
        BRAVE_SOURCE_ROOT, args.source_string_path[0])
    filename = os.path.basename(source_string_path).split('.')[0]

    use_crowdin = service == 'Crowdin'
    should_use_service_for_file = (should_use_crowdin_for_file(
        source_string_path, filename) if use_crowdin else
                                   should_use_transifex_for_file(
                                       source_string_path, filename))

    if should_use_service_for_file:
        print(f'{service}: ', source_string_path)
        pull_source_file_from_service(use_crowdin, channel, source_string_path,
                                      filename, dump_path)
    else:
        print('Local: ', source_string_path)
        override_path = get_override_file_path(source_string_path)
        override_exists = os.path.exists(override_path)
        if override_exists:
            print(f'{service} override: ', override_path)
            override_filename = os.path.basename(override_path).split('.')[0]
            pull_source_file_from_service(use_crowdin, channel, override_path,
                                          override_filename, dump_path)
        else:
            print(f'No {service} override.')

        update_xtbs_locally(source_string_path, BRAVE_SOURCE_ROOT)
        if override_exists:
            combine_override_xtb_into_original(source_string_path)


def pull_source_file_from_service(use_crowdin, channel, source_file_path,
                                  filename, dump_path):
    if use_crowdin:
        pull_source_file_from_crowdin(channel, source_file_path, filename,
                                      dump_path)
    else:
        pull_source_files_from_transifex(source_file_path, filename, dump_path)


if __name__ == '__main__':
    sys.exit(main())
