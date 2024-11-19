#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */


# Strings we want to replace but that we also replace automatically
# for XTB files
branding_replacements = [
    (r'The Chromium Authors. All rights reserved.',
     r'The Brave Authors. All rights reserved.'),
    (r'Google LLC. All rights reserved.',
     r'The Brave Authors. All rights reserved.'),
    (r'The Chromium Authors', r'Brave Software Inc'),
    (r'Google Chrome', r'Brave'),
    (r'(Google)(?! Play)', r'Brave'),
    (r'Chromium', r'Brave'),
    (r'Chrome', r'Brave'),
    (r'क्रोमियम', r'Brave'),  # Chromium in Hindi
]


# Strings we want to replace but that we need to use Transifex for
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
    (r'Invite profiles', r'Invite people'),
    (r'Profiles & Society', r'People & Society'),
    (r'help profiles', r'help people'),
    (r'Most profiles', r'Most people'),
    (r'files with profiles', r'files with people'),
    (r'harder for profiles', r'harder for people'),
    (r'better for profiles', r'better for people'),
    (r'share with profiles', r'share with people'),
    (r'free to profiles', r'free to people'),
    (r'programs in which profiles', r'programs in which people'),
    (r'famous profiles', r'famous people'),
    (r'ordinary profiles', r'ordinary people'),
    (r'manufacturers and other profiles', r'manufacturers and other people'),
    (r'use by everyday profiles', r'use by everyday people'),
    (r'prepare taxes for profiles', r'prepare taxes for people'),
    (r'scientific study of profiles', r'scientific study of people'),
    (r'children and young profiles', r'children and young people'),
    (r'sport in which two profiles', r'sport in which two people'),
    (r'group of profiles', r'group of people'),
    (r'keeps unwanted profiles', r'keeps unwanted people'),
    (r'a real profile', r'a real person'),
    (r'The profile who set up', r'The person who set up'),
    (r'name could be a profile', r'name could be a person'),
    (r'name identifies the profile', r'name identifies the person'),
    (r'the insured profile', r'the insured person'),
    (r'programs where one profile', r'programs where one person'),
    (r'Ask in profile', r'Ask in person'),
]


# Replacements for text nodes and neither for inside descriptions nor comments
main_text_only_replacements = [
    # By converting it back first, it makes this idempotent
    ('Copyright \xa9', 'Copyright'),
    ('Copyright', 'Copyright \xa9'),
]
