import argparse
import io
import os.path
from os import walk
import re
import sys
from lxml import etree
from lib.config import get_env_var
from lib.grd_string_replacements import (write_xml_file_from_tree, write_braveified_grd_override,
                                         update_braveified_grd_tree_override, get_override_file_path)
from lib.transifex import pull_source_files_from_transifex, textify

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def parse_args():
    parser = argparse.ArgumentParser(description='Push strings to Transifex')
    parser.add_argument('--source_string_path', nargs=1)
    return parser.parse_args()


def generate_overrides_and_replace_strings(source_string_path):
    original_xml_tree_with_branding_fixes = etree.parse(source_string_path)
    update_braveified_grd_tree_override(original_xml_tree_with_branding_fixes, True)
    write_braveified_grd_override(source_string_path)
    modified_xml_tree = etree.parse(source_string_path)

    original_messages = original_xml_tree_with_branding_fixes.xpath('//message')
    modified_messages = modified_xml_tree.xpath('//message')
    assert len(original_messages) == len(modified_messages)
    for i in range(0, len(original_messages)):
        if textify(original_messages[i]) == textify(modified_messages[i]):
            modified_messages[i].getparent().remove(modified_messages[i])

    # Remove uneeded things from the override grds
    nodes_to_remove = [
        '//outputs',
        '//comment()',
    ]
    for xpath_expr in nodes_to_remove:
        nodes = modified_xml_tree.xpath(xpath_expr)
        for n in nodes:
            if n.getparent() is not None:
                n.getparent().remove(n)
    parts = modified_xml_tree.xpath('//part')
    for part in parts:
        override_file = get_override_file_path(part.attrib['file'])
        if os.path.exists(os.path.join(os.path.dirname(source_string_path), override_file)):
            part.attrib['file'] = override_file
        else:
            # No grdp override here, carry on
            part.getparent().remove(part)
    files = modified_xml_tree.xpath('//file')
    for f in files:
        f.attrib['path'] = get_override_file_path(f.attrib['path'])

    # Write out an override file that is a duplicate of the original file but with strings that
    # are shared with Chrome stripped out.
    filename = os.path.basename(source_string_path)
    (basename, ext) = filename.split('.')
    override_string_path = get_override_file_path(source_string_path)
    modified_messages = modified_xml_tree.xpath('//message')
    modified_parts = modified_xml_tree.xpath('//part')
    if len(modified_messages) > 0 or len(modified_parts) > 0:
        write_xml_file_from_tree(override_string_path, modified_xml_tree)


