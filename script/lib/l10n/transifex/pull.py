#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import html
import json
import os
import re
import lxml.etree  # pylint: disable=import-error

from lib.l10n.grd_utils import (get_grd_strings,
                                get_override_file_path,
                                get_xtb_files)
from lib.l10n.transifex.common import (get_acceptable_json_lang_codes,
                                       get_api_wrapper,
                                       get_strings_dict_from_xml_content,
                                       textify_from_transifex,
                                       transifex_name_from_filename)
from lib.l10n.validation import validate_tags_in_one_string


# This module contains functionality specific to pulling down translations
# from Transifex.


# API functions
# -------------

def pull_source_files_from_transifex(source_file_path, filename, dump_path):
    """Downloads translations from Transifex"""
    ext = os.path.splitext(source_file_path)[1]
    if ext == '.grd':
        # Generate the intermediate Transifex format
        xtb_files = get_xtb_files(source_file_path)
        base_path = os.path.dirname(source_file_path)
        grd_strings = get_grd_strings(source_file_path)
        for (lang_code, xtb_rel_path) in xtb_files:
            xtb_file_path = os.path.join(base_path, xtb_rel_path)
            print(f'Updating: {xtb_file_path} {lang_code}')
            xml_content = get_transifex_translation_file_content(
                source_file_path, filename, lang_code, dump_path)
            xml_content = fixup_bad_ph_tags_from_raw_transifex_string(
                xml_content)
            errors = validate_tags_in_transifex_strings(xml_content)
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
            content = get_transifex_translation_file_content(
                source_file_path, filename, lang_code, dump_path)
            localized_translation_path = (
                os.path.join(langs_dir_path, lang_code, 'messages.json'))
            dir_path = os.path.dirname(localized_translation_path)
            if not os.path.exists(dir_path):
                os.mkdir(dir_path)
            with open(localized_translation_path, mode='wb') as f:
                f.write(content.encode('utf-8'))


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

        xtb_tree = lxml.etree.parse(os.path.join(source_base_path, xtb_path))
        override_xtb_tree = lxml.etree.parse(
            os.path.join(override_base_path, override_xtb_path))
        translationbundle = xtb_tree.xpath('//translationbundle')[0]
        override_translations = override_xtb_tree.xpath('//translation')
        translations = xtb_tree.xpath('//translation')

        override_translation_fps = [t.attrib['id']
                                    for t in override_translations]
        translation_fps = [t.attrib['id'] for t in translations]

        # Remove translations that we have a matching FP for
        for translation in xtb_tree.xpath('//translation'):
            if translation.attrib['id'] in override_translation_fps:
                translation.getparent().remove(translation)
            elif translation_fps.count(translation.attrib['id']) > 1:
                translation.getparent().remove(translation)
                translation_fps.remove(translation.attrib['id'])

        # Append the override translations into the original translation bundle
        for translation in override_translations:
            translationbundle.append(translation)

        xtb_content = (b'<?xml version="1.0" ?>\n' +
            lxml.etree.tostring(xtb_tree, pretty_print=True,
                xml_declaration=False, encoding='utf-8').strip())
        with open(os.path.join(source_base_path, xtb_path), mode='wb') as f:
            f.write(xtb_content)
        # Delete the override xtb for this lang
        os.remove(os.path.join(override_base_path, override_xtb_path))


# Helper functions
# ----------------

def xtb_lang_to_transifex_lang(lang):
    """Reformats language code from XTB format to Transifex format"""
    lang = lang.replace('-', '_')
    # The lang code "iw" is the old code for Hebrew, Transifex and GRDs use
    # "he", but Chromium still uses "iw" inside the XTBs.
    lang = lang.replace('iw', 'he')
    lang = lang.replace('sr_Latn', 'sr_BA@latin')
    return lang


def transifex_lang_to_xtb_lang(lang):
    """Reformats language code from Transifex format to XTB format"""
    lang = lang.replace('_', '-')
    # The lang code "iw" is the old code for Hebrew, Transifex and GRDs use
    # "he", but Chromium still uses "iw" inside the XTBs, and it causes a
    # compiling error on Windows if "he" is used.
    lang = lang.replace('he', 'iw')
    lang = lang.replace('sr-BA@latin', 'sr-Latn')
    return lang


