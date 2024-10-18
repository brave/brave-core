# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils
import os
import shutil
import glob


def purge_overrides(out_folder):
    """Deletes all overridden upstream files from the preprocess directory - we
       need this so removing an override doesn't break the build."""
    for f in glob.glob(os.path.join(out_folder, '**/*-chromium*')):
        if os.path.exists(f):
            os.remove(f)


def maybe_keep_upstream_version(override_in_folder, out_folder, override_file):
    name_bits = os.path.splitext(override_file)
    overridden_name = "".join([name_bits[0], "-chromium", name_bits[1]])

    # The path to the brave-core override file
    override_in_path = os.path.join(override_in_folder, override_file)

    # The path to the upstream file
    upstream_file_path = os.path.join(out_folder, override_file)

    with open(override_in_path) as f:
        text = f.read()

        # If we reference the upstream file in our overridden file, make sure
        # we keep it around.
        if os.path.basename(overridden_name).replace('.ts', '.js') in text:
            shutil.copy(upstream_file_path,
                        os.path.join(out_folder, overridden_name))


def get_brave_overrides(in_folder, in_files):
    """Gets all the overrides we have in brave-core for this target"""
    root_folder = os.path.abspath(
        os.path.join(os.path.dirname(__file__), '..', '..'))
    override_root_folder = os.path.join(root_folder, 'brave', 'chromium_src')

    override_files = []
    for file in in_files:
        in_file = os.path.join(in_folder,
                               file).replace(root_folder, override_root_folder)
        if os.path.exists(in_file):
            override_files.append(file)

    return (in_folder.replace(root_folder,
                              override_root_folder), override_files)


@override_utils.override_function(globals())
def main(original_function, argv):
    frame, result = override_utils.call_function_get_frame(
        original_function, argv)

    globals().update(frame.f_locals)
    purge_overrides(out_folder)

    for input_file in args.in_files:
        in_path = os.path.join(in_folder, input_file)
        process_file(in_path, input_file)

    (override_root_folder,
     overrides) = override_files = get_brave_overrides(in_folder,
                                                       args.in_files)
    if len(overrides) != 0:
        for override_file in overrides:
            maybe_keep_upstream_version(override_root_folder, out_folder,
                                        override_file)

            in_path = os.path.join(override_root_folder, override_file)
            process_file(in_path, override_file)
    return result
