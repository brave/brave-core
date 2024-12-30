#!/usr/bin/env python3
#
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import html
import json
import os
import re
import defusedxml.ElementTree as ET

from lib.l10n.grd_utils import (get_grd_strings, get_override_file_path,
                                get_xtb_files)
from lib.l10n.crowdin.common import (
    get_acceptable_json_lang_codes, get_crowdin_client_wrapper,
    get_json_strings, get_strings_dict_from_xml_content,
    json_lang_to_crowdin_lang, textify_from_crowdin,
    crowdin_name_from_filename, xtb_lang_to_crowdin_lang)
from lib.l10n.validation import validate_tags_in_one_string

# This module contains functionality specific to pulling down translations
# from Crowdin.

# API functions
# -------------


def pull_source_file_from_crowdin(channel, source_file_path, filename,
                                  dump_path):
    """Downloads translations from Crowdin"""
    ext = os.path.splitext(source_file_path)[1]
    if ext == '.grd':
        xtb_files = get_xtb_files(source_file_path)
        base_path = os.path.dirname(source_file_path)
        grd_strings = get_grd_strings(source_file_path)
        for (lang_code, xtb_rel_path) in xtb_files:
            xtb_file_path = os.path.join(base_path, xtb_rel_path)
            print(f'Updating: {xtb_file_path} {lang_code}')
            xml_content = get_crowdin_translation_file_content(
                channel, source_file_path, filename, lang_code, dump_path)
            xml_content = fixup_bad_ph_tags_from_raw_crowdin_string(
                xml_content)
            errors = validate_tags_in_crowdin_strings(xml_content)
            assert errors is None, errors
            xml_content = trim_ph_tags_in_xtb_file_content(xml_content)
            translations = get_strings_dict_from_xml_content(xml_content)
            xtb_content = generate_xtb_content(lang_code, grd_strings,
                                               translations)
            with open(xtb_file_path, mode='wb') as f:
                f.write(xtb_content)
    elif ext == '.json':
        langs_dir_path = os.path.dirname(os.path.dirname(source_file_path))
        lang_codes = get_acceptable_json_lang_codes(langs_dir_path)
        for lang_code in lang_codes:
            print(f'getting filename {filename} for lang_code {lang_code}')
            content = get_crowdin_translation_file_content(
                channel, source_file_path, filename, lang_code, dump_path)
            json_content = generate_json_content(content, source_file_path)
            localized_translation_path = (os.path.join(langs_dir_path,
                                                       lang_code,
                                                       'messages.json'))
            dir_path = os.path.dirname(localized_translation_path)
            if not os.path.exists(dir_path):
                os.mkdir(dir_path)
            with open(localized_translation_path, mode='wb') as f:
                f.write(json_content.encode('utf-8'))


def combine_override_xtb_into_original(source_string_path):
    """Applies XTB override file to the original"""
    source_base_path = os.path.dirname(source_string_path)
    override_path = get_override_file_path(source_string_path)
    override_base_path = os.path.dirname(override_path)
    xtb_files = get_xtb_files(source_string_path)
    override_xtb_files = get_xtb_files(override_path)
    assert len(xtb_files) == len(override_xtb_files)

    for (idx, _) in enumerate(xtb_files):
        (lang, xtb_path) = xtb_files[idx]
        (override_lang, override_xtb_path) = override_xtb_files[idx]
        assert lang == override_lang

        xtb_tree = ET.parse(os.path.join(source_base_path, xtb_path))
        override_xtb_tree = ET.parse(
            os.path.join(override_base_path, override_xtb_path))
        translationbundle = xtb_tree.getroot()
        override_translations = override_xtb_tree.findall('.//translation')
        translations = xtb_tree.findall('.//translation')

        override_translation_fps = [
            t.attrib['id'] for t in override_translations
        ]
        translation_fps = [t.attrib['id'] for t in translations]

        # Remove translations that we have a matching FP for
        for translation in xtb_tree.findall('.//translation'):
            if translation.attrib['id'] in override_translation_fps:
                translation.getparent().remove(translation)
            elif translation_fps.count(translation.attrib['id']) > 1:
                translation.getparent().remove(translation)
                translation_fps.remove(translation.attrib['id'])

        # Append the override translations into the original translation bundle
        for translation in override_translations:
            translationbundle.append(translation)

        xtb_content = (b'<?xml version="1.0" ?>\n' +
                       ET.tostring(xtb_tree.getroot(),
                                   encoding='utf-8',
                                   xml_declaration=False).strip())
        with open(os.path.join(source_base_path, xtb_path), mode='wb') as f:
            f.write(xtb_content)
        # Delete the override xtb for this lang
        os.remove(os.path.join(override_base_path, override_xtb_path))


