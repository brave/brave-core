# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os

def preprocess_overrides(args, in_folder, out_folder, preprocess_file):
  for input_file in args.in_files:
    preprocess_file(input_file, os.path.join(in_folder, input_file))

  for input_file in args.in_files:
    in_path = os.path.join(in_folder, input_file)
    
    # Don't add chromium_src overrides for things defined in Brave.
    if not args.in_folder.startswith('..') or 'brave' in args.in_folder: continue
    
    last_dot_dot = args.in_folder.rindex('../') + 3 # Add for the length of '../'
    override_file = os.path.normpath(os.path.join(_CWD, args.in_folder[:last_dot_dot], 'brave', 'chromium_src', args.in_folder[last_dot_dot:], input_file))
    
    # Only override files which exist
    if not os.path.exists(override_file): continue

    original_output_file = os.path.join(out_folder, input_file)
    [path, ext] = os.path.splitext(input_file)
    upstream_output_file = os.path.join(out_folder, path + "-chromium" + ext)

    # Copy the original output file to `<name>-chromium.<ext>`
    os.rename(original_output_file, upstream_output_file)

    # Preprocess file just uses the output name of the file, so this will work!
    preprocess_file(input_file, override_file)
