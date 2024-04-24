# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
from gen_webui_override_additional_inputs import get_webui_override_additional_inputs

def preprocess_overrides(args, in_folder, out_folder, preprocess_file):
  for input_file in args.in_files:
    preprocess_file(input_file, os.path.join(in_folder, input_file))

  overrides = get_webui_override_additional_inputs(args.in_folder, args.in_files, True)
  for (input_file, override_file) in overrides.items():   
    original_output_file = os.path.join(out_folder, input_file)
    [path, ext] = os.path.splitext(input_file)
    upstream_output_file = os.path.join(out_folder, path + "-chromium" + ext)

    # Copy the original output file to `<name>-chromium.<ext>`
    os.rename(original_output_file, upstream_output_file)

    # Preprocess file just uses the output name of the file, so this will work!
    preprocess_file(input_file, override_file)
