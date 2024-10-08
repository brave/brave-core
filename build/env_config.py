#!/usr/bin/env python3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import json
import os
import re
import sys


# This script reads a .env configuration file and converts it into a JSON
# object, which is then printed to the standard output. It is used as part of
# `gn gen` to access .env variables directly from `.gn/.gni` files.
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('file_path')

    args = parser.parse_args()

    json.dump(read_env_config_as_dict(args.file_path),
              sort_keys=True,
              fp=sys.stdout)


# Regex to define a .env config line with C++/GN restrictions.
ENV_CONFIG_LINE = re.compile(
    r"""
    (?:^|\n)                # Start of line or newline
    \s*                     # Optional leading whitespace
    ([a-zA-Z_]\w*)          # Key: a valid C++/GN identifier
    (?:\s*=\s*)             # Assignment operator: '=' with optional whitespace
    (                       # Start of value capturing group
        \s*'(?:\\'|[^'])*'  # Single-quoted value
        |                   # OR
        \s*"(?:\\"|[^"])*"  # Double-quoted value
        |                   # OR
        \s*`(?:\\`|[^`])*`  # Backtick-quoted value
        |                   # OR
        [^#\n]+             # Unquoted value (anything except # or newline)
    )                       # Value is required
    \s*                     # Optional trailing whitespace
    (?:\#.*)?               # Optional comment
    (?:$|\n)                # End of line or newline
""", re.VERBOSE | re.MULTILINE)


def read_env_config_as_dict(file_path, result_dict=None):
    if result_dict is None:
        result_dict = {}

    # PowerShell saves files with BOM, to support this we use utf-8-sig.
    with open(file_path, 'r', encoding='utf-8-sig') as file:
        for match in ENV_CONFIG_LINE.finditer(file.read()):
            key = match.group(1)
            value = match.group(2).strip()

            # Check if double quoted
            maybe_quote = value[0] if value else ''

            # Remove surrounding quotes
            value = re.sub(r"^(['\"`])([\s\S]*)\1$", r'\2', value)

            # Expand newlines if double quoted
            if maybe_quote == '"':
                value = value.replace('\\n', '\n')

            try:
                # Try to parse the value as JSON to represent numbers and
                # booleans correctly.
                value = json.loads(value)
            except json.JSONDecodeError:
                # Keep the value as a string if it's not a valid JSON.
                pass

            if key == 'include_env':
                include_path = value
                if not os.path.isabs(include_path):
                    include_path = os.path.abspath(
                        os.path.join(os.path.dirname(file_path), include_path))
                result_dict.setdefault('include_env', []).append(
                    include_path.replace("\\", "/"))
                read_env_config_as_dict(include_path, result_dict)
            else:
                result_dict[key] = value
                # Additionaly store the key in uppercase to be able to access it
                # during BUILDFLAG generation. GN doesn't have a way to convert
                # strings to uppercase.
                result_dict[key.upper()] = value

                # Store JSON-escaped variants to be used in a buildflag header.
                escaped_value = json.dumps(value)
                result_dict[key + '_escaped'] = escaped_value
                result_dict[key.upper() + '_escaped'] = escaped_value

    return result_dict


if __name__ == '__main__':
    main()