# Helper functions
# ----------------


def crowdin_lang_to_xtb_lang(lang):
    """Reformats language code from Crowdin format to XTB format"""
    # The lang code "iw" is the old code for Hebrew, Crowdin and GRDs use
    # "he", but Chromium still uses "iw" inside the XTBs, and it causes a
    # compiling error on Windows if "he" is used.
    if lang == 'he':
        return 'iw'
    if lang == 'pt':
        return 'pt-PT'
    return lang


def get_crowdin_translation_file_content(channel, source_file_path, filename,
                                         lang_code, dump_path):
    """Obtains a translation Android xml format and returns the string"""
    ext = os.path.splitext(source_file_path)[1]
    assert ext in ('.grd', '.json'), f'Unexpected extension {ext}'
    crowdin_lang_code = xtb_lang_to_crowdin_lang(
        lang_code) if ext == '.grd' else json_lang_to_crowdin_lang(lang_code)
    resource_name = crowdin_name_from_filename(source_file_path, filename)
    content = get_crowdin_client_wrapper().get_resource_l10n(
        channel, resource_name, crowdin_lang_code, ext)
    content = fix_crowdin_translation_file_content(content, ext)
    if dump_path:
        with open(dump_path, mode='wb') as f:
            f.write(content)
    verify_crowdin_translation_file_content(content, ext)
    return content.decode('utf-8')


def fix_crowdin_translation_file_content(content, file_ext):
    """Fixes escaped quotes in Crowdin translation file content"""
    if file_ext == '.json':
        # For .json files, for some reason Crowdin puts a \'
        return content.replace(b"\\'", b"'")
    if file_ext == '.grd':
        # For .grd files, for some reason Crowdin puts a \\" and \'
        return content.replace(b'\\\\"',
                               b'"').replace(b'\\"',
                                             b'"').replace(b"\\'", b"'")
    return None


def verify_crowdin_translation_file_content(content, file_ext):
    """Verifies that Crowdin translation file content is parse-able"""
    if file_ext == '.json':
        json.loads(content)
    elif file_ext == '.grd':
        ET.fromstring(content)


def fixup_bad_ph_tags_from_raw_crowdin_string(xml_content):
    """Attempts to fix improperly formatted PH tags in Crowdin translation
       file content"""
    begin_index = 0
    while begin_index < len(xml_content) and begin_index != -1:
        string_index = xml_content.find('<string', begin_index)
        if string_index == -1:
            return xml_content
        string_index = xml_content.find('>', string_index)
        if string_index == -1:
            return xml_content
        string_index += 1
        string_end_index = xml_content.find('</string>', string_index)
        if string_end_index == -1:
            return xml_content
        before_part = xml_content[:string_index]
        ending_part = xml_content[string_end_index:]
        val = process_bad_ph_tags_for_one_string(
            xml_content[string_index:string_end_index])
        xml_content = before_part + val + ending_part
        begin_index = xml_content.find('</string>', begin_index)
        if begin_index != -1:
            begin_index += 9
    return xml_content


def process_bad_ph_tags_for_one_string(val):
    """Fixes common issues with PH tag formatting"""
    val = (val.replace('\r\n', '\n').replace('\r', '\n'))
    if val.find('&lt;ph') == -1:
        return val
    val = (val.replace('&lt;', '<').replace(
        'ph name=&quot;',
        'ph name="').replace('ph name= &quot;', 'ph name="').replace(
            'ph name= ', 'ph name=').replace('&quot;&gt;', '">').replace(
                '&gt;', '>').replace('>  ', '> ').replace('  <', ' <'))
    return val


def trim_ph_tags_in_xtb_file_content(xml_content):
    """Removes all children of <ph> tags including text inside ph tag"""
    root = ET.fromstring(xml_content)
    for ph in root.findall('.//ph'):
        # Remove all children
        for child in list(ph):
            ph.remove(child)
        # Clear text
        ph.text = ''
    return ET.tostring(root, encoding='utf-8')


