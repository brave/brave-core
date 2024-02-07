#!/usr/bin/env python3
#
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

from shutil import copyfile

import os
import tempfile
import lxml.etree  # pylint: disable=import-error

from lib.l10n.grd_utils import (get_grd_strings, get_xtb_files, textify)
from lib.l10n.crowdin.common import (
    crowdin_name_from_filename, get_acceptable_json_lang_codes,
    get_crowdin_client_wrapper, get_json_strings,
    get_strings_dict_from_xml_content, json_lang_to_crowdin_lang,
    xtb_lang_to_crowdin_lang)

# This module contains functionality specific to pushing translations up
# to Crowdin.

# API functions
# -------------


def upload_source_file_to_crowdin(channel, source_file_path, filename):
    uploaded = False
    i18n_type = ''
    ext = os.path.splitext(source_file_path)[1]
    assert ext in ('.grd', '.json'), ('Unsupported source file ext ' +
                                      f'{ext}: {source_file_path}')
    # Storage API derives the storage name from the filename, so use the
    # resource name for consistency.
    resource_name = crowdin_name_from_filename(source_file_path, filename)
    tempdir = tempfile.gettempdir()
    upload_file_path = os.path.join(tempdir, resource_name)
    if os.path.exists(upload_file_path):
        os.remove(upload_file_path)

    if ext == '.grd':
        # Generate the intermediate Android XML format for the source file.
        generate_source_strings_xml_from_grd(upload_file_path,
                                             source_file_path)
        i18n_type = 'ANDROID'
    elif ext == '.json':
        copyfile(source_file_path, upload_file_path)
        i18n_type = 'CHROME'

    uploaded = get_crowdin_client_wrapper().upload_resource_file(
        channel, upload_file_path, resource_name, i18n_type)
    assert uploaded, f'Could not upload file {upload_file_path}'
    os.remove(upload_file_path)


def check_source_grd_strings_parity_with_crowdin(channel, grd_file_path):
    """Compares the GRD strings to the strings on Crowdin and uploads any
       missing strings."""
    source_grd_strings = get_grd_strings(grd_file_path)
    if len(source_grd_strings) == 0:
        return
    strings_dict = get_crowdin_source_resource_strings(channel, grd_file_path)
    crowdin_string_ids = set(strings_dict.keys())
    grd_string_names = {string_name for (string_name, _, _, _) \
        in source_grd_strings}
    x_grd_extra_strings = grd_string_names - crowdin_string_ids
    assert len(x_grd_extra_strings) == 0, \
        f'GRD has extra strings over Crowdin {list(x_grd_extra_strings)}'
    x_crowdin_extra_strings = crowdin_string_ids - grd_string_names
    assert len(x_crowdin_extra_strings) == 0, \
        'Crowdin has extra strings over GRD ' \
            f'{list(x_crowdin_extra_strings)}'


def upload_json_translations_to_crowdin(channel, source_string_path,
                                        missing_only):
    resource_name = crowdin_name_from_filename(source_string_path, '')
    missing = 'missing' if missing_only else ''
    print(f'Uploading {missing} translations for {source_string_path} ' \
          f'(resource: {resource_name})', flush=True)
    source_strings = get_json_strings(source_string_path)
    langs_dir_path = os.path.dirname(os.path.dirname(source_string_path))
    lang_codes = get_acceptable_json_lang_codes(langs_dir_path)
    translations = {}
    for lang_code in lang_codes:
        crowdin_lang = json_lang_to_crowdin_lang(lang_code)
        if not get_crowdin_client_wrapper().is_supported_language(
                crowdin_lang):
            print(f'Skipping language {crowdin_lang} ({lang_code}).')
            continue
        print(f'Processing language {lang_code}')
        l10n_path = os.path.join(langs_dir_path, lang_code, 'messages.json')
        l10n_strings = get_json_strings(l10n_path)
        l10n_dict = {
            string_name: string_value
            for (string_name, string_value, _) in l10n_strings
        }
        for (string_name, string_value, _) in source_strings:
            if string_name not in l10n_dict:
                continue
            if len(l10n_dict[string_name]) == 0 or (lang_code != 'en_GB' and \
                l10n_dict[string_name] == string_value):
                continue
            key = string_name.split(".")[0]
            translation_value = (l10n_dict[string_name].replace(
                "\"", "\\\"").replace("\r", "\\r").replace("\n", "\\n"))
            if key in translations:
                translations[key].append((crowdin_lang, translation_value))
            else:
                translations[key] = [(crowdin_lang, translation_value)]

    upload_translations_to_crowdin(channel, resource_name, translations,
                                   missing_only)


