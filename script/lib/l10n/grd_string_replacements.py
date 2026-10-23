#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */


# Strings we want to replace but that we also replace automatically
# for XTB files
branding_replacements = [
    (
        r'The\sChromium\sAuthors.\sAll\srights\sreserved.',
        r'The Brave Authors. All rights reserved.',
    ),
    (
        r'Google\sLLC.\sAll\srights\sreserved.',
        r'The Brave Authors. All rights reserved.',
    ),
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


# Fix up some strings after aggressive first round replacement.
fixup_replacements = [
    (r'Brave Cloud Print', r'Google Cloud Print'),
    (r'Brave Docs', r'Google Docs'),
    (r'Brave Drive', r'Google Drive'),
    (r'Brave OS', r'Chrome OS'),
    (r'BraveOS', r'ChromeOS'),
    (r'Brave Safe Browsing', r'Google Safe Browsing'),
    (
        r'Safe Browsing \(protects you and your device from dangerous sites\)',
        r'Google Safe Browsing (protects you and your device from dangerous sites)',
    ),
    (
        r'Sends URLs of some pages you visit to Brave',
        r'Sends URLs of some pages you visit to Google',
    ),
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
    (r'Brave Wallet', r'Google Wallet'),
    (r'powered by Brave AI', r'powered by Google AI'),
    (
        r'Brave Extension developer documentation',
        r'Google Extension developer documentation',
    ),
]


# Declined or mistyped brand forms that the language-agnostic rules above miss,
# keyed by locale. Applied to XTB translations after the rules above.
language_branding_replacements = {
    'cs': [(r'\bChromu\b', r'Brave')],
    # Skip chrome:// and chrome-untrusted:// URLs and paths such as /chrome.
    'fa': [(r'(?<![/.\-])\bchrome\b(?![:\-./])', r'Brave')],
    # Chromium's translation starts with a Cyrillic C.
    'ky': [(r'\u0421hrome', r'Brave')],
    'sl': [
        (r'Brave Chroma\b', r'Brave'),
        (r'\bChroma\b', r'Brave'),
        (r'\bChromov\b', r'Braveov'),
    ],
}


# Descriptions for messages that Chromium leaves without one, so translators
# get context. Descriptions do not affect translation ids.
missing_descriptions = {
    'IDS_EXTENSION_EXTERNAL_INITIAL_INSTALL_PROMPT_TITLE_EXTENSION': (
        'Title of the prompt window for an extension added by a partner program'
    ),
    'IDS_EXTENSION_BAD_FILE_ENCODING': (
        'Error shown when a content script file of an extension is not UTF-8 '
        'encoded and cannot be loaded'
    ),
    'IDS_EXTENSION_LOAD_ABOUT_PAGE_FAILED': (
        'Error shown when the about page declared by an extension cannot be '
        'loaded'
    ),
    'IDS_EXTENSION_LOAD_BACKGROUND_SCRIPT_FAILED': (
        'Error shown when the background script declared by an extension '
        'cannot be loaded'
    ),
    'IDS_EXTENSION_LOAD_BACKGROUND_PAGE_FAILED': (
        'Error shown when the background page declared by an extension '
        'cannot be loaded'
    ),
    'IDS_EXTENSION_LOAD_CSS_FAILED': (
        'Error shown when a CSS file that an extension lists for a content '
        'script cannot be loaded'
    ),
    'IDS_EXTENSION_LOAD_JAVASCRIPT_FAILED': (
        'Error shown when a JavaScript file that an extension lists for a '
        'content script cannot be loaded'
    ),
    'IDS_EXTENSION_LOAD_OPTIONS_PAGE_FAILED': (
        'Error shown when the options page declared by an extension cannot be '
        'loaded'
    ),
    'IDS_EXTENSION_LOCALES_NO_DEFAULT_LOCALE_SPECIFIED': (
        'Error shown when an extension uses localization but its manifest '
        'does not specify a default locale'
    ),
    'IDS_EXTENSION_MANIFEST_UNREADABLE': (
        'Error shown when the manifest file of an extension is missing or '
        'cannot be read'
    ),
    'IDS_EXTENSION_MANIFEST_INVALID': (
        'Error shown when the manifest file of an extension is not valid'
    ),
    'IDS_EXTENSION_MOVE_DIRECTORY_TO_PROFILE_FAILED': (
        'Error shown when the directory of an extension cannot be moved into '
        'the user profile during installation'
    ),
}


# Replacements for text nodes and neither for inside descriptions nor comments
main_text_only_replacements = [
    # By converting it back first, it makes this idempotent
    ('Copyright \xa9', 'Copyright'),
    ('Copyright', 'Copyright \xa9'),
]


# Replacements for strings in brave_strings.grd for situations where using a
# different GRD would be impractical. These need to be translated in Crowdin.
brave_strings_grd_replacements = [
    (
        'IDS_LOCAL_NETWORK_ACCESS_PERMISSION_DESC',
        r'''
          This will allow you to share content from Brave to your local devices, such as a TV or speaker.
        ''',
    ),
]
