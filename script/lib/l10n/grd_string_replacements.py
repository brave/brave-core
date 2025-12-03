#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */


def get_branding_replacements(brand):
    """Returns branding replacements for the specified brand.

    Args:
        brand: The brand name ('brave' or 'brave_origin')
    """
    product_name = 'Brave Origin' if brand == 'brave_origin' else 'Brave'
    return [
        (r'The\sChromium\sAuthors.\sAll\srights\sreserved.',
         r'The Brave Authors. All rights reserved.'),
        (r'Google\sLLC.\sAll\srights\sreserved.',
         r'The Brave Authors. All rights reserved.'),
        (r'The\sChromium\sAuthors', r'Brave Software Inc'),
        (r'Google\sLLC', r'Brave Software Inc'),
        (r'Google\sChrome', product_name),
        (r'(Google)(?!\sPlay)', product_name),
        (r'Chromium', product_name),
        (r'Chrome', product_name),
        (r'क्रोमियम', product_name),  # Chromium in Hindi
    ]


# Strings we want to replace but that we also replace automatically
# for XTB files (default to 'brave' brand for backwards compatibility)
branding_replacements = get_branding_replacements('brave')


def get_default_replacements(brand):
    """Returns default replacements for the specified brand.

    Args:
        brand: The brand name ('brave' or 'brave_origin')
    """
    product_name = 'Brave Origin' if brand == 'brave_origin' else 'Brave'
    return [
        (product_name + r' Web Store', r'Web Store'),
        (r'You\'re incognito', r'This is a private window'),
        (r'an incognito', r'a private'),
        (r'an Incognito', r'a Private'),
        (r'incognito', r'private'),
        (r'Incognito', r'Private'),
        (r'inco&gnito', r'&private'),
        (r'Inco&gnito', r'&Private'),
        (r'Bookmarks Bar\n', r'Bookmarks\n'),
        (r'Bookmarks bar\n', r'Bookmarks\n'),
        (r'bookmarks bar\n', r'bookmarks\n'),
    ]


# Strings we want to replace but that we need to use Crowdin for
# to translate the XTB files (default to 'brave' brand for backwards compat)
default_replacements = get_default_replacements('brave')


def get_fixup_replacements(brand):
    """Returns fixup replacements for the specified brand.

    These fix up some strings after aggressive first round replacement.

    Args:
        brand: The brand name ('brave' or 'brave_origin')
    """
    product_name = 'Brave Origin' if brand == 'brave_origin' else 'Brave'
    # For multi-word brands like "Brave Origin", match both spaced version
    # (Brave OriginOS) and no-space version (BraveOriginOS)
    product_nospace = product_name.replace(' ', '')
    replacements = [
        (product_name + r' Cloud Print', r'Google Cloud Print'),
        (product_name + r' Docs', r'Google Docs'),
        (product_name + r' Drive', r'Google Drive'),
        (product_name + r' OS', r'Chrome OS'),
        (product_nospace + r'OS', r'ChromeOS'),
        (product_name + r' Safe Browsing', r'Google Safe Browsing'),
        (r'Safe Browsing \(protects you and your device from dangerous sites\)',
         r'Google Safe Browsing (protects you and your device from dangerous '
         r'sites)'),
        (r'Sends URLs of some pages you visit to ' + product_name,
         r'Sends URLs of some pages you visit to Google'),
        (r'Google Google', r'Google'),
        (product_name + r' Account', product_name + r' sync chain'),
        (product_name + r' Lens', r'Google Lens'),
        (product_nospace + r'book', r'Chromebook'),
        (product_nospace + r'cast', r'Chromecast'),
        (product_name + r' Cloud', r'Google Cloud'),
        (product_name + r' Pay', r'Google Pay'),
        (product_name + r' Photos', r'Google Photos'),
        (product_name + r' Projects', r'Chromium Projects'),
        (product_name + r' Root Program', r'Chrome Root Program'),
        (product_nospace + r'Vox', r'ChromeVox'),
        (r'powered by ' + product_name + r' AI', r'powered by Google AI'),
    ]
    # For multi-word brands, also handle variants like "Brave OriginOS"
    if ' ' in product_name:
        # Handle patterns like "Brave OriginOS" (space before OS suffix)
        parts = product_name.split(' ')
        replacements.extend([
            (parts[0] + r' ' + parts[1] + r'OS', r'ChromeOS'),
            (parts[0] + r' ' + parts[1] + r'book', r'Chromebook'),
            (parts[0] + r' ' + parts[1] + r'cast', r'Chromecast'),
            (parts[0] + r' ' + parts[1] + r'Vox', r'ChromeVox'),
        ])
    return replacements


# Fix up some strings after aggressive first round replacement
# (default to 'brave' brand for backwards compatibility)
fixup_replacements = get_fixup_replacements('brave')


# Replacements for text nodes and neither for inside descriptions nor comments
main_text_only_replacements = [
    # By converting it back first, it makes this idempotent
    ('Copyright \xa9', 'Copyright'),
    ('Copyright', 'Copyright \xa9'),
]
