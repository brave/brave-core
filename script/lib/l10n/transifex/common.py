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

brave_project_name = 'brave_en'

# Transifex API v2 will be deprecated on April 7, 2022
use_api_v3 = True

if use_api_v3:
    from lib.l10n.transifex.api_v3_wrapper import \
        TransifexAPIV3Wrapper as APIWrapper
else:
    from lib.l10n.transifex.api_v2_wrapper import \
        TransifexAPIV2Wrapper as APIWrapper


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
    return slug in transifex_handled_slugs


# pylint: disable=inconsistent-return-statements
def transifex_name_from_filename(source_file_path, filename):
    ext = os.path.splitext(source_file_path)[1]
    if 'brave_components_strings' in source_file_path:
        return 'brave_components_resources'
    if ext == '.grd':
        return filename
    if 'brave_extension' in source_file_path:
        return 'brave_extension'
    if 'brave_rewards' in source_file_path:
        return 'rewards_extension'
    assert False, ('JSON files should be mapped explicitly, this '
                   f'one is not: {source_file_path}')
# pylint: enable=inconsistent-return-statements


def xtb_lang_to_transifex_lang(lang):
    """Reformats language code from XTB format to Transifex format"""
    lang = lang.replace('-', '_')
    # The lang code "iw" is the old code for Hebrew, Transifex and GRDs use
    # "he", but Chromium still uses "iw" inside the XTBs.
    lang = lang.replace('iw', 'he')
    lang = lang.replace('sr_Latn', 'sr_BA@latin')
    return lang


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

    # Files that are not locales
    lang_codes.discard('.DS_Store')
    lang_codes.discard('index.json')

    return sorted(lang_codes)


# Wrapper instance
def get_api_wrapper():
    if get_api_wrapper.wrapper is None:
        get_api_wrapper.wrapper = APIWrapper(project_name=brave_project_name)
    return get_api_wrapper.wrapper


get_api_wrapper.wrapper = None
