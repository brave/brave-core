# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
import os
import shlex
import shutil

import brave_chromium_utils
import override_utils

VERBOSE = False  # Set to True to print verbose messages.


# Loads the leo overrides from leo_overrides.json.
def LoadLeoOverrides():
    leo_overrides_config_path = brave_chromium_utils.wspath(
        '//brave/components/vector_icons/leo_overrides.json')
    leo_override_format = '//brave/node_modules/@brave/leo/icons-skia/{}.icon'

    with open(leo_overrides_config_path, 'r') as f:
        leo_overrides_config = json.load(f)

    leo_overrides = {}
    for icon, leo_override in leo_overrides_config.items():
        if not icon.startswith("//"):
            raise ValueError(f'Invalid icon path: {icon}, must start with //')
        leo_override = brave_chromium_utils.wspath(
            leo_override_format.format(leo_override))
        if not os.path.exists(leo_override):
            raise FileNotFoundError(f'leo override not found: {leo_override}')
        leo_overrides[icon] = os.path.relpath(leo_override)

    return leo_overrides


# Rewrites the file list with overridden icons.
def RewriteFileListWithOverrides(file_list):
    with open(file_list, "r") as f:
        file_list_contents = f.read()
    icon_list = shlex.split(file_list_contents)

    leo_overrides = LoadLeoOverrides()

    rewritten_icon_list = []
    for icon_path in icon_list:
        icon_ws_path = brave_chromium_utils.to_wspath(icon_path)
        chromium_src_override = brave_chromium_utils.get_chromium_src_override(
            icon_path)

        # Leo override may have a different name than the original icon.
        # Copy the override to gen/brave/vector_icons_overrides with the
        # original name.
        leo_override = leo_overrides.get(icon_ws_path)
        if leo_override:
            if os.path.exists(chromium_src_override):
                raise RuntimeError(
                    f'leo override and chromium_src override both exist for '
                    f'{icon_path}')

            gen_file = f'gen/brave/vector_icons_overrides/{icon_ws_path[2:]}'
            os.makedirs(os.path.dirname(gen_file), exist_ok=True)
            shutil.copyfile(leo_override, gen_file)
            rewritten_icon_list.append(gen_file)
            if VERBOSE:
                print(
                    f'Using leo override: {leo_override} copied to {gen_file}')
            continue

        # If the icon is not in leo overrides, check if it's in chromium_src
        # overrides.
        if os.path.exists(chromium_src_override):
            chromium_src_override = os.path.relpath(chromium_src_override)
            rewritten_icon_list.append(chromium_src_override)
            if VERBOSE:
                print(f'Using chromium_src override: {chromium_src_override}')
            continue

        # Otherwise use the original icon.
        rewritten_icon_list.append(icon_path)

    # Write the rewritten file list to the original file list.
    with open(file_list, "w") as f:
        f.write(shlex.join(rewritten_icon_list))


@override_utils.override_function(globals())
def AggregateVectorIcons(orig_func, working_directory, file_list, output_cc,
                         output_h):
    RewriteFileListWithOverrides(file_list)
    return orig_func(working_directory, file_list, output_cc, output_h)
