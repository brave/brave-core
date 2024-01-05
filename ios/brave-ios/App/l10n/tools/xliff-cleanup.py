#!/usr/bin/env python3

#
# xliff-cleanup.py <files>
#
#  1. Look at all remaining <file> sections and remove those strings that should not
#     be localized. Currently that means: CFBundleName and CFBundleShortVersionString.
#
#  2. Remove all remaining <file> sections that are now have no <trans-unit> nodes
#     in their <body> anymore.
#
# Modifies files in place. Makes no backup.
#

import sys

from lxml import etree

NS = {'x': 'urn:oasis:names:tc:xliff:document:1.2'}

STRINGS_TO_REMOVE = ('CFBundleName', 'CFBundleShortVersionString',
                     'CFBundleDisplayName')

REMOVE_FILES = []

if __name__ == "__main__":
    for path in sys.argv[1:]:
        # Read it in and modify it in memory
        with open(path, 'rb') as fp:
            tree = etree.parse(fp)
            root = tree.getroot()

            for file_node in root.xpath("//x:file", namespaces=NS):
                original = file_node.get('original')
                if original and any(x in original for x in REMOVE_FILES):
                    file_node.getparent().remove(file_node)

            # 1. Remove strings we don't want to be translated
            for file_node in root.xpath("//x:file", namespaces=NS):
                original = file_node.get('original')
                if original and original.endswith('InfoPlist.strings'):
                    for trans_unit_node in file_node.xpath(
                            "./x:body/x:trans-unit", namespaces=NS):
                        id = trans_unit_node.get('id')
                        if id and id in STRINGS_TO_REMOVE:
                            trans_unit_node.getparent().remove(trans_unit_node)

            # 2. Remove empty file sections
            for file_node in root.xpath("//x:file", namespaces=NS):
                original = file_node.get('original')
                if original and original.endswith('Info.plist'):
                    trans_unit_nodes = file_node.xpath("x:body/x:trans-unit",
                                                       namespaces=NS)
                    if len(trans_unit_nodes) == 0:
                        file_node.getparent().remove(file_node)
        # Write it back to the same file
        with open(path, "wb") as fp:
            fp.write(etree.tostring(tree, encoding='UTF-8'))