def upload_grd_translations_to_crowdin(channel,
                                       source_string_path,
                                       filename,
                                       missing_only,
                                       is_override=False):
    resource_name = crowdin_name_from_filename(source_string_path, filename)
    missing = 'missing' if missing_only else ''
    print(f'Uploading {missing} translations for {source_string_path} ' \
          f'(resource: {resource_name})', flush=True)
    source_base_path = os.path.dirname(source_string_path)
    grd_strings = get_grd_strings(source_string_path, False)
    grd_xtbs = get_xtb_files(source_string_path)
    translations = {}
    for (lang, path) in grd_xtbs:
        crowdin_lang = xtb_lang_to_crowdin_lang(lang)
        if not get_crowdin_client_wrapper().is_supported_language(
                crowdin_lang):
            print(f'Skipping language {crowdin_lang} ({lang}).')
            continue
        xtb_full_path = os.path.join(source_base_path, path).replace('\\', '/')
        if is_override:
            xtb_full_path = xtb_full_path.replace('_override', '')
        print(
            f'Processing language {crowdin_lang} ({lang}) from {xtb_full_path}'
        )
        xtb_tree = lxml.etree.parse(xtb_full_path)
        xtb_strings = xtb_tree.xpath('//translation')
        for xtb_string in xtb_strings:
            string_fp = xtb_string.attrib['id']
            matches = [tup for tup in grd_strings if tup[2] == string_fp]
            # XTB files may have translations for string that are no longer in
            # the GRD, so only upload those that are needed for the GRD.
            if len(matches):
                key = matches[0][0]
                value = textify(xtb_string)
                if len(value) == 0:
                    print(f'Translation for {key} is empty')
                    continue
                if key in translations:
                    translations[key].append((crowdin_lang, value))
                else:
                    translations[key] = [(crowdin_lang, value)]

    upload_translations_to_crowdin(channel, resource_name, translations,
                                   missing_only)


def upload_translation_strings_xml_for_grd(channel,
                                           source_string_path,
                                           filename,
                                           is_override=False):
    """Generates string xml files for a GRD file from its XTB files in the
       same format as the source we upload to Crowdin. These xml files can be
       manually uploaded to Crowdin via their Translations page."""
    resource_name = crowdin_name_from_filename(source_string_path, filename)
    print(f'Generating translations for {source_string_path} ' \
          f'(resource: {resource_name})', flush=True)
    source_base_path = os.path.dirname(source_string_path)
    # Get all grd strings (with fingerprints)
    grd_strings = get_grd_strings(source_string_path, False)
    # Get all xtb files from grd header
    grd_xtbs = get_xtb_files(source_string_path)
    tempdir = tempfile.gettempdir()

    for (lang, path) in grd_xtbs:
        crowdin_lang = xtb_lang_to_crowdin_lang(lang)
        if not get_crowdin_client_wrapper().is_supported_language(
                crowdin_lang):
            print(f'Skipping language {crowdin_lang} ({lang}).')
            continue
        # Prepare output xml and file
        resources_tag = lxml.etree.Element('resources')
        output_xml_file_path = os.path.join(
            tempdir, resource_name + f'_{crowdin_lang}.xml')
        if os.path.exists(output_xml_file_path):
            os.remove(output_xml_file_path)

        # Load XTB strings
        xtb_full_path = os.path.join(source_base_path, path).replace('\\', '/')
        if is_override:
            xtb_full_path = xtb_full_path.replace('_override', '')
        print(
            f'Processing language {crowdin_lang} ({lang}) from {xtb_full_path}'
        )
        xtb_tree = lxml.etree.parse(xtb_full_path)
        xtb_strings = xtb_tree.xpath('//translation')
        # print(f'Loaded {len(xtb_strings)} translations')

        for xtb_string in xtb_strings:
            string_fp = xtb_string.attrib['id']
            matches = [tup for tup in grd_strings if tup[2] == string_fp]
            # XTB files may have translations for string that are no longer in
            # the GRD, so only upload those that are needed for the GRD.
            if len(matches):
                # Revert escaping of & because lxml.etree.tostring will do
                # it again and we'll end up with &amp;amp;
                value = textify(xtb_string).replace('&amp;', '&')
                for match in matches:
                    key = match[0]
                    # Leave description empty - it's not needed for translation
                    # files.
                    resources_tag.append(
                        create_android_format_string_tag(key,
                                                         value,
                                                         string_desc=""))

        xml_string = lxml.etree.tostring(resources_tag,
                                         xml_declaration=True,
                                         encoding='utf-8')
        with open(output_xml_file_path, mode='wb') as f:
            f.write(xml_string)
        print(f'Uploading l10n for {resource_name}: {crowdin_lang}')
        uploaded = get_crowdin_client_wrapper().upload_grd_l10n_file(
            channel, output_xml_file_path, resource_name, crowdin_lang)
        assert uploaded, 'Failed to upload.'
        os.remove(output_xml_file_path)


