# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Finds upstream GRD/GRDP files whose strings Brave would rebrand but that
are not in the localization mapping."""

import posixpath
import re

import lxml.etree  # pylint: disable=import-error

from lib.l10n.grd_string_replacements import (
    branding_replacements,
    fixup_replacements,
)

# Directories whose strings are not part of any Brave build.
EXCLUDED_PREFIXES = (
    'ios/',
    'ash/',
    'chromeos/',
    'ui/chromeos/',
    'chrome/browser/resources/chromeos/',
    'remoting/',
    'chromecast/',
    'tools/',
    'third_party/',
)

EXCLUDED_SUBSTRINGS = ('/testdata/', '/tests/', '/examples/', '/test/')

# Files that contain brand names but must not be rebranded or are managed
# elsewhere. Every entry needs a reason.
ALLOWLIST = {
    'chrome/app/google_chrome_strings.grd': 'Chrome-branded builds only',
    'chrome/app/settings_google_chrome_strings.grdp': (
        'Chrome-branded builds only'
    ),
    'components/components_google_chrome_strings.grd': (
        'Chrome-branded builds only'
    ),
    'chrome/app/chromeos_strings.grdp': (
        'ChromeOS only; excluded in l10nUtil.js'
    ),
    'chrome/app/brave_strings.grd': 'Brave-owned file',
    'chrome/app/brave_origin_strings.grd': 'Brave-owned file',
    'chrome/app/settings_brave_strings.grdp': 'Brave-owned file',
    'components/components_brave_strings.grd': 'Brave-owned file',
    'components/components_brave_origin_strings.grd': 'Brave-owned file',
    'chrome/credential_provider/gaiacp/gaia_resources.grd': (
        'Windows Google Credential Provider, not shipped'
    ),
    'components/omnibox/resources/omnibox_pedal_synonyms.grd': (
        'Search keywords that are never displayed'
    ),
    'ui/strings/auto_image_annotation_strings.grd': (
        'Names the Google image annotation service'
    ),
    'content/public/android/java/strings/android_content_strings.grd': (
        'Names the Google speech service'
    ),
    'chrome/browser/indigo/resources/indigo_strings.grd': (
        'Names Google as the content generator'
    ),
}


def _message_text(message):
    """Returns a message's text with placeholders replaced by their names, so
    brand names in <ex> examples are ignored."""
    parts = [message.text or '']
    for child in message:
        if child.tag == 'ph':
            parts.append(f"<{child.get('name')}>")
        else:
            parts.append(_message_text(child))
        parts.append(child.tail or '')
    return ' '.join(''.join(parts).split())


def _rebrand(text):
    for pattern, replacement in branding_replacements:
        text = re.sub(pattern, replacement, text)
    for pattern, replacement in fixup_replacements:
        text = re.sub(pattern, replacement, text)
    return text


def get_changed_message_names(grd_content):
    """Returns the names of translateable messages that branding changes."""
    root = lxml.etree.fromstring(grd_content)
    names = []
    for message in root.iter('message'):
        if message.get('translateable') == 'false':
            continue
        text = _message_text(message)
        if _rebrand(text) != text:
            names.append(message.get('name'))
    return names


def is_excluded(path):
    return path.startswith(EXCLUDED_PREFIXES) or any(
        part in '/' + path for part in EXCLUDED_SUBSTRINGS
    )


def find_unmapped(paths, read_file, mapped, allowlist=None):
    """Returns {path: [message names]} for each upstream GRD/GRDP in `paths`
    that is not mapped, excluded or allowlisted but has strings branding
    changes. `read_file` returns a path's content as bytes."""
    allowlist = ALLOWLIST if allowlist is None else allowlist
    mapped = {posixpath.normpath(path) for path in mapped}
    found = {}
    for path in sorted(paths):
        if not path.endswith(('.grd', '.grdp')):
            continue
        if path in mapped or path in allowlist or is_excluded(path):
            continue
        names = get_changed_message_names(read_file(path))
        if names:
            found[path] = names
    return found