def main():
    args = parse_args()
    # This file path is a string path inside brave/ but just recently copied
    # in from chromium files which need replacements.
    source_string_path = os.path.join(SOURCE_ROOT, args.source_string_path[0])
    filename = os.path.basename(source_string_path)
    extension = os.path.splitext(source_string_path)[1]
    if (extension != '.grd' and extension != '.grdp'):
        print 'returning early'
        return

    print 'Rebasing source string file:', source_string_path
    print 'filename:', filename

    generate_overrides_and_replace_strings(source_string_path)

    # If you modify the translateable attribute then also update
    # is_translateable_string function in brave/script/lib/transifex.py.
    xml_tree = etree.parse(source_string_path)
    (basename, ext) = filename.split('.')
    if basename == 'brave_strings':
        elem1 = xml_tree.xpath('//message[@name="IDS_SXS_SHORTCUT_NAME"]')[0]
        elem1.text = 'Brave Nightly'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath('//message[@name="IDS_SHORTCUT_NAME_BETA"]')[0]
        elem1.text = 'Brave Beta'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath('//message[@name="IDS_SHORTCUT_NAME_DEV"]')[0]
        elem1.text = 'Brave Dev'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_APP_SHORTCUTS_SUBDIR_NAME_BETA"]')[0]
        elem1.text = 'Brave Apps'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_APP_SHORTCUTS_SUBDIR_NAME_DEV"]')[0]
        elem1.text = 'Brave Apps'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_NAME_BETA"]')[0]
        elem1.text = 'Brave Beta (mDNS-In)'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_NAME_CANARY"]')[0]
        elem1.text = 'Brave Nightly (mDNS-In)'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_NAME_DEV"]')[0]
        elem1.text = 'Brave Dev (mDNS-In)'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION"]')[0]
        elem1.attrib.pop('desc')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION_BETA"]')[0]
        elem1.text = 'Inbound rule for Brave Beta to allow mDNS traffic.'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION_CANARY"]')[0]
        elem1.text = 'Inbound rule for Brave Nightly to allow mDNS traffic.'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//message[@name="IDS_INBOUND_MDNS_RULE_DESCRIPTION_DEV"]')[0]
        elem1.text = 'Inbound rule for Brave Dev to allow mDNS traffic.'
        elem1.attrib.pop('desc')
        elem1.attrib.pop('translateable')
        elem1 = xml_tree.xpath(
            '//part[@file="settings_chromium_strings.grdp"]')[0]
        elem1.set('file', 'settings_brave_strings.grdp')

    if filename == 'extensions_strings':
        for child in xml_tree.getroot():
            if child.tag != 'outputs':
                continue
            ifElem = etree.Element('if')
            ifElem.set('expr', 'is_android')
            add_extensions_output(ifElem, 'af')
            add_extensions_output(ifElem, 'as')
            add_extensions_output(ifElem, 'az')
            add_extensions_output(ifElem, 'be')
            add_extensions_output(ifElem, 'bs')
            add_extensions_output(ifElem, 'eu')
            add_extensions_output(ifElem, 'fr-CA')
            add_extensions_output(ifElem, 'gl')
            add_extensions_output(ifElem, 'hy')
            add_extensions_output(ifElem, 'is')
            add_extensions_output(ifElem, 'ka')
            add_extensions_output(ifElem, 'kk')
            add_extensions_output(ifElem, 'km')
            add_extensions_output(ifElem, 'ky')
            add_extensions_output(ifElem, 'lo')
            add_extensions_output(ifElem, 'mk')
            add_extensions_output(ifElem, 'mn')
            add_extensions_output(ifElem, 'my')
            add_extensions_output(ifElem, 'ne')
            add_extensions_output(ifElem, 'or')
            add_extensions_output(ifElem, 'pa')
            add_extensions_output(ifElem, 'si')
            add_extensions_output(ifElem, 'sq')
            add_extensions_output(ifElem, 'ur')
            add_extensions_output(ifElem, 'uz')
            add_extensions_output(ifElem, 'zh-HK')
            add_extensions_output(ifElem, 'zu')
            child.append(ifElem)
            break

    if filename == 'ui_resources':
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_CLOSE_2"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_DEFAULT_FAVICON_32"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_NTP_DEFAULT_FAVICON"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_SIGNAL_0_BAR"]')

    if filename == 'theme_resources':
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_ACCESSIBILITY_CAPTIONS_PREVIEW_BACKGROUND"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_APP_WINDOW_CLOSE"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_BOOKMARK_BAR_APPS_SHORTCUT"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_CLOSE_1"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_DOWNLOADS_FAVICON"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_HELP_MENU"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_MANAGEMENT_FAVICON"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_PRERENDER"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_PROFILES_DICE_TURN_ON_SYNC"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_SAFETY_TIP_LOOKALIKE_ILLUSTRATION_DARK"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_SETTINGS_FAVICON"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_COOKIE_BLOCKING_ON_HEADER"]')
        applyIsAndroidOr(xml_tree, '//structure[@file="chromium/webstore_icon.png"]')
        applyIsAndroidOr(xml_tree, '//structure[@file="google_chrome/webstore_icon.png"]')

    if filename == 'browser_resources':
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_SIGNIN_SHARED_CSS_JS"]')
        applyIsAndroidOr(xml_tree, '//structure[@name="IDR_INCOGNITO_TAB_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_DISCARDS_MOJO_API_JS"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_BROWSER_SWITCH_APP_JS"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_ABOUT_SYS_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_PAGE_NOT_AVAILABLE_FOR_GUEST_APP_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_PLUGIN_DB_JSON"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_MANAGEMENT_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_SYNC_DISABLED_CONFIRMATION_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_SIGNIN_EMAIL_CONFIRMATION_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_CONTROL_BAR_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_IDENTITY_INTERNALS_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_MEDIA_ROUTER_INTERNALS_HTML"]')
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_TAB_RANKER_EXAMPLE_PREPROCESSOR_CONFIG_PB"]')
        elem = xml_tree.xpath('//structure[@name="IDR_INCOGNITO_TAB_HTML"]')[0]
        elem.set('file', 'resources/ntp4/incognito_tab.html')
        elem = xml_tree.xpath('//structure[@name="IDR_INCOGNITO_TAB_THEME_CSS"]')[0]
        elem.set('file', 'resources/ntp4/incognito_tab_theme.css')

    if filename == 'mojo_bindings_resources':
        applyIsAndroidOr(xml_tree, '//include[@name="IDR_MOJO_MOJO_BINDINGS_JS"]')

    grit_root = xml_tree.xpath(
        '//grit' if extension == '.grd' else '//grit-part')[0]
    previous_to_grit_root = grit_root.getprevious()
    comment_text = 'This file is created by l10nUtil.js. Do not edit manually.'
    if previous_to_grit_root is None or (
            previous_to_grit_root.text != comment_text):
        comment = etree.Comment(comment_text)
        grit_root.addprevious(comment)

    transformed_content = etree.tostring(xml_tree, pretty_print=True,
                                         xml_declaration=True,
                                         encoding='UTF-8')
    # Fix some minor formatting differences from what Chromium outputs
    transformed_content = (transformed_content.replace('/>', ' />'))
    print 'writing file ', source_string_path
    with open(source_string_path, mode='w') as f:
        f.write(transformed_content)
    print '-----------'

def add_extensions_output(ifElem, lang):
    outputElem = etree.Element('output')
    outputElem.set('filename', 'extensions_strings_' + lang + '.pak')
    outputElem.set('type', 'data_package')
    outputElem.set('lang', lang)
    ifElem.append(outputElem)

def applyIsAndroidOr(xml_tree, toFind):
    elem = xml_tree.xpath(toFind)[0]
    parent = elem.find('..')
    parent.set('expr', 'is_android or ' + parent.get('expr'))


if __name__ == '__main__':
    sys.exit(main())
