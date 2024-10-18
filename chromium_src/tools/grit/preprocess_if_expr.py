# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils
import os


@override_utils.override_function(globals())
def main(original_function, argv):
    frame, result = override_utils.call_function_get_frame(
        original_function, argv)

    globals().update(frame.f_locals)

    for input_file in args.in_files:
        in_path = os.path.join(in_folder, input_file)
        process_file(in_path, input_file)

    if args.overrides:
        import shutil

        for override_file in args.overrides:
            name_bits = os.path.splitext(override_file)
            overridden_name = "".join(
                [name_bits[0], "-chromium", name_bits[1]])
            shutil.copy(os.path.join(out_folder, override_file),
                        os.path.join(out_folder, overridden_name))

            in_path = os.path.join(args.override_in_folder, override_file)
            process_file(in_path, override_file)
    return result
