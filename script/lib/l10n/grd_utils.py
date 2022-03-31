#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

from collections import defaultdict

import os
import re
import FP
import lxml.etree  # pylint: disable=import-error

from lib.l10n.grd_string_replacements import (branding_replacements,
                                              default_replacements,
                                              fixup_replacements,
                                              main_text_only_replacements)
from lib.l10n.validation import validate_tags_in_one_string

def braveify_grd_text(text, is_main_text, branding_replacements_only):
    """Replaces text string to Brave wording"""
    for (pattern, to) in branding_replacements:
        text = re.sub(pattern, to, text)
    if not branding_replacements_only:
        for (pattern, to) in default_replacements:
            text = re.sub(pattern, to, text)
    for (pattern, to) in fixup_replacements:
        text = re.sub(pattern, to, text)
    if is_main_text:
        for (pattern, to) in main_text_only_replacements:
            text = re.sub(pattern, to, text)
    return text


def generate_braveified_node(elem, is_comment, branding_replacements_only):
    """Replaces a node and attributes to Brave wording"""
    if elem.text:
        elem.text = braveify_grd_text(
            elem.text, not is_comment, branding_replacements_only)

    if elem.tail:
        elem.tail = braveify_grd_text(
            elem.tail, not is_comment, branding_replacements_only)

    if 'desc' in elem.keys():
        elem.attrib['desc'] = braveify_grd_text(
            elem.attrib['desc'], False, branding_replacements_only)
    for child in elem:
        generate_braveified_node(child, is_comment, branding_replacements_only)


def format_xml_style(xml_content):
    """Formats an xml file according to how Chromium GRDs are formatted"""
    xml_content = re.sub(rb'\s+desc="', rb' desc="', xml_content)
    xml_content = xml_content.replace(b'/>', b' />')
    xml_content = xml_content.replace(
        rb'<?xml version="1.0" encoding="UTF-8"?>',
            rb'<?xml version=\'1.0\' encoding=\'UTF-8\'?>')
    return xml_content


def write_xml_file_from_tree(string_path, xml_tree):
    """Writes out an xml tree to a file with Chromium GRD formatting
       replacements"""
    transformed_content = lxml.etree.tostring(xml_tree,
                                              pretty_print=True,
                                              xml_declaration=True,
                                              encoding='UTF-8')
    transformed_content = format_xml_style(transformed_content)
    with open(string_path, mode='wb') as f:
        f.write(transformed_content)


def update_braveified_grd_tree_override(source_xml_tree,
                                        branding_replacements_only):
    """Takes in a grd(p) tree and replaces all messages and comments with Brave
       wording"""
    for elem in source_xml_tree.xpath('//message'):
        generate_braveified_node(elem, False, branding_replacements_only)
    for elem in source_xml_tree.xpath('//comment()'):
        generate_braveified_node(elem, True, branding_replacements_only)


def write_braveified_grd_override(source_string_path):
    """Takes in a grd file and replaces all messages and comments with Brave
       wording"""
    source_xml_tree = lxml.etree.parse(source_string_path)
    update_braveified_grd_tree_override(source_xml_tree, False)
    write_xml_file_from_tree(source_string_path, source_xml_tree)


def get_override_file_path(source_string_path):
    """Obtain src/brave source string override path for local grd strings with
       replacements"""
    filename = os.path.basename(source_string_path)
    (basename, ext) = filename.split('.')
    if ext == 'xtb':
        # _override goes after the string name but before the _[locale].xtb part
        parts = basename.split('_')
        parts.insert(-1, 'override')
        override_string_path = os.path.join(os.path.dirname(source_string_path),
                                            '.'.join(('_'.join(parts), ext)))
    else:
        override_string_path = os.path.join(os.path.dirname(source_string_path),
            '.'.join((basename + '_override', ext)))
    return override_string_path