# Helper functions
# ----------------


def generate_source_strings_xml_from_grd(output_xml_file_path, grd_file_path):
    """Generates a source string xml file from a GRD file"""
    resources_tag = lxml.etree.Element('resources')
    all_strings = get_grd_strings(grd_file_path)
    assert len(all_strings) > 0, f'GRD {grd_file_path} appears to be empty'
    for (string_name, string_value, _, string_desc) in all_strings:
        (string_value,
         string_desc) = process_source_string_value(string_value, string_desc)
        # Revert escaping of & because lxml.etree.tostring will do it again
        # and we'll end up with &amp;amp;
        resources_tag.append(
            create_android_format_string_tag(
                string_name, string_value.replace('&amp;', '&'), string_desc))
    print(f'Generating {len(all_strings)} strings for GRD: {grd_file_path}')
    xml_string = lxml.etree.tostring(resources_tag,
                                     xml_declaration=True,
                                     encoding='utf-8')
    with open(output_xml_file_path, mode='wb') as f:
        f.write(xml_string)


def process_source_string_value(string_value, string_desc):
    """Empty everything out from placeholders. The content of placeholders
       doesn't need to be localized and only confuses localizers. Plus, it
       gets stripped out anyway when we download the translations. The only
       useful parts of the placeholders are the example values which we can
       extract here and add to the comment."""
    value_xml = lxml.etree.fromstring('<string>' + string_value + '</string>')
    phs = value_xml.findall('.//ph')
    examples = []
    for ph in phs:
        name = ph.get('name')
        example = ph.findtext('ex')
        if example is not None:
            examples.append((name, example))
        lxml.etree.strip_elements(ph, '*')
        if ph.text is not None:
            ph.text = ''
    string_desc = add_placeholders_examples_to_description(
        string_desc, examples)
    string_value = lxml.etree.tostring(
        value_xml, encoding='utf-8').decode('utf-8').replace('></ph>', '/>')
    return (string_value[8:-9], string_desc)


def add_placeholders_examples_to_description(string_desc, examples):
    if len(examples):
        string_desc = string_desc.strip()
        if not string_desc.endswith('.'):
            string_desc = string_desc + '.'
        string_desc = string_desc + '\nPlaceholders examples:'
        for example in examples:
            string_desc = string_desc + f'\n{example[0]}={example[1]}'
    return string_desc


def create_android_format_string_tag(string_name, string_value, string_desc):
    """Creates intermediate Android format child tag for each translation
       string"""
    string_tag = lxml.etree.Element('string')
    string_tag.set('name', string_name)
    string_tag.set('comment', string_desc)
    string_tag.text = string_value
    string_tag.tail = '\n'
    return string_tag


def get_crowdin_source_resource_strings(channel, grd_file_path):
    """Obtains the list of strings from Crowdin"""
    filename = os.path.basename(grd_file_path).split('.')[0]
    resource_name = crowdin_name_from_filename(grd_file_path, filename)
    content = get_crowdin_client_wrapper().get_resource_source(
        channel, resource_name)
    return get_strings_dict_from_xml_content(content)


def upload_translations_to_crowdin(channel, resource_name, translations,
                                   missing_only):
    """Uploads the list of (lang_code, key, translation)s."""
    print(f'Uploading translations for {len(translations)} strings.')
    get_crowdin_client_wrapper().upload_strings_l10n(channel, resource_name,
                                                     translations,
                                                     missing_only)
