#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

from hashlib import md5

import io
import json
import os
import tempfile
import lxml.etree  # pylint: disable=import-error

from lib.l10n.grd_utils import (get_grd_languages,
                                get_grd_strings,
                                get_original_grd)
from lib.l10n.transifex.common import (get_acceptable_json_lang_codes,
                                       get_api_wrapper,
                                       get_strings_dict_from_xml_content,
                                       transifex_name_from_filename)


# This module contains functionality specific to pushing translations up
# to Transifex.


# API functions
# -------------

def upload_source_files_to_transifex(source_file_path, filename):
    uploaded = False
    i18n_type = ''
    content = ''
    ext = os.path.splitext(source_file_path)[1]
    if ext == '.grd':
        # Generate the intermediate Transifex format for the source
        # translations.
        temp_file = tempfile.mkstemp('.xml')
        output_xml_file_handle, _ = temp_file
        content = generate_source_strings_xml_from_grd(
            output_xml_file_handle, source_file_path).decode('utf-8')
        os.close(output_xml_file_handle)
        i18n_type = 'ANDROID'
    elif ext == '.json':
        i18n_type = 'CHROME'
        with io.open(source_file_path, mode='r', encoding='utf-8') as json_file:
            content = json_file.read()
    else:
        assert False, f'Unsupported source file ext {ext}: {source_file_path}'

    uploaded = upload_source_string_file_to_transifex(source_file_path,
                                                      filename, content,
                                                      i18n_type)
    assert uploaded, 'Could not upload xml file'


def check_for_chromium_upgrade(src_root, grd_file_path):
    """Performs various checks and changes as needed for when Chromium source
       files change."""
    check_for_chromium_upgrade_extra_langs(src_root, grd_file_path)


def check_missing_source_grd_strings_to_transifex(grd_file_path):
    """Compares the GRD strings to the strings on Transifex and uploads any
       missing strings."""
    source_grd_strings = get_grd_strings(grd_file_path)
    if len(source_grd_strings) == 0:
        return
    strings_dict = get_transifex_source_resource_strings(grd_file_path)
    transifex_string_ids = set(strings_dict.keys())
    grd_string_names = {string_name for (string_name, _, _, _) \
        in source_grd_strings}
    x_grd_extra_strings = grd_string_names - transifex_string_ids
    assert len(x_grd_extra_strings) == 0, \
        f'GRD has extra strings over Transifex {list(x_grd_extra_strings)}'
    x_transifex_extra_strings = transifex_string_ids - grd_string_names
    assert len(x_transifex_extra_strings) == 0, \
        'Transifex has extra strings over GRD ' \
            f'{list(x_transifex_extra_strings)}'


def upload_source_strings_desc(source_file_path, filename):
    ext = os.path.splitext(source_file_path)[1]
    print(f'Uploading strings descriptions for {source_file_path}')
    if ext == '.json':
        json_strings = get_json_strings(source_file_path)
        for (string_name, _, string_desc) in json_strings:
            if len(string_desc) > 0:
                upload_string_desc(source_file_path, filename,
                                   string_name, string_desc)
    else:
        grd_strings = get_grd_strings(source_file_path)
        for (string_name, _, _, string_desc) in grd_strings:
            if len(string_desc) > 0:
                upload_string_desc(source_file_path, filename,
                                   string_name, string_desc)


def upload_missing_json_translations_to_transifex(source_string_path):
    source_strings = get_json_strings(source_string_path)
    langs_dir_path = os.path.dirname(os.path.dirname(source_string_path))
    lang_codes = get_acceptable_json_lang_codes(langs_dir_path)
    filename = transifex_name_from_filename(source_string_path, '')
    for lang_code in lang_codes:
        l10n_path = os.path.join(langs_dir_path, lang_code, 'messages.json')
        l10n_strings = get_json_strings(l10n_path)
        l10n_dict = {string_name: string_value for (
            string_name, string_value, _) in l10n_strings}
        for (string_name, string_value, _) in source_strings:
            if string_name not in l10n_dict:
                # print(f'Skipping string name {string_name} for language ' +
                #       f'{lang_code}: non-existent')
                continue
            if l10n_dict[string_name] == string_value:
                # print(f'Skipping string name {string_name} for language ' +
                #       f'{lang_code}: not localized')
                continue
            translation_value = (l10n_dict[string_name]
                                 .replace("\"", "\\\"")
                                 .replace("\r", "\\r")
                                 .replace("\n", "\\n"))
            upload_missing_translation_to_transifex(source_string_path,
                                                    lang_code, filename,
                                                    string_name.split(".")[0],
                                                    translation_value)