def update_xtbs_locally(grd_file_path, brave_source_root):
    """Updates XTBs from the local Chromium files"""
    xtb_files = get_xtb_files(grd_file_path)
    chromium_grd_file_path = get_chromium_grd_src_with_fallback(grd_file_path,
        brave_source_root)
    chromium_xtb_files = get_xtb_files(grd_file_path)
    if len(xtb_files) != len(chromium_xtb_files):
        assert False, 'XTB files and Chromium XTB file length mismatch.'

    grd_base_path = os.path.dirname(grd_file_path)
    chromium_grd_base_path = os.path.dirname(chromium_grd_file_path)

    # Update XTB FPs so it uses the branded source string
    grd_strings = get_grd_strings(grd_file_path, validate_tags=False)
    chromium_grd_strings = get_grd_strings(
        chromium_grd_file_path, validate_tags=False)
    assert len(grd_strings) == len(chromium_grd_strings)

    fp_map = {chromium_grd_strings[idx][2]: grd_strings[idx][2] for
              (idx, grd_string) in enumerate(grd_strings)}

    xtb_file_paths = [os.path.join(
        grd_base_path, path) for (_, path) in xtb_files]
    chromium_xtb_file_paths = [
        os.path.join(chromium_grd_base_path, path) for
        (_, path) in chromium_xtb_files]
    for idx, xtb_file in enumerate(xtb_file_paths):
        chromium_xtb_file = chromium_xtb_file_paths[idx]
        if not os.path.exists(chromium_xtb_file):
            print('Warning: Skipping because Chromium path does not exist: ' \
                  f'{chromium_xtb_file}')
            continue
        xml_tree = lxml.etree.parse(chromium_xtb_file)

        for node in xml_tree.xpath('//translation'):
            generate_braveified_node(node, False, True)
            # Use our fp, when exists.
            old_fp = node.attrib['id']
            # It's possible for an xtb string to not be in our GRD.
            # This happens, for exmaple, with Chrome OS strings which
            # we don't process files for.
            if old_fp in fp_map:
                new_fp = fp_map.get(old_fp)
                if new_fp != old_fp:
                    node.attrib['id'] = new_fp
                    # print(f'fp: {old_fp} -> {new_fp}')

        transformed_content = (b'<?xml version="1.0" ?>\n' +
            lxml.etree.tostring(xml_tree, pretty_print=True,
                xml_declaration=False, encoding='utf-8').strip())
        with open(xtb_file, mode='wb') as f:
            f.write(transformed_content)


def get_xtb_files(grd_file_path):
    """Obtains all the XTB files from the specified GRD"""
    all_xtb_file_tags = (
        lxml.etree.parse(grd_file_path).findall('.//translations/file'))
    xtb_files = []
    for xtb_file_tag in all_xtb_file_tags:
        lang = xtb_file_tag.get('lang')
        path = xtb_file_tag.get('path')
        pair = (lang, path)
        xtb_files.append(pair)
    return xtb_files


def get_grd_languages(grd_file_path):
    """Extracts the list of locales supported by the passed in GRD file"""
    xtb_files = get_xtb_files(grd_file_path)
    return {lang for (lang, _) in xtb_files}


def get_chromium_grd_src_with_fallback(grd_file_path, brave_source_root):
    source_root = os.path.dirname(brave_source_root)
    chromium_grd_file_path = get_original_grd(source_root, grd_file_path)
    if not chromium_grd_file_path:
        rel_path = os.path.relpath(grd_file_path, brave_source_root)
        chromium_grd_file_path = os.path.join(source_root, rel_path)
    return chromium_grd_file_path


def get_original_grd(src_root, grd_file_path):
    """Obtains the Chromium GRD file for a specified Brave GRD file."""
    # pylint: disable=fixme
    # TODO: consider passing this mapping into the script from l10nUtil.js
    grd_file_name = os.path.basename(grd_file_path)
    if grd_file_name == 'components_brave_strings.grd':
        return os.path.join(src_root, 'components',
                            'components_chromium_strings.grd')
    if grd_file_name == 'brave_strings.grd':
        return os.path.join(src_root, 'chrome', 'app', 'chromium_strings.grd')
    if grd_file_name == 'generated_resources.grd':
        return os.path.join(src_root, 'chrome', 'app',
                            'generated_resources.grd')
    if grd_file_name == 'android_chrome_strings.grd':
        return os.path.join(src_root, 'chrome', 'browser', 'ui', 'android',
                            'strings', 'android_chrome_strings.grd')
    return None


def get_grd_strings(grd_file_path, validate_tags=True):
    """Obtains a tuple of (name, value, FP, description) for each string in
       a GRD file"""
    strings = []
    # Keep track of duplicate mesasge_names
    dupe_dict = defaultdict(int)
    all_message_tags = get_grd_message_tags(grd_file_path)
    for message_tag in all_message_tags:
        # Skip translateable="false" strings
        if not is_translateable_string(grd_file_path, message_tag):
            continue
        message_name = message_tag.get('name')
        dupe_dict[message_name] += 1

        # Check for a duplicate message_name, this can happen, for example,
        # for the same message id but one is title case and the other isn't.
        # Both need to be uploaded to Transifex with different message names.
        # When XTB files are later generated, the ID doesn't matter at all.
        # The only thing that matters is the fingerprint string hash.
        if dupe_dict[message_name] > 1:
            message_name += f"_{dupe_dict[message_name]}"
        if validate_tags:
            message_xml = lxml.etree.tostring(
                message_tag, method='xml', encoding='utf-8')
            errors = validate_tags_in_one_string(
                lxml.etree.fromstring(message_xml), textify)
            assert errors is None, '\n' + errors
        message_desc = message_tag.get('desc') or ''
        message_value = textify(message_tag)
        assert message_name, 'Message name is empty'
        assert (message_name.startswith('IDS_') or
                message_name.startswith('PRINT_PREVIEW_MEDIA_')), \
            f'Invalid message ID: {message_name}'
        # None of the PRINT_PREVIEW_MEDIA_ messages currently get uploaded for
        # translation, but in case this changes let's keep the prefix in the
        # name (as opposed to IDS_ which we strip)
        if message_name.startswith('IDS_'):
            string_name = message_name[4:].lower()
        string_fp = get_fingerprint_for_xtb(message_tag)
        string_tuple = (string_name, message_value, string_fp, message_desc)
        strings.append(string_tuple)
    return strings


