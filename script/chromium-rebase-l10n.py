import argparse
import io
import os.path
from os import walk
import sys
from lxml import etree
from lib.config import get_env_var
from lib.transifex import pull_source_files_from_transifex

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def parse_args():
    parser = argparse.ArgumentParser(description='Push strings to Transifex')
    parser.add_argument('--source_string_path', nargs=1)
    return parser.parse_args()


def main():
    args = parse_args()
    source_string_path = os.path.join(SOURCE_ROOT, args.source_string_path[0])
    filename = os.path.basename(source_string_path).split('.')[0]
    extension = os.path.splitext(source_string_path)[1]
    if (extension != '.grd' and extension != '.grdp'):
        print 'returning early'
        return

    print 'Rebasing source string file:', source_string_path
    print 'filename:', filename

    content = ''
    xml_tree = etree.parse(source_string_path)
    # If you modify the translateable attribute then also update
    # is_translateable_string function in brave/script/lib/transifex.py.
    if filename == 'brave_strings':
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


if __name__ == '__main__':
    sys.exit(main())
