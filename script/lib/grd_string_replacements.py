from lxml import etree
import os
import re


# Strings we want to replace but that we also replace automatically for XTB files
branding_replacements = [
    (r'The Chromium Authors. All rights reserved.', r'The Brave Authors. All rights reserved.'),
    (r'Google LLC. All rights reserved.', r'The Brave Authors. All rights reserved.'),
    (r'The Chromium Authors', r'Brave Software Inc'),
    (r'Google Chrome', r'Brave'),
    (r'(Google)(?! Play)', r'Brave'),
    (r'Chromium', r'Brave'),
    (r'Chrome', r'Brave'),
]


# Strings we want to replace but that we need to use Transifex for
# to translate the XTB files
default_replacements = [
    (r'Brave Web Store', r'Web Store'),
    (r'Automatically send usage statistics and crash reports to Brave',
     r'Automatically send crash reports to Brave'),
    (r'Automatically sends usage statistics and crash reports to Brave',
     r'Automatically sends crash reports to Brave'),
    (r'You\'re incognito', r'This is a private window'),
    (r'an incognito', r'a private'),
    (r'an Incognito', r'a Private'),
    (r'incognito', r'private'),
    (r'Incognito', r'Private'),
    (r'inco&gnito', r'&private'),
    (r'Inco&gnito', r'&Private'),
    (r'People', r'Profiles'),
    # 'people' but only in the context of profiles, not humans.
    (r'(?<!authenticate )people', r'profiles'),
    (r'(Person)(?!\w)', r'Profile'),
    (r'(person)(?!\w)', r'profile'),
    (r'Bookmarks Bar\n', r'Bookmarks\n'),
    (r'Bookmarks bar\n', r'Bookmarks\n'),
    (r'bookmarks bar\n', r'bookmarks\n'),
]


# Fix up some strings after aggressive first round replacement.
fixup_replacements = [
    (r'Brave Cloud Print', r'Google Cloud Print'),
    (r'Brave Docs', r'Google Docs'),
    (r'Brave Drive', r'Google Drive'),
    (r'Brave OS', r'Chrome OS'),
    (r'Brave Safe Browsing', r'Google Safe Browsing'),
    (r'Safe Browsing \(protects you and your device from dangerous sites\)',
     r'Google Safe Browsing (protects you and your device from dangerous sites)'),
    (r'Sends URLs of some pages you visit to Brave', r'Sends URLs of some pages you visit to Google'),
    (r'Google Google', r'Google'),
]


# Replacements for text nodes and neither for inside descriptions nor comments
main_text_only_replacements = [
    (r'Copyright', u'Copyright \xa9'),
]


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
        elem.text = braveify_grd_text(elem.text, not is_comment, branding_replacements_only)

    if elem.tail:
        elem.tail = braveify_grd_text(elem.tail, not is_comment, branding_replacements_only)

    if 'desc' in elem.keys():
        elem.attrib['desc'] = braveify_grd_text(elem.attrib['desc'], False, branding_replacements_only)
    if 'meaning' in elem.keys():
        elem.attrib['meaning'] = braveify_grd_text(elem.attrib['meaning'], False, branding_replacements_only)
    for child in elem:
        generate_braveified_node(child, is_comment, branding_replacements_only)


def format_xml_style(xml_content):
    """Formats an xml file according to how Chromium GRDs are formatted"""
    xml_content = re.sub(r'\s+desc="', r' desc="', xml_content)
    xml_content = xml_content.replace('/>', ' />')
    xml_content = xml_content.replace(r'<?xml version="1.0" encoding="UTF-8"?>',
                                      r'<?xml version=\'1.0\' encoding=\'UTF-8\'?>')
    return xml_content


def write_xml_file_from_tree(string_path, xml_tree):
    """Writes out an xml tree to a file with Chromium GRD formatting replacements"""
    transformed_content = etree.tostring(xml_tree,
                                         pretty_print=True,
                                         xml_declaration=True,
                                         encoding='UTF-8')
    transformed_content = format_xml_style(transformed_content)
    with open(string_path, mode='w') as f:
        f.write(transformed_content)


def update_braveified_grd_tree_override(source_xml_tree, branding_replacements_only):
    """Takes in a grd(p) tree and replaces all messages and comments with Brave wording"""
    for elem in source_xml_tree.xpath('//message'):
        generate_braveified_node(elem, False, branding_replacements_only)
    for elem in source_xml_tree.xpath('//comment()'):
        generate_braveified_node(elem, True, branding_replacements_only)


def write_braveified_grd_override(source_string_path):
    """Takes in a grd file and replaces all messages and comments with Brave wording"""
    source_xml_tree = etree.parse(source_string_path)
    update_braveified_grd_tree_override(source_xml_tree, False)
    write_xml_file_from_tree(source_string_path, source_xml_tree)


def get_override_file_path(source_string_path):
    """Obtain src/brave source string override path for local grd strings with replacements"""
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
