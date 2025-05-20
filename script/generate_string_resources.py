#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
import sys
import re

import xml.sax
import xml.sax.handler

TS_STRING_IDS_FILE = 'string_ids.ts'
CPP_LOCALIZED_STRINGS_FILE = 'localized_strings'

root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))


def get_include_path(path):
    """Gets the include path for the generated string ids file"""
    return os.path.relpath(path, root)


def get_string_ids(grdp_file):
    """Returns all the message names that are marked as webui=true in the grdp
       file.
    """
    string_ids = []

    class MessageHandler(xml.sax.handler.ContentHandler):

        def startElement(self, name, attrs):
            if name == 'message' and attrs.get('webui') == 'true':
                string_ids.append(attrs['name'])

    xml.sax.parse(grdp_file, MessageHandler())

    return string_ids


def get_string_mapping(args):
    """Returns a mapping of string ids to their corresponding TS names"""
    mapping = {}

    for string_id in get_string_ids(args.input_grdp):
        # Strip the IDS_ prefix
        stripped_string_id = string_id[4:]

        for prefix in args.strip_string_prefixes:
            if stripped_string_id.startswith(prefix):
                stripped_string_id = stripped_string_id[len(prefix):]
            if stripped_string_id.startswith('_'):
                stripped_string_id = stripped_string_id[1:]

        tsName = [
            x[0].upper() + x[1:] for x in stripped_string_id.lower().split('_')
        ]
        tsName[0] = tsName[0].lower()
        mapping[string_id] = ''.join(tsName)

    return mapping


def parse_args():
    parser = argparse.ArgumentParser(
        description='Generate WebUI strings from a resources file')
    parser.add_argument(
        '--input_grdp',
        type=str,
        help='The path to the grdp file to generate string ids from')
    parser.add_argument('--generate_ts',
                        action='store_true',
                        help='Whether to generate string ids for typescript')
    parser.add_argument(
        '--generate_localized_strings',
        action='store_true',
        help=
        'Controls whether C++ files for exposing strings to WebUI are generated'
    )
    parser.add_argument('--target_gen_dir',
                        type=str,
                        help='The directory to write the generated files to')
    parser.add_argument(
        '--cpp_namespace',
        type=str,
        help='The namespace to generate the C++ to WebUI function in')
    parser.add_argument('--string_include_path',
                        type=str,
                        default='components/grit/brave_components_strings.h',
                        help='The path to the generated grit file')
    parser.add_argument(
        '--strip_string_prefixes',
        type=str,
        nargs='*',
        help=
        'Prefixes to strip from the string ids. **Note:** IDS_ is automatically stripped.'
    )
    args = parser.parse_args()

    return args


def generate_ts(mapping, args):
    """Generates a TS ambient enum containing the string ids"""
    enum_values = ",\n    ".join(
        [f'{k[0].upper() + k[1:]} = "{k}"' for k in mapping.values()])
    type_def = f"""
declare global {{
  const enum StringIds {{
    {enum_values}
  }}
}}

export {{ }}
"""

    with open(os.path.join(args.target_gen_dir, TS_STRING_IDS_FILE), 'w') as f:
        f.write(type_def)


def generate_localized_strings(mapping, args):
    """Generates a C++ header/source file for exposing strings to WebUI"""
    header = f"""
#include "base/containers/span.h"
#include "ui/base/webui/web_ui_util.h"

namespace {args.cpp_namespace} {{
  base::span<const webui::LocalizedString> GetLocalizedStrings();
}}
"""
    header_path = os.path.join(args.target_gen_dir,
                               CPP_LOCALIZED_STRINGS_FILE + '.h')

    with open(header_path, 'w') as f:
        f.write(header)

    localized_strings = ',\n          '.join(
        [f'{{"{v}", {k}}}' for k, v in mapping.items()])
    definition = f"""
#include "{get_include_path(header_path)}"

#include "{args.string_include_path}"

namespace {args.cpp_namespace} {{
  base::span<const webui::LocalizedString> GetLocalizedStrings() {{
    static constexpr auto kLocalizedStrings = std::to_array<
      webui::LocalizedString>(
        {{{localized_strings}}});
    return kLocalizedStrings;
  }}
}}"""
    with open(
            os.path.join(args.target_gen_dir,
                         CPP_LOCALIZED_STRINGS_FILE + '.cc'), 'w') as f:
        f.write(definition)


def main():
    """Generates code for exposing string to WebUI and a StringIds enum to access
     them from TS in a Typesafe way."""
    args = parse_args()
    if not os.path.exists(args.target_gen_dir):
        os.makedirs(args.target_gen_dir)

    mapping = get_string_mapping(args)

    if args.generate_ts:
        generate_ts(mapping, args)

    if args.generate_localized_strings:
        generate_localized_strings(mapping, args)


if __name__ == '__main__':
    main()
