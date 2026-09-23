#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import json
import os
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    with open(args.input, encoding="utf-8") as input_file:
        package_json = json.load(input_file)

    if not isinstance(package_json, dict):
        raise ValueError("Expected package.json top-level object")

    package_json.pop("version", None)
    content = json.dumps(package_json, indent=2) + "\n"

    os.makedirs(os.path.dirname(args.output), exist_ok=True)

    if os.path.exists(args.output):
        with open(args.output, encoding="utf-8") as output_file:
            if output_file.read() == content:
                return

    with open(args.output, "w", encoding="utf-8", newline="\n") as output_file:
        output_file.write(content)


if __name__ == "__main__":
    main()
