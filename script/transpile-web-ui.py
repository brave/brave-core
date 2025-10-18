#!/usr/bin/env python3
# Copyright (c) 2018 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import json
import os
import sys
import shutil

from pathlib import Path

from lib.util import execute_stdout, scoped_cwd


NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
    NPM += '.cmd'


def main():
    # Accept all paths as either absolute or relative to working dir,
    # except grd_path, which we must know the relative location of
    # in order to create a valid depfile.
    args = parse_args()

    output_path_absolute = os.path.abspath(args.output_path[0])
    root_gen_dir = args.root_gen_dir[0]
    grd_path = os.path.join(args.output_path[0], args.grd_name[0])
    resource_path_prefix = args.resource_path_prefix

    clean_target_dir(output_path_absolute)

    webpack_gen_dir = output_path_absolute

    depfile_path = os.path.abspath(args.depfile_path[0])
    data_paths_file = os.path.abspath(args.data_paths_file[0])
    transpile_options = dict(production=args.production,
                             target_gen_dir=webpack_gen_dir,
                             root_gen_dir=root_gen_dir,
                             entry_points=args.entry,
                             depfile_path=depfile_path,
                             depfile_sourcename=grd_path,
                             webpack_aliases=args.webpack_alias,
                             output_module=args.output_module,
                             extra_modules=args.extra_modules,
                             public_asset_path=args.public_asset_path,
                             sync_wasm=args.sync_wasm,
                             no_iife=args.no_iife)
    transpile_web_uis(transpile_options)
    generate_grd(output_path_absolute, args.grd_name[0], args.resource_name[0],
                 resource_path_prefix)

    verify_webpack_srcs(root_gen_dir, data_paths_file, depfile_path,
                        args.extra_modules)



def parse_args():
    parser = argparse.ArgumentParser(description='Transpile web-uis')
    parser.add_argument('-p', '--production',
                        action='store_true',
                        help='Uses production config')
    parser.add_argument('--entry',
                        action='append',
                        help='Entry points',
                        required=True)
    parser.add_argument('--output_path', nargs=1)
    parser.add_argument('--root_gen_dir', nargs=1)
    parser.add_argument('--depfile_path', nargs=1)
    parser.add_argument('--data_paths_file', nargs=1)
    parser.add_argument('--grd_name', nargs=1)
    parser.add_argument('--resource_name', nargs=1)
    parser.add_argument('--public_asset_path', nargs='?')
    parser.add_argument('--webpack_alias',
                        action='append',
                        help='Webpack alias',
                        required=False,
                        default=[])
    parser.add_argument('--output_module', action='store_true')
    parser.add_argument(
        "--resource_path_prefix",
        nargs='?',
        help="The resource path prefix. Used in grit part files.")
    parser.add_argument('--extra_modules',
                        action='append',
                        help='Extra paths to find modules',
                        required=False,
                        default=[])
    parser.add_argument('--sync_wasm', action='store_true')
    parser.add_argument('--no_iife', action='store_true')

    args = parser.parse_args()
    # validate args
    if (args.output_path is None or len(args.output_path) != 1 or
            len(args.output_path[0]) == 0):
        raise Exception(" output_path argument was not specified correctly")
    # args are valid
    return args


def clean_target_dir(target_dir):
    try:
        if os.path.exists(target_dir):
            shutil.rmtree(target_dir)
    except Exception as e:
        raise Exception("Error removing previous webpack target dir") from e


def transpile_web_uis(options):
    env = os.environ.copy()

    args = [NPM, 'run', 'web-ui', '--']

    if options['production']:
        args.append("--mode=production")
    else:
        args.append("--mode=development")

    if options['public_asset_path'] is not None:
        args.append("--env=output_public_path=" + options['public_asset_path'])

    # web pack aliases
    if options['webpack_aliases']:
        args.append("--env=webpack_aliases=" +
                    ",".join(options['webpack_aliases']))

    if options['output_module']:
        args.append("--env=output_module")

    # extra module locations
    if options['extra_modules']:
        args.append("--env=extra_modules=" + ",".join(options['extra_modules']))

    # In webpack.config.js there is a custom parser to support named entry
    # points. As webpack-cli no longer supports named entries, we provide them
    # via in a custom variable, comma-separated and with
    # "[name]=[path]" syntax.
    args.append("--env=brave_entries=" + ",".join(options['entry_points']))

    if options['sync_wasm']:
        args.append("--env=sync_wasm")

    # Webpack will by default wrap the output in an IIFE, which is not
    # desirable for some bundles.
    if options['no_iife']:
        args.append("--env=no_iife")

    # We should use webpack-cli env param to not pollute environment
    env["ROOT_GEN_DIR"] = options['root_gen_dir']
    env["TARGET_GEN_DIR"] = options['target_gen_dir']
    env["DEPFILE_PATH"] = options['depfile_path']
    env["DEPFILE_SOURCE_NAME"] = options['depfile_sourcename']

    dirname = os.path.abspath(os.path.join(__file__, '..', '..'))
    with scoped_cwd(dirname):
        execute_stdout(args, env)

def generate_grd(target_include_dir, grd_name, resource_name,
                 resource_path_prefix):
    env = os.environ.copy()

    args = [NPM, 'run', 'web-ui-gen-grd']

    env["RESOURCE_NAME"] = resource_name
    env["GRD_NAME"] = grd_name
    env["ID_PREFIX"] = "IDR_" + resource_name.upper() + '_'
    env["TARGET_DIR"] = target_include_dir

    if resource_path_prefix is not None:
        env["RESOURCE_PATH_PREFIX"] = resource_path_prefix

    dirname = os.path.abspath(os.path.join(__file__, '..', '..'))
    with scoped_cwd(dirname):
        execute_stdout(args, env)


def verify_webpack_srcs(root_gen_dir, data_paths_file, depfile_path,
                        extra_modules):

    src_folder = Path(os.path.abspath(os.path.join(
        __file__, '..', '..', '..'))).resolve().as_posix()

    make_source_absolute = lambda path: Path(path).resolve().as_posix(
    ).replace(src_folder, '/')

    out_dir = make_source_absolute(Path(root_gen_dir).resolve().parents[1])

    with open(data_paths_file) as f:
        src_roots = json.loads(f.read())
        assert isinstance(src_roots, list)

    with open(depfile_path) as f:
        files = [make_source_absolute(file) for file in f.read().split()[1:]]

    all_roots = src_roots + [
        '//brave/node_modules',  # handled via package.json
        out_dir  # generated assets are deps and handled by gn already
    ] + [make_source_absolute(module) for module in extra_modules]

    not_contained = get_not_contained(all_roots, files)

    if len(not_contained) > 0:
        print("error occured cross-referencing data folders.")
        print("transpile_web_ui accessed the following files:")
        print("  " + "\n  ".join(not_contained))

        if len(src_roots) > 0:
            print(
                "However they are not listed as data in target. data conatains:"
            )
            print("  " + "\n  ".join(src_roots))
        else:
            print("However data is empty")

        print(
            "fix this issue by adding the containing source folders into the transpile_web_ui target data section"
        )

        sys.exit(1)


def get_not_contained(roots, test_paths):
    """
    Check whether all given paths are contained within the source roots.
    Returns list of paths that were not contained
    """

    not_contained = [
        str(path) for path in test_paths
        if not any(path.startswith(root) for root in roots)
    ]

    return not_contained

if __name__ == '__main__':
    sys.exit(main())
