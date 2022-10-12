#!/usr/bin/env python3

import argparse
import os
import sys
import shutil
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

    clean_target_dir(output_path_absolute)

    webpack_gen_dir = output_path_absolute
    if args.extra_relative_path is not None:
        webpack_gen_dir = webpack_gen_dir + args.extra_relative_path

    depfile_path = os.path.abspath(args.depfile_path[0])
    transpile_options = dict(
        production=args.production,
        target_gen_dir=webpack_gen_dir,
        root_gen_dir=root_gen_dir,
        entry_points=args.entry,
        depfile_path=depfile_path,
        depfile_sourcename=grd_path,
        webpack_aliases=args.webpack_alias,
        extra_modules=args.extra_modules,
        public_asset_path=args.public_asset_path
    )
    transpile_web_uis(transpile_options)
    generate_grd(output_path_absolute, args.grd_name[0], args.resource_name[0])


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
    parser.add_argument('--grd_name', nargs=1)
    parser.add_argument('--resource_name', nargs=1)
    parser.add_argument('--extra_relative_path', nargs='?')
    parser.add_argument('--public_asset_path', nargs='?')
    parser.add_argument('--webpack_alias',
                        action='append',
                        help='Webpack alias',
                        required=False,
                        default=[])
    parser.add_argument('--extra_modules',
                        action='append',
                        help='Extra paths to find modules',
                        required=False,
                        default=[])

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
        raise Exception("Error removing previous webpack target dir", e)

def transpile_web_uis(options):
    env = os.environ.copy()

    args = [NPM, 'run', 'web-ui', '--']

    if options['production']:
        args.append("--mode=production")
    else:
        args.append("--mode=development")

    if options['public_asset_path'] is not None:
        args.append("--output-public-path=" + options['public_asset_path'])

    # web pack aliases
    for alias in options['webpack_aliases']:
        args.append("--webpack_alias=" + alias)

    # extra module locations
    for module_path in options['extra_modules']:
        args.append("--extra_modules=" + module_path)

    # entrypoints
    for entry in options['entry_points']:
        args.append(entry)
    env["ROOT_GEN_DIR"] = options['root_gen_dir']
    env["TARGET_GEN_DIR"] = options['target_gen_dir']
    env["DEPFILE_PATH"] = options['depfile_path']
    env["DEPFILE_SOURCE_NAME"] = options['depfile_sourcename']

    dirname = os.path.abspath(os.path.join(__file__, '..', '..'))
    with scoped_cwd(dirname):
        execute_stdout(args, env)


def generate_grd(target_include_dir, grd_name, resource_name, env=None):
    if env is None:
        env = os.environ.copy()

    args = [NPM, 'run', 'web-ui-gen-grd']

    env["RESOURCE_NAME"] = resource_name
    env["GRD_NAME"] = grd_name
    env["ID_PREFIX"] = "IDR_" + resource_name.upper() + '_'
    env["TARGET_DIR"] = target_include_dir

    dirname = os.path.abspath(os.path.join(__file__, '..', '..'))
    with scoped_cwd(dirname):
        execute_stdout(args, env)


if __name__ == '__main__':
    sys.exit(main())