def get_grd_message_tags(grd_file_path):
    """Obtains all message tags of the specified GRD file"""
    output_elements = []
    elements = lxml.etree.parse(grd_file_path).findall('//message')
    for element in elements:
        if element.tag == 'message':
            output_elements.append(element)
        else:
            assert False, f'Unexpected tag name {element.tag}'

    elements = lxml.etree.parse(grd_file_path).findall('.//part')
    for element in elements:
        grd_base_path = os.path.dirname(grd_file_path)
        grd_part_filename = element.get('file')
        if grd_part_filename in ['chromeos_strings.grdp']:
            continue
        grd_part_path = os.path.join(grd_base_path, grd_part_filename)
        part_output_elements = get_grd_message_tags(grd_part_path)
        output_elements.extend(part_output_elements)

    return output_elements


def is_translateable_string(grd_file_path, message_tag):
    """ Checks translateable attribute of the given message and additionally
        certain exceptions"""
    if message_tag.get('translateable') != 'false':
        return True
    # Check for exceptions that aren't translateable in Chromium, but are made
    # to be translateable in Brave. These can be found in the main function in
    # brave/script/chromium-rebase-l10n.py
    grd_file_name = os.path.basename(grd_file_path)
    if grd_file_name == 'chromium_strings.grd':
        exceptions = {'IDS_SXS_SHORTCUT_NAME',
                      'IDS_SHORTCUT_NAME_BETA',
                      'IDS_SHORTCUT_NAME_DEV',
                      'IDS_APP_SHORTCUTS_SUBDIR_NAME_BETA',
                      'IDS_APP_SHORTCUTS_SUBDIR_NAME_DEV',
                      'IDS_INBOUND_MDNS_RULE_NAME_BETA',
                      'IDS_INBOUND_MDNS_RULE_NAME_CANARY',
                      'IDS_INBOUND_MDNS_RULE_NAME_DEV',
                      'IDS_INBOUND_MDNS_RULE_DESCRIPTION_BETA',
                      'IDS_INBOUND_MDNS_RULE_DESCRIPTION_CANARY',
                      'IDS_INBOUND_MDNS_RULE_DESCRIPTION_DEV'}
        if message_tag.get('name') in exceptions:
            return True
    return False


def get_fingerprint_for_xtb(message_tag):
    """Obtains the fingerprint meant for xtb files from a message tag."""
    string_to_hash = message_tag.text
    string_phs = message_tag.findall('ph')
    for string_ph in string_phs:
        string_to_hash = (
            (string_to_hash or '') + string_ph.get('name').upper() + (
                string_ph.tail or ''))
    string_to_hash = (string_to_hash or '').strip()
    string_to_hash = clean_triple_quoted_string(string_to_hash)
    fp = FP.FingerPrint(string_to_hash)
    meaning = (message_tag.get('meaning') if 'meaning' in message_tag.attrib
               else None)
    if meaning:
        # combine the fingerprints of message and meaning
        fp2 = FP.FingerPrint(meaning)
        if fp < 0:
            fp = fp2 + (fp << 1) + 1
        else:
            fp = fp2 + (fp << 1)
    # To avoid negative ids we strip the high-order bit
    return str(fp & 0x7fffffffffffffff)


def clean_triple_quoted_string(val):
    """Grit parses out first 3 and last 3 single quote chars if they exist."""
    val = val.strip()
    if val.startswith("'''"):
        val = val[3:]
    if val.endswith("'''"):
        val = val[:-3]
    return val.strip()


def textify(tag):
    """Returns the text content of a tag"""
    val = lxml.etree.tostring(tag, method='xml', encoding='unicode')
    val = val[val.index('>')+1:val.rindex('<')]
    val = clean_triple_quoted_string(val)
    return val
