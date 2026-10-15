#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# /// script
# dependencies = [
#   'lxml==4.9.3'
# ]
# ///
"""Tests for grd_utils.py. Run with: vpython3 grd_utils_test.py"""

import os
import re
import shutil
import sys
import tempfile
import unittest

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BRAVE_SOURCE_ROOT = os.path.dirname(
    os.path.dirname(os.path.dirname(SCRIPT_DIR))
)
SRC_SOURCE_ROOT = os.path.dirname(BRAVE_SOURCE_ROOT)

sys.path.insert(0, os.path.join(BRAVE_SOURCE_ROOT, 'script'))
sys.path.insert(0, os.path.join(SRC_SOURCE_ROOT, 'tools/grit/grit/extern'))

# pylint: disable=wrong-import-position
import lxml.etree  # pylint: disable=import-error
from lib.l10n.grd_utils import (
    braveify_grd_tree,
    braveify_language_forms,
    update_xtbs_locally,
)

GRD_TEMPLATE = """<?xml version="1.0" encoding="UTF-8"?>
<grit latest_public_release="0" current_release="1" source_lang_id="en">
  <translations>
    <file path="strings/test_strings_fr.xtb" lang="fr" />
  </translations>
  <release seq="1">
    <messages>
{messages}
    </messages>
  </release>
</grit>
"""

MESSAGE_TEMPLATE = '      <message name="{name}" desc="d">{text}</message>'

XTB_TEMPLATE = """<?xml version="1.0" ?>
<!DOCTYPE translationbundle>
<translationbundle lang="fr">
{translations}
</translationbundle>
"""


def fingerprint(grd_path, name):
    """Returns the XTB id of a message by name, via get_grd_strings()."""
    # pylint: disable=import-outside-toplevel
    from lib.l10n.grd_utils import get_grd_strings

    string_name = name[len('IDS_') :].lower()
    for string_tuple in get_grd_strings(grd_path, validate_tags=False):
        if string_tuple[0] == string_name:
            return string_tuple[2]
    raise AssertionError(f'{name} not found in {grd_path}')


class UpdateXtbsLocallyTest(unittest.TestCase):
    def setUp(self):
        self.src_root = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.src_root)
        self.brave_root = os.path.join(self.src_root, 'brave')

    def write(self, path, content):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(content)

    def write_grd(self, root, messages):
        body = '\n'.join(
            MESSAGE_TEMPLATE.format(name=name, text=text)
            for name, text in messages
        )
        path = os.path.join(root, 'test_strings.grd')
        self.write(path, GRD_TEMPLATE.format(messages=body))
        return path

    def test_identical_after_branding_keeps_first_translation(self):
        # Chromium and Chrome variants have different ids upstream but the same
        # text once both are rebranded.
        chromium_grd = self.write_grd(
            self.src_root,
            [
                ('IDS_REMOVE_CHROMIUM', 'Remove from Chromium'),
                ('IDS_REMOVE_CHROME', 'Remove from Chrome'),
                ('IDS_OTHER', 'Other'),
            ],
        )
        brave_grd = self.write_grd(
            self.brave_root,
            [
                ('IDS_REMOVE_CHROMIUM', 'Remove from Brave'),
                ('IDS_REMOVE_CHROME', 'Remove from Brave'),
                ('IDS_OTHER', 'Other'),
            ],
        )
        chromium_ids = {
            name: fingerprint(chromium_grd, name)
            for name in (
                'IDS_REMOVE_CHROMIUM',
                'IDS_REMOVE_CHROME',
                'IDS_OTHER',
            )
        }
        self.assertEqual(len(set(chromium_ids.values())), 3)
        brave_id = fingerprint(brave_grd, 'IDS_REMOVE_CHROMIUM')
        self.assertEqual(brave_id, fingerprint(brave_grd, 'IDS_REMOVE_CHROME'))

        translations = '\n'.join(
            [
                f'<translation id="{chromium_ids["IDS_REMOVE_CHROMIUM"]}">'
                'Supprimer de Chromium</translation>',
                f'<translation id="{chromium_ids["IDS_REMOVE_CHROME"]}">'
                'Retirer de Chrome</translation>',
                f'<translation id="{chromium_ids["IDS_OTHER"]}">'
                'Autre</translation>',
            ]
        )
        self.write(
            os.path.join(self.src_root, 'strings', 'test_strings_fr.xtb'),
            XTB_TEMPLATE.format(translations=translations),
        )
        brave_xtb = os.path.join(
            self.brave_root, 'strings', 'test_strings_fr.xtb'
        )
        self.write(brave_xtb, XTB_TEMPLATE.format(translations=''))

        update_xtbs_locally(brave_grd, self.brave_root, None)

        with open(brave_xtb, encoding='utf-8') as f:
            result = f.read()
        ids = re.findall(r'<translation id="(\d+)"', result)
        self.assertEqual(len(ids), len(set(ids)))
        self.assertEqual(ids.count(brave_id), 1)
        self.assertIn('Supprimer de Brave', result)
        self.assertNotIn('Retirer de', result)
        self.assertIn('Autre', result)


class BraveifyMissingDescriptionTest(unittest.TestCase):
    def braveify(self, desc_attribute):
        name = 'IDS_EXTENSION_EXTERNAL_INITIAL_INSTALL_PROMPT_TITLE_EXTENSION'
        tree = lxml.etree.fromstring(
            f'<grit-part><message name="{name}"{desc_attribute}>'
            'Chrome works</message></grit-part>'
        )
        braveify_grd_tree(tree, False)
        return tree.find('message')

    def test_fills_empty_description(self):
        message = self.braveify(' desc=""')
        self.assertIn('partner program', message.get('desc'))
        self.assertEqual(message.text, 'Brave works')

    def test_keeps_existing_description(self):
        self.assertEqual(
            self.braveify(' desc="Chrome title"').get('desc'), 'Brave title'
        )


class BraveifyLanguageFormsTest(unittest.TestCase):
    def braveify(self, lang, text):
        node = lxml.etree.fromstring(f'<translation>{text}</translation>')
        braveify_language_forms(node, lang)
        return node.text

    def test_replaces_declined_and_mistyped_forms(self):
        self.assertEqual(
            self.braveify('sl', 'Prilagajanje Brave Chroma'),
            'Prilagajanje Brave',
        )
        self.assertEqual(
            self.braveify('sl', 'Chromov predlog'), 'Braveov predlog'
        )
        self.assertEqual(self.braveify('cs', 'funkce Chromu.'), 'funkce Brave.')
        self.assertEqual(self.braveify('fa', 'کنید chrome را'), 'کنید Brave را')

    def test_leaves_chrome_urls_alone(self):
        for text in (
            'chrome://flags',
            'chrome-untrusted://terminal',
            'google.com/chrome',
            'chrome.google.com',
        ):
            self.assertEqual(self.braveify('fa', text), text)
        self.assertEqual(self.braveify('ky', '\u0421hrome\'ду'), "Brave'ду")

    def test_leaves_other_locales_alone(self):
        self.assertEqual(self.braveify('de', 'Chroma Chromu'), 'Chroma Chromu')


class GeneratedExtensionsStringsTest(unittest.TestCase):
    def test_every_message_has_a_description(self):
        grd = os.path.join(
            BRAVE_SOURCE_ROOT, 'extensions', 'strings', 'extensions_strings.grd'
        )
        empty = [
            message.get('name')
            for message in lxml.etree.parse(grd).iter('message')
            if not (message.get('desc') or '').strip()
        ]
        self.assertEqual(empty, [])


if __name__ == '__main__':
    unittest.main()
