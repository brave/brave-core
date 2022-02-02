#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */


import re
import os
import lxml.etree  # pylint: disable=import-error

from lib.l10n.grd_utils import textify


# This module contains functionality common to both pulling down translations
# from Transifex and pushing source strings up to Transifex.


# Filenames (slugs) that are fully handled by Transifex
transifex_handled_slugs = [
    'android_brave_strings',
    'brave_generated_resources',
    'brave_components_resources',
    'brave_extension',
    'rewards_extension',
    'ethereum_remote_client_extension'
]


def should_use_transifex_for_file(source_string_path, filename):
    """ Determines if the given filename should be pulled from Transifex or
        handled locally"""
    slug = transifex_name_from_filename(source_string_path, filename)
    return slug in transifex_handled_slugs or slug.startswith('greaselion_')


def transifex_name_from_filename(source_file_path, filename):
    ext = os.path.splitext(source_file_path)[1]
    if 'brave_components_strings' in source_file_path:
        return 'brave_components_resources'
    if ext == '.grd':
        return filename
    if 'brave-site-specific-scripts/scripts/' in source_file_path:
        return transifex_name_from_greaselion_script_name(source_file_path)
    if 'brave_extension' in source_file_path:
        return 'brave_extension'
    if 'brave_rewards' in source_file_path:
        return 'rewards_extension'
    if 'ethereum-remote-client/app' in source_file_path:
        return 'ethereum_remote_client_extension'
    assert False, ('JSON files should be mapped explicitly, this '
                   f'one is not: {source_file_path}')


def transifex_name_from_greaselion_script_name(script_name):
    match = re.search(('brave-site-specific-scripts/scripts/(.*)/_locales/' +
                       'en_US/messages.json$'), script_name)
    if match:
        return ('greaselion_' +
            match.group(1).replace('-', '_').replace('/', '_'))
    return ''


def get_strings_dict_from_xml_content(xml_content):
    """Obtains a dictionary mapping the string name to text from Android xml
       content"""
    strings = lxml.etree.fromstring(xml_content).findall('string')
    return {string_tag.get('name'): textify_from_transifex(string_tag)
            for string_tag in strings}


def fixup_string_from_transifex(val):
    """Returns the text of a node from Transifex which also fixes up common
       problems that localizers do"""
    if val is None:
        return val
    val = (val.replace('&amp;lt;', '&lt;')
              .replace('&amp;gt;', '&gt;')
                  .replace('&amp;amp;', '&amp;'))
    return val


def textify_from_transifex(tag):
    """Returns the text content of a tag received from Transifex while fixing
       up common problems that localizers cause"""
    return fixup_string_from_transifex(textify(tag))


def get_acceptable_json_lang_codes(langs_dir_path):
    lang_codes = set(os.listdir(langs_dir_path))
    # Source language for Brave locales
    lang_codes.discard('en_US')

    # Source language for ethereum-remote-client
    lang_codes.discard('en')

    # Files that are not locales
    lang_codes.discard('.DS_Store')
    lang_codes.discard('index.json')

    # ethereum-remote-client has these unsupported locales
    lang_codes.discard('tml')
    lang_codes.discard('hn')
    lang_codes.discard('ph')
    lang_codes.discard('ht')
    return sorted(lang_codes)
