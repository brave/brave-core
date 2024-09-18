#!/usr/bin/env python3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import inspect
import json
import os
import re
import sys

ENV_CONFIG_SAMPLE = inspect.cleandoc('''
# This is a sample .env config file for the build system.
# See https://github.com/brave/brave-browser/wiki/Build-configuration
''')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('command',
                        choices=['create_if_not_found', 'convert_to_json'])
    parser.add_argument('filename', nargs='?')

    args = parser.parse_args()

    if args.command == 'create_if_not_found':
        if not os.path.exists(args.filename):
            with open(args.filename, 'w', newline='\n') as file:
                file.write(ENV_CONFIG_SAMPLE + '\n')
    elif args.command == 'convert_to_json':
        json.dump(read_env_config_as_dict(args.filename),
                  sort_keys=True,
                  fp=sys.stdout)


def read_env_config_as_dict(file_path, result_dict=None):
    if result_dict is None:
        result_dict = {}

    # PowerShell saves files with BOM, to support this we use utf-8-sig.
    with open(file_path, 'r', encoding='utf-8-sig') as file:
        for line in file:
            # Remove comments at the end of the line
            line = re.sub(r'#.*$', '', line).strip()

            splitted_line = line.split('=', 1)
            if len(splitted_line) != 2:
                continue

            key, value = map(str.strip, splitted_line)

            # Skip invalid keys.
            if not re.match(r'[a-zA-Z_]+[a-zA-Z0-9_]*', key):
                continue

            try:
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

    return result_dict


if __name__ == '__main__':
    main()