# Helper functions
# ----------------


def generate_source_strings_xml_from_grd(output_xml_file_handle, grd_file_path):
    """Generates a source string xml file from a GRD file"""
    resources_tag = create_android_format_resources_tag()
    all_strings = get_grd_strings(grd_file_path)
    for (string_name, string_value, _, _) in all_strings:
        resources_tag.append(
            create_android_format_string_tag(string_name, string_value))
    print(f'Generating {len(all_strings)} strings for GRD: {grd_file_path}')
    xml_string = lxml.etree.tostring(resources_tag, encoding='utf-8')
    os.write(output_xml_file_handle, xml_string)
    return xml_string


def create_android_format_resources_tag():
    """Creates intermediate Android format root tag"""
    return lxml.etree.Element('resources')


def create_android_format_string_tag(string_name, string_value):
    """Creates intermediate Android format child tag for each translation
       string"""
    string_tag = lxml.etree.Element('string')
    string_tag.set('name', string_name)
    string_tag.text = string_value
    string_tag.tail = '\n'
    return string_tag


def upload_source_string_file_to_transifex(source_file_path, filename,
                                           xml_content, i18n_type):
    """Uploads the specified source string file to transifex"""
    print(f'Uploading resource for filename {filename}')
    resource_name = transifex_name_from_filename(source_file_path, filename)
    return get_api_wrapper().transifex_upload_resource_content(resource_name,
                                                               xml_content,
                                                               i18n_type)


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


def get_transifex_source_resource_strings(grd_file_path):
    """Obtains the list of strings from Transifex"""
    filename = os.path.basename(grd_file_path).split('.')[0]
    resource_name = transifex_name_from_filename(grd_file_path, filename)
    content = get_api_wrapper().transifex_get_resource_source(resource_name)
    return get_strings_dict_from_xml_content(content)


def get_json_strings(json_file_path):
    with open(json_file_path, mode='r', encoding='utf-8') as f:
        data = json.load(f)
    strings = []
    for key in data:
        # Our json (brave_extension and rewards_extension) resources somehow
        # ended up on Transifex with keys having a '.message' suffix.
        # Adding the same files to a new projects doesn't result in the suffix
        # being added, so perhaps this is some old quirk of Transifex that's
        # been grandfathered for the brave project.
        string_name = key + '.message'
        string_value = data[key]["message"]
        string_desc = data[key]["description"] if "description" \
            in data[key] else ""
        string_tuple = (string_name, string_value, string_desc)
        strings.append(string_tuple)
    return strings


def upload_string_desc(source_file_path, filename, string_name, string_desc):
    """Uploads descriptions for strings"""
    string_hash = get_transifex_string_hash(string_name)
    resource_name = transifex_name_from_filename(source_file_path, filename)
    print('Uploading string description for string: '
          f'{string_name} (hash: {string_hash})')
    get_api_wrapper().transifex_upload_string_desc(resource_name, string_hash,
                                                   string_desc)


def get_transifex_string_hash(string_name):
    """Obtains transifex string hash for the passed string."""
    return str(md5(':'.join([string_name, '']).encode('utf-8')).hexdigest())


def upload_missing_translation_to_transifex(source_string_path, lang_code,
                                            filename, string_name,
                                            translated_value):
    """Uploads the specified string to the specified language code."""
    resource_name = transifex_name_from_filename(source_string_path, filename)
    string_hash = get_transifex_string_hash(string_name)
    translated_value = braveify(translated_value)
    get_api_wrapper().transifex_upload_string_l10n(resource_name, string_hash,
                                                   lang_code, translated_value)
    print(f'Uploaded {lang_code} string: {string_name}...')


def braveify(string_value):
    """Replace Chromium branded strings with Brave branded strings."""
    return (string_value.replace('Chrome', 'Brave')
            .replace('Chromium', 'Brave')
            .replace('Google', 'Brave')
            .replace('Brave Docs', 'Google Docs')
            .replace('Brave Drive', 'Google Drive')
            .replace('Brave Play', 'Google Play')
            .replace('Brave Safe', 'Google Safe')
            .replace('Sends URLs of some pages you visit to Brave',
                     'Sends URLs of some pages you visit to Google')
            .replace('Brave Account', 'Brave sync chain')
            .replace('Brave Lens', 'Google Lens')
            .replace('Bravebook', 'Chromebook')
            .replace('Bravecast', 'Chromecast')
            .replace('Brave Cloud', 'Google Cloud')
            .replace('Brave Pay', 'Google Pay')
            .replace('Brave Photos', 'Google Photos')
            .replace('Brave Projects', 'Chromium Projects'))