def generate_xtb_content(lang_code, grd_strings, translations):
    """Generates an XTB file from a set of translations and GRD strings"""
    # Used to make sure duplicate fingerprint strings are not made
    # XTB only contains 1 entry even if multiple string names are
    # different but have the same value.
    all_string_fps = set()
    translationbundle_tag = create_xtb_format_translationbundle_tag(lang_code)
    for string in grd_strings:
        if string[0] in translations:
            fingerprint = string[2]
            if fingerprint in all_string_fps:
                continue
            all_string_fps.add(fingerprint)
            translation = translations[string[0]]
            if len(translation) != 0:
                check_plural_string_formatting(string[1], translation)
                translationbundle_tag.append(
                    create_xtb_format_translation_tag(fingerprint,
                                                      translation))

    xml_string = ET.tostring(translationbundle_tag, encoding='utf-8')
    xml_string = html.unescape(xml_string.decode('utf-8'))
    xml_string = ('<?xml version="1.0" ?>\n<!DOCTYPE translationbundle>\n' +
                  xml_string)
    return xml_string.encode('utf-8')


def create_xtb_format_translationbundle_tag(lang):
    """Creates the root XTB XML element"""
    root = ET.Element('translationbundle')
    lang = crowdin_lang_to_xtb_lang(lang)
    root.set('lang', lang)
    root.text = '\n'
    return root


def check_plural_string_formatting(grd_string_content, translation_content):
    """Checks 'plural' string formatting in translations"""
    pattern = re.compile(r"\s*{(.*,\s*plural,)(\s*offset:[0-2])?"
                         r"(\s*(=0|zero)\s*{(.*)})?"
                         r"(\s*(=1|one)\s*{(.*)})?"
                         r"(\s*(=2|two)\s*{(.*)})?"
                         r"(\s*(few)\s*{(.*)})?"
                         r"(\s*(many)\s*{(.*)})?"
                         r"(\s*other\s*{(.*)})?"
                         r"\s*}\s*")
    if pattern.match(grd_string_content) is not None:
        if pattern.match(translation_content) is None:
            error = ('Translation of plural string:\n'
                     '-----------\n'
                     f"{grd_string_content.encode('utf-8')}\n"
                     '-----------\n'
                     'does not match:\n'
                     '-----------\n'
                     f"{translation_content.encode('utf-8')}\n"
                     '-----------\n')
            raise ValueError(error)
    else:
        # This finds plural strings that the pattern above doesn't catch
        leading_pattern = re.compile(r"\s*{.*,\s*plural,.*")
        if leading_pattern.match(grd_string_content) is not None:
            error = ('Uncaught plural pattern:\n'
                     '-----------\n'
                     f"{grd_string_content.encode('utf-8')}\n"
                     '-----------\n')
            raise ValueError(error)


def create_xtb_format_translation_tag(fingerprint, string_value):
    """Creates child XTB elements for each translation tag"""
    string_tag = ET.Element('translation')
    string_tag.set('id', str(fingerprint))
    if string_value.count('<') != string_value.count('>'):
        assert False, \
            'Warning: Unmatched < character, consider fixing on Crowdin, ' \
            f'force encoding the following string: {string_value}'
    string_tag.text = string_value
    string_tag.tail = '\n'
    return string_tag


def validate_tags_in_crowdin_strings(xml_content):
    """Validates that all child elements of all <string>s are allowed"""
    xml = ET.fromstring(xml_content)
    string_tags = xml.findall('.//string')
    # print(f'Validating HTML tags in {len(string_tags)} strings')
    errors = None
    for string_tag in string_tags:
        error = validate_tags_in_one_string(string_tag, textify_from_crowdin)
        if error is not None:
            errors = (errors or '') + error
    if errors is not None:
        errors = ("\n") + errors
    return errors


def generate_json_content(l10n_content, source_file_path):
    """Creates localized json file from source file and translations downloaded
       from Crowdin. Some of the translations may no longer be needed and
       untranslated strings need to be pulled from the source."""
    l10n_data = json.loads(l10n_content)
    source_strings = get_json_strings(source_file_path)
    content = {}
    for (string_name, string_value, string_desc) in source_strings:
        if string_name not in l10n_data:
            content[string_name] = \
                {"message": string_value,
                 "description": string_desc}
        else:
            # Fix escaped double quotes in values
            content[string_name] = \
                {"message": l10n_data[string_name]["message"].replace(
                    '\\"', '\"'),
                 "description": l10n_data[string_name]["description"]}
    return json.dumps(content, ensure_ascii=False, indent=2) + '\n'
