# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import sys
import argparse
import json
import shutil


def main():
    parser = argparse.ArgumentParser(
        description="Creates an asset catalog in the provided output directory"
    )
    parser.add_argument("-o",
                        "--output",
                        required=True,
                        help="The xcassets file to output")
    parser.add_argument("-l",
                        "--leo_sf_symbols_directory",
                        required=True,
                        help="Directory of Leo SF symbols")
    parser.add_argument(
        "-i",
        "--icons",
        required=True,
        help="Comma-separated list of icons (e.g., icon1,icon2)")

    args = parser.parse_args()
    output_directory = args.output
    leo_sf_symbols_directory = args.leo_sf_symbols_directory
    icons = args.icons.split(",")

    if not output_directory or not leo_sf_symbols_directory or not icons:
        print("Missing required arguments")
        sys.exit(1)

    # We consider the xcassets a "file" so we want to replace the entire
    # thing each time something changes
    if os.path.exists(output_directory):
        shutil.rmtree(output_directory)

    # Create the xcassets folder and standard Contents.json
    os.makedirs(output_directory, exist_ok=True)
    with open(os.path.join(output_directory, "Contents.json"), "w") as f:
        json.dump({"info": {"author": "xcode", "version": 1}}, f)

    for icon in icons:
        plain_name = os.path.splitext(icon)[0]  # without .svg extension
        svg_path = os.path.join(leo_sf_symbols_directory, "symbols", icon)

        if not os.path.exists(svg_path):
            print(f"Could not find Leo SF symbol named {icon}")
            sys.exit(1)

        symbolset = os.path.join(output_directory, f"{plain_name}.symbolset")
        os.makedirs(symbolset, exist_ok=True)

        # Copy the SVG file into the symbolset folder
        shutil.copyfile(svg_path, os.path.join(symbolset, icon))

        # Create Contents.json for the symbolset
        with open(os.path.join(symbolset, "Contents.json"), "w") as f:
            json.dump(
                {
                    "info": {
                        "author": "xcode",
                        "version": 1
                    },
                    "symbols": [{
                        "filename": icon,
                        "idiom": "universal"
                    }]
                }, f)


if __name__ == "__main__":
    main()
