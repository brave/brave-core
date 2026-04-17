# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import sys
import argparse
import override_utils

def add_mapping(path_mappings, import_path, source_path):
  source_path = source_path.replace('\\', '/')

  path_mappings[import_path].append(source_path)
  path_mappings['chrome:' + import_path].append(source_path)

@override_utils.override_function(globals())
def _write_path_mappings_file(
    original_function, path_mappings, output_suffix, gen_dir, pretty_print
):
    # Unfortunately we need to reparse the arguments to get the root_gen_dir
    parser = argparse.ArgumentParser()
    parser.add_argument('--root_gen_dir', required=True)
    (args, _) = parser.parse_known_args(sys.argv[1:])

    add_mapping(path_mappings, '//resources/brave/leo.bundle.js', f'{args.root_gen_dir}/brave/web-ui-leo/leo.bundle.d.ts')

    original_function(path_mappings, output_suffix, gen_dir, pretty_print)