def get_transifex_translation_file_content(source_file_path, filename,
                                           lang_code, dump_path):
    """Obtains a translation Android xml format and returns the string"""
    lang_code = xtb_lang_to_transifex_lang(lang_code)
    resource_name = transifex_name_from_filename(source_file_path, filename)
    ext = os.path.splitext(source_file_path)[1]
    content = get_api_wrapper().transifex_get_resource_l10n(
        resource_name, lang_code, ext)
    content = fix_transifex_translation_file_content(content, ext)
    if dump_path:
        with open(dump_path, mode='wb') as f:
            f.write(content)
    verify_transifex_translation_file_content(content, ext)
    return content.decode('utf-8')


def fix_transifex_translation_file_content(content, file_ext):
    """Fixes escaped quotes in Transifex translation file content"""
    if file_ext == '.json':
        # For .json files, for some reason Transifex puts a \'
        return content.replace(b"\\'", b"'")
    if file_ext == '.grd':
        # For .grd files, for some reason Transifex puts a \\" and \'
        return content.replace(b'\\\\"', b'"').replace(
            b'\\"', b'"').replace(b"\\'", b"'")
    return None


def verify_transifex_translation_file_content(content, file_ext):
    """Verifies that Transifex translation file content is parse-able"""
    if file_ext == '.json':
        # Make sure it's parseable
        json.loads(content)
    elif file_ext == '.grd':
        # Make sure it's parseable
        lxml.etree.fromstring(content)


def fixup_bad_ph_tags_from_raw_transifex_string(xml_content):
    """Attempts to fix improperly formatted PH tags in Transifex translation
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
    val = (val.replace('\r\n', '\n')
           .replace('\r', '\n'))
    if val.find('&lt;ph') == -1:
        return val
    val = (val.replace('&lt;', '<')
           .replace('&gt;', '>')
           .replace('>  ', '> ')
           .replace('  <', ' <'))
    return val


def trim_ph_tags_in_xtb_file_content(xml_content):
    """Removes all children of <ph> tags including text inside ph tag"""
    xml = lxml.etree.fromstring(xml_content)
    phs = xml.findall('.//ph')
    for ph in phs:
        lxml.etree.strip_elements(ph, '*')
        if ph.text is not None:
            ph.text = ''
    return lxml.etree.tostring(xml, encoding='utf-8')


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
                    create_xtb_format_translation_tag(
                        fingerprint, translation))

    xml_string = lxml.etree.tostring(translationbundle_tag, encoding='utf-8')
    xml_string = html.unescape(xml_string.decode('utf-8'))
    xml_string = (
        '<?xml version="1.0" ?>\n<!DOCTYPE translationbundle>\n' + xml_string)
    return xml_string.encode('utf-8')


def create_xtb_format_translationbundle_tag(lang):
    """Creates the root XTB XML element"""
    translationbundle_tag = lxml.etree.Element('translationbundle')
    lang = transifex_lang_to_xtb_lang(lang)
    translationbundle_tag.set('lang', lang)
    # Adds a newline so the first translation isn't glued to the
    # translationbundle element for us weak humans.
    translationbundle_tag.text = '\n'
    return translationbundle_tag


def check_plural_string_formatting(grd_string_content, translation_content):
    """Checks 'plural' string formatting in translations"""
    pattern = re.compile(
        r"\s*{.*,\s*plural,(\s*offset:[0-2])?"
        r"(\s*(=[0-2]|[zero|one|two|few|many])"
        r"\s*{(.*)})+\s*other\s*{(.*)}\s*}\s*")
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
    string_tag = lxml.etree.Element('translation')
    string_tag.set('id', str(fingerprint))
    if string_value.count('<') != string_value.count('>'):
        assert False, \
            'Warning: Unmatched < character, consider fixing on Transifex, ' \
            f'force encoding the following string: {string_value}'
    string_tag.text = string_value
    string_tag.tail = '\n'
    return string_tag


def validate_tags_in_transifex_strings(xml_content):
    """Validates that all child elements of all <string>s are allowed"""
    xml = lxml.etree.fromstring(xml_content)
    string_tags = xml.findall('.//string')
    # print(f'Validating HTML tags in {len(string_tags)} strings')
    errors = None
    for string_tag in string_tags:
        error = validate_tags_in_one_string(string_tag, textify_from_transifex)
        if error is not None:
            errors = (errors or '') + error
    if errors is not None:
        errors = ("\n") + errors
    return errors
