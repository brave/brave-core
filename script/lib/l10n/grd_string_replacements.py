#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */


# Strings we want to replace but that we also replace automatically
# for XTB files
branding_replacements = [
    (r'The\sChromium\sAuthors.\sAll\srights\sreserved.',
     r'The Brave Authors. All rights reserved.'),
    (r'Google\sLLC.\sAll\srights\sreserved.',
     r'The Brave Authors. All rights reserved.'),
    (r'The\sChromium\sAuthors', r'Brave Software Inc'),
    (r'Google\sChrome', r'Brave'),
    (r'(Google)(?!\sPlay)', r'Brave'),
    (r'Chromium', r'Brave'),
    (r'Chrome', r'Brave'),
    (r'क्रोमियम', r'Brave'),  # Chromium in Hindi
]


# Strings we want to replace but that we need to use Crowdin for
# to translate the XTB files
default_replacements = [
    (r'Brave Web Store', r'Web Store'),
    (r'You\'re incognito', r'This is a private window'),
    (r'an incognito', r'a private'),
    (r'an Incognito', r'a Private'),
    (r'incognito', r'private'),
    (r'Incognito', r'Private'),
    (r'inco&gnito', r'&private'),
    (r'Inco&gnito', r'&Private'),
]


# Strings we want to replace only in a specific source string file, keyed by
# the file name. Like `default_replacements`, these need Crowdin to translate
# the XTB files.
per_file_replacements = {
    # Upstream names Gemini as the agent that works on the user's task; in
    # Brave that agent is Leo. Scoped to this file because the same wording in
    # the Gemini panel strings (e.g. glic_strings.grdp) really is about Gemini,
    # as are the Gemini entry point, "Gemini in Chrome" and "Gemini Spark"
    # mentions left untouched in this file.
    'actor_strings.grdp': [
        (r'Gemini\sis\sworking\son\syour\stask',
         r'Leo is working on your task'),
        (r'Gemini\sis\scurrently\sin\scontrol',
         r'Leo is currently in control'),
        (r'Gemini\sneeds\syour\shelp', r'Leo needs your help'),
        (r'Gemini\sstopped\sworking', r'Leo stopped working'),
        (r'Gemini\scompleted\syour\stask', r'Leo completed your task'),
        (r'Gemini\swill\sstop', r'Leo will stop'),
        (r'Gemini\shas\sbeen\sworking', r'Leo has been working'),
        (r'wait\sfor\sGemini\sto\sfinish', r'wait for Leo to finish'),
        # Descriptions of the strings above, so that translators don't get
        # Gemini as the context for Leo's text.
        (r'shared\swith\sGemini', r'shared with Leo'),
        (r'the\sGemini\sis\sworking\son\stab', r'Leo is working on tab'),
    ],
}


# Fix up some strings after aggressive first round replacement.
fixup_replacements = [
    (r'Brave Cloud Print', r'Google Cloud Print'),
    (r'Brave Docs', r'Google Docs'),
    (r'Brave Drive', r'Google Drive'),
    (r'Brave OS', r'Chrome OS'),
    (r'BraveOS', r'ChromeOS'),
    (r'Brave Safe Browsing', r'Google Safe Browsing'),
    (r'Safe Browsing \(protects you and your device from dangerous sites\)',
     r'Google Safe Browsing (protects you and your device from dangerous sites)'
     ),
    (r'Sends URLs of some pages you visit to Brave',
     r'Sends URLs of some pages you visit to Google'),
    (r'Google Google', r'Google'),
    (r'Brave Account', r'Brave sync chain'),
    (r'Brave Lens', r'Google Lens'),
    (r'Bravebook', r'Chromebook'),
    (r'Bravecast', r'Chromecast'),
    (r'Brave Cloud', r'Google Cloud'),
    (r'Brave Pay', r'Google Pay'),
    (r'Brave Photos', r'Google Photos'),
    (r'Brave Projects', r'Chromium Projects'),
    (r'Brave Root Program', r'Chrome Root Program'),
    (r'BraveVox', r'ChromeVox'),
    (r'powered by Brave AI', r'powered by Google AI'),
    (r'Brave Extension developer documentation',
     r'Google Extension developer documentation'),
]


# Replacements for text nodes and neither for inside descriptions nor comments
main_text_only_replacements = [
    # By converting it back first, it makes this idempotent
    ('Copyright \xa9', 'Copyright'),
    ('Copyright', 'Copyright \xa9'),
]


# Replacements for strings in brave_strings.grd for situations where using a
# different GRD would be impractical. These need to be translated in Crowdin.
brave_strings_grd_replacements = [
    ('IDS_LOCAL_NETWORK_ACCESS_PERMISSION_DESC', r'''
          This will allow you to share content from Brave to your local devices, such as a TV or speaker.
        '''),
]
