#!/usr/bin/env python3
# Copyright (c) 2020 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import json
import os
import sys

from io import open
from lib.config import SOURCE_ROOT


KNOWN_MISSING = [
    # Emailed author (henrik@schack.dk) on 2019-11-05.
    os.path.join('components', 'third_party',
                 'adblock', 'lists', 'adblock_dk'),
    # https://github.com/gfmaster/adblock-korea-contrib/issues/47
    os.path.join('components', 'third_party', 'adblock',
                 'lists', 'adblock_korea_contrib'),
]


def extract_license_info(directory):
    readme_path = os.path.join(directory, 'README.chromium')
    if not os.path.isfile(readme_path):
        print('Missing README.chromium in %s' % directory)
        sys.exit(1)

    metadata = {
        'slug': os.path.basename(directory),
        'Name': None,
        'URL': None,
        'License': None,
        'License File': None,
        'License Text': None,
    }

    with open(readme_path, mode='rt', encoding='utf-8') as file_handle:
        for line in file_handle:
            for field in metadata:
                if '%s:' % field in line:
                    metadata[field] = line[len(field) + 1:].strip()
                    break

    if not metadata['License File']:
        license_path = os.path.join(directory, 'LICENSE')
        if os.path.isfile(license_path):
            with open(license_path, mode='rt', encoding='utf-8') as file_handle:
                metadata['License Text'] = file_handle.read()
        elif metadata['License'] == 'unknown':
            relative_dir = directory[len(SOURCE_ROOT) + 1:]
            if relative_dir not in KNOWN_MISSING:
                print('Unknown license is not whitelisted: %s' % relative_dir)
                sys.exit(1)
        else:
            print(metadata)
            print('Missing LICENSE in %s' % directory)
            sys.exit(1)

    return metadata


def read_license_text(filename):
    # README.chromium files assume UNIX path separators.
    full_path = os.path.dirname(SOURCE_ROOT)
    for piece in filename.split('/'):
        full_path = os.path.join(full_path, piece)

    if not os.path.isfile(full_path):
        print('License file not found: %s' % full_path)
        sys.exit(1)

    with open(full_path, mode='rt', encoding='utf-8') as file_handle:
        return file_handle.read()


def external_component_license_file(preamble, components):
    component_notices = ''
    component_licenses = ''

    licenses = {}
    for component in components:
        if component_notices:
            component_notices += '\n'

        license_id = component['License']
        license_text = component['License Text']
        if license_text:
            # Custom license
            license_id += '-' + component['slug']

        component_notices += 'Name: %s\nURL: %s\nLicense: %s\n' % (
            component['Name'], component['URL'], license_id)

        if license_id == 'unknown':
            continue

        if license_id not in licenses:
            if license_text:
                licenses[license_id] = license_text
            else:
                licenses[license_id] = read_license_text(
                    component['License File'])

    for license_id in licenses:
        component_licenses += '--------------------------------------------------------------------------------\n'
        component_licenses += '%s:\n\n' % license_id
        component_licenses += '%s\n' % licenses[license_id]

    return '%s\n\n%s\n%s' % (preamble, component_notices, component_licenses)


def list_sub_components(base_dir):
    found = []
    for dirpath, dirs, dummy in os.walk(base_dir):
        for dir_name in dirs:
            found.append(extract_license_info(os.path.join(dirpath, dir_name)))
    return found


def write_license_file(directory, contents):
    file_path = os.path.join(directory, 'LICENSE')
    with open(file_path, mode='wt', encoding='utf-8') as file_handle:
        file_handle.write(contents)


def list_ntp_backgrounds(metadata_file):
    json_metadata = ''
    with open(metadata_file, mode='rt', encoding='utf-8') as file_handle:
        # Strip out copyright header
        metadata = file_handle.readlines()[3:]
        # Hack to turn this TypeScript file into valid JSON
        json_metadata = "".join(metadata) \
            .replace("export const images: NewTab.BackgroundWallpaper[] = [", "[") \
            .replace('"', '"').replace("'", '"')

    images = json.loads(json_metadata)
    return images


def validated_data_field(data, field_name):
    field_value = data[field_name]
    if not field_value:
        print('Missing %s for background image %s' %
              (field_name, data['name']))
        sys.exit(1)

    return field_value


def generate_backgrounds_license(preamble, backgrounds):
    notices = ''

    for background in backgrounds:
        if notices:
            notices += '\n'

        filename = validated_data_field(background, 'wallpaperImageUrl')
        author_name = validated_data_field(background, 'author')
        # Don't validate link. it can be empty.
        author_link = background['link']
        original_url = validated_data_field(background, 'originalUrl')
        license_text = validated_data_field(background, 'license')
        if license_text != 'used with permission' and license_text[0:8] != 'https://' \
           and license_text[0:7] != 'http://':
            print('Invalid license for background image %s. It needs to be a URL or the string "used with permission".'
                  % background['name'])
            sys.exit(1)

        if author_link != '':
            notices += 'File: %s\nAuthor: %s (%s)\nURL: %s\nLicense: %s\n' \
                       % (filename, author_name, author_link, original_url, license_text)
        else:
            notices += 'File: %s\nAuthor: %s\nURL: %s\nLicense: %s\n' \
                       % (filename, author_name, original_url, license_text)

    return '%s\n\n%s' % (preamble, notices)


def main():
    components_dir = os.path.join(SOURCE_ROOT, 'components')
    third_party_dir = os.path.join(components_dir, 'third_party')

    # Brave Ad Block component
    adblock_dir = os.path.join(third_party_dir, 'adblock')
    adblock_lists_dir = os.path.join(adblock_dir, 'lists')
    adblock_preamble = 'These licenses do not apply to any of the code shipped with the Brave Browser, but may ' \
                       'apply to lists downloaded after installation for use with the Brave Shields feature. ' \
                       'The Brave Browser and such lists are separate and independent works.'

    adblock_components = list_sub_components(adblock_lists_dir)
    write_license_file(adblock_dir, external_component_license_file(
        adblock_preamble, adblock_components))
    print('  - %s sub-components added in adblock/LICENSE' %
          len(adblock_components))

    # Brave Local Data component
    local_data_dir = os.path.join(third_party_dir, 'local_data')
    local_data_lists_dir = os.path.join(local_data_dir, 'lists')
    local_data_preamble = 'These licenses do not apply to any of the code shipped with the Brave Browser, but may ' \
                          'apply to data files downloaded after installation for use with various Brave features. ' \
                          'The Brave Browser and such data files are separate and independent works.'

    local_data_components = list_sub_components(local_data_lists_dir)
    write_license_file(local_data_dir, external_component_license_file(
        local_data_preamble, local_data_components))
    print('  - %s sub-components added in local_data/LICENSE' %
          len(local_data_components))

    # Brave New Tab UI component
    ntp_data_dir = os.path.join(components_dir, 'brave_new_tab_ui', 'data')
    ntp_backgrounds_preamble = 'These licenses do not apply to any of the code shipped with the Brave Browser and ' \
                               'instead apply to background images used on the new tab page. The Brave Browser and ' \
                               'such data files are separate and independent works.'

    ntp_backgrounds = list_ntp_backgrounds(
        os.path.join(ntp_data_dir, 'backgrounds.ts'))
    write_license_file(ntp_data_dir, generate_backgrounds_license(
        ntp_backgrounds_preamble, ntp_backgrounds))
    print('  - %s sub-components added in brave_new_tab_ui/data/LICENSE' %
          len(ntp_backgrounds))


if __name__ == '__main__':
    sys.exit(main())
