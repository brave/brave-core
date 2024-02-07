#!/usr/bin/env python3
#
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import json
import re
import os
import lxml.etree  # pylint: disable=import-error

from lib.l10n.grd_utils import textify

# pylint: disable=import-error
from lib.l10n.crowdin.api_v2_client_wrapper import CrowdinClientWrapper
# pylint: enable=import-error

brave_project_id = 6  # Brave Core (Android+Chrome)

# This module contains functionality common to both pulling down translations
# from Crowdin and pushing source strings up to Crowdin.

# Filenames that are fully handled by Crowdin (as opposed to files for which
# we create overrides that are then handled by Crowdin).
crowdin_handled_files = [
    'android_brave_strings.xml',
    'brave_generated_resources.xml',
    'brave_components_strings.xml',
    'brave_extension.json',
]


def should_use_crowdin_for_file(source_string_path, filename):
    """ Determines if the given file should be handled by Crowdin locally"""
    name = crowdin_name_from_filename(source_string_path, filename)
    return name in crowdin_handled_files or name.startswith('greaselion_')


# pylint: disable=inconsistent-return-statements
def crowdin_name_from_filename(source_file_path, filename):
    ext = os.path.splitext(source_file_path)[1]
    # GRD files are uploaded in "Android XML" format.
    if ext == '.grd':
        return filename + '.xml'
    # JSON files are uploaded as "Chrome JSON" format.
    if 'brave_extension' in source_file_path:
        return 'brave_extension.json'
    assert False, ('JSON files should be mapped explicitly, this '
                   f'one is not: {source_file_path}')


# pylint: enable=inconsistent-return-statements


def xtb_lang_to_crowdin_lang(lang):
    """Reformats language code from XTB format to Crowdin format"""
    # The lang code "iw" is the old code for Hebrew, Crowdin and GRDs use
    # "he", but Chromium still uses "iw" inside the XTBs.
    if lang == 'iw':
        return 'he'
    if lang == 'pt-PT':
        return 'pt'
    return lang


def json_lang_to_crowdin_lang(lang):
    """Reformats language code from json format to Crowdin format"""
    lang = lang.replace('_', '-')
    if lang == 'pt-PT':
        return 'pt'
    return lang


def get_strings_dict_from_xml_content(xml_content):
    """Obtains a dictionary mapping the string name to text from XML content"""
    strings = lxml.etree.fromstring(xml_content).findall('string')
    return {
        string_tag.get('name'): textify_from_crowdin(string_tag)
        for string_tag in strings
    }


def fixup_string_from_crowdin(val):
    """Returns the text of a node from Crowdin which also fixes up common
       problems that localizers do"""
    if val is None:
        return val
    val = (val.replace('&amp;lt;',
                       '&lt;').replace('&amp;gt;',
                                       '&gt;').replace('&amp;amp;', '&amp;'))
    return val


def textify_from_crowdin(tag):
    """Returns the text content of a tag received from Crowdin while fixing
       up common problems that localizers cause"""
    return fixup_string_from_crowdin(textify(tag))


def get_acceptable_json_lang_codes(langs_dir_path):
    lang_codes = set(os.listdir(langs_dir_path))
    # Source language for Brave locales
    lang_codes.discard('en_US')

    # Files that are not locales
    lang_codes.discard('.DS_Store')
    lang_codes.discard('index.json')

    return sorted(lang_codes)


def get_json_strings(json_file_path):
    with open(json_file_path, mode='r', encoding='utf-8') as f:
        data = json.load(f)
    strings = []
    for key in data:
        string_name = key
        string_value = data[key]["message"]
        string_desc = data[key]["description"] if "description" \
            in data[key] else ""
        string_tuple = (string_name, string_value, string_desc)
        strings.append(string_tuple)
    return strings


# Client instance
def get_crowdin_client_wrapper():
    if get_crowdin_client_wrapper.wrapper is None:
        get_crowdin_client_wrapper.wrapper = CrowdinClientWrapper(
            project_id=brave_project_id)
    return get_crowdin_client_wrapper.wrapper


get_crowdin_client_wrapper.wrapper = None
