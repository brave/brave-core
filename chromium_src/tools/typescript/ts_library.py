# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os

import override_utils


def is_gen_brave_dir(out_dir):
    GEN_BRAVE = 'gen/brave'
    dirs = out_dir.split('/')
    # Test gen/brave/... and <target_toolchain>/gen/brave/...
    return '/'.join(dirs[:2]) == GEN_BRAVE or '/'.join(dirs[1:3]) == GEN_BRAVE


@override_utils.override_function(globals())
def _write_tsconfig_json(original_function, gen_dir, tsconfig, tsconfig_file):
    tsconfig['include'] = ["preprocessed/**/*-chromium*.ts"]
    original_function(gen_dir, tsconfig, tsconfig_file)


@override_utils.override_function(globals())
def main(original_function, argv):
    # Parse only the arguments used by this override
    parser = argparse.ArgumentParser()
    parser.add_argument('--root_dir', required=True)
    parser.add_argument('--out_dir', required=True)
    parser.add_argument('--output_suffix', required=True)
    parser.add_argument('--gen_dir', required=True)
    parser.add_argument('--in_files', nargs='*')
    args, _ = parser.parse_known_args(argv)

    if args.in_files is not None:
        for f in args.in_files:
            pathname, _ = os.path.splitext(f)
            # Delete any obsolete .d.ts files (from previous builds)
            # corresponding to .ts |in_files| in |out_dir| folder to avoid
            # throwing the following error:
            #
            # "error TS5055: Cannot write file '...' because it would overwrite
            # input file."
            if args.root_dir != args.out_dir and is_gen_brave_dir(args.out_dir):
                to_check = os.path.join(args.out_dir, pathname + '.d.ts')
                if os.path.exists(to_check):
                    os.remove(to_check)

    original_function(argv)

    manifest_path = os.path.join(args.gen_dir,
                                 f'{args.output_suffix}_manifest.json')
    if os.path.exists(manifest_path):
        import json
        import glob
        import re
        manifest = json.load(open(manifest_path))

        preprocess_dir = os.path.join(args.gen_dir, 'preprocessed')
        for override_file in glob.glob(
                os.path.join(preprocess_dir, '**/*-chromium*.ts')):
            rel_path = os.path.relpath(override_file, preprocess_dir)
            manifest['files'].append(re.sub(r'\.ts$', '.js', rel_path))

        with open(manifest_path, 'w') as f:
            json.dump(manifest, f)
