import argparse
import os
import sys
import shutil
from lib.util import execute_stdout, scoped_cwd


NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
    NPM += '.cmd'


def main():
    args = parse_args()
    clean_target_dir(args.target_gen_dir[0])
    webpack_gen_dir = args.target_gen_dir[0]
    if args.extra_relative_path is not None:
        webpack_gen_dir = webpack_gen_dir + args.extra_relative_path
    transpile_web_uis(args.production, webpack_gen_dir,
                      args.entry,
                      args.depfile_path[0], args.depfile_outputpath[0],
                      args.public_asset_path)
    generate_grd(args.target_gen_dir[0], args.resource_name[0])


def parse_args():
    parser = argparse.ArgumentParser(description='Transpile web-uis')
    parser.add_argument('-p', '--production',
                        action='store_true',
                        help='Uses production config')
    parser.add_argument('--entry',
                        action='append',
                        help='Entry points',
                        required=True)
    parser.add_argument('--target_gen_dir', nargs=1)
    parser.add_argument('--depfile_outputpath', nargs=1)
    parser.add_argument('--depfile_path', nargs=1)
    parser.add_argument('--resource_name', nargs=1)
    parser.add_argument('--extra_relative_path', nargs='?')
    parser.add_argument('--public_asset_path', nargs='?')
    args = parser.parse_args()
    # validate args
    if (args.target_gen_dir is None or
        len(args.target_gen_dir) is not 1 or
            len(args.target_gen_dir[0]) is 0):
        raise Exception("target_gen_dir argument was not specified correctly")
    if "out" not in args.target_gen_dir[0]:
        raise Exception("target_gen_dir did not contain 'out'")
    # args are valid
    return args


def clean_target_dir(target_dir, env=None):
    try:
        if os.path.exists(target_dir):
            shutil.rmtree(target_dir)
    except Exception as e:
        raise Exception("Error removing previous webpack target dir", e)


def transpile_web_uis(production, target_gen_dir,
                      entry_points, depfile_path, depfile_outputpath, public_asset_path=None, env=None):
    if env is None:
        env = os.environ.copy()

    args = [NPM, 'run', 'web-ui', '--']

    if production:
        args.append("--mode=production")
    else:
        args.append("--mode=development")

    if public_asset_path is not None:
        args.append("--output-public-path=" + public_asset_path)

    # entrypoints
    for entry in entry_points:
        args.append(entry)

    env["TARGET_GEN_DIR"] = os.path.abspath(target_gen_dir)
    env["DEPFILE_PATH"] = depfile_path
    env["DEPFILE_OUTPUT_PATH"] = depfile_outputpath

    dirname = os.path.abspath(os.path.join(__file__, '..', '..'))
    with scoped_cwd(dirname):
        execute_stdout(args, env)


def generate_grd(target_include_dir, resource_name, env=None):
    if env is None:
        env = os.environ.copy()

    args = [NPM, 'run', 'web-ui-gen-grd']

    env["RESOURCE_NAME"] = resource_name
    env["ID_PREFIX"] = "IDR_" + resource_name.upper() + '_'
    env["TARGET_DIR"] = os.path.abspath(target_include_dir)

    dirname = os.path.abspath(os.path.join(__file__, '..', '..'))
    with scoped_cwd(dirname):
        execute_stdout(args, env)


if __name__ == '__main__':
    sys.exit(main())
