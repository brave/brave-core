import argparse
import glob
import os
import shutil
import re
import subprocess
import sys
import tarfile

import revisions

# Updates/adds new crates under brave/tools/crates.
#
# This script works based on the variables defined in brave/tools/crates/revisions.py, e.g.:
# WASM_PACK_REPO = 'https://github.com/rustwasm/wasm-pack'
# WASM_PACK_REVISION = 'v0.13.1'
#
# `npm run update_brave_tools_crates -- --crate=wasm_pack` will then
# - clone 'https://github.com/rustwasm/wasm-pack'
# - at tag 'v0.13.1'
# - and assemble 'wasm-pack' with `cargo package` (manifest assumed to be in the root of the repo).
#
# If the desired crate is not in the root of the repo,
# specify its path with the `*_MANIFEST_PATH` varible, e.g.:
# WASM_BINDGEN_CLI_MANIFEST_PATH = '/crates/cli'
#
# Additionally, this script will update the Revision and Version fields in README.chromium
# (if one is present).
#
# Notes:
# - after running this script, you'll need to create a PR to bump the vendored version.

cargo_bin = os.path.abspath(
    os.path.join(os.environ['RUSTUP_HOME'], 'bin',
                 'cargo' if sys.platform != 'win32' else 'cargo.exe'))
crate_dir = 'crate'
temp_dir = 'temp'


def parse_args():
    parser = argparse.ArgumentParser(
        description='Updates/adds new crates under brave/tools/crates.')
    parser.add_argument('--crate', required=True)
    return parser.parse_args()


def get_file_system_entry(pattern):
    entries = glob.glob(pattern)
    if not entries:
        return None
    elif len(entries) != 1:
        raise Exception(
            f'{pattern} yields multiple entries ({", ".join(entries)})!')
    return entries[0]


def remove_dir(pattern):
    entry = get_file_system_entry(pattern)
    if entry is None:
        return
    print(f'Removing {entry}...')
    shutil.rmtree(entry)


def run_git_clone(url, tag):
    args = [
        'git', 'clone', '--recurse-submodules', '--depth=1', f'--branch={tag}',
        url, temp_dir
    ]
    print(f'Running {" ".join(args)}...')
    subprocess.check_call(args)


def run_cargo_package(manifest_path, package):
    args = [
        cargo_bin, 'package', '--no-verify',
        f'--manifest-path={os.path.join(temp_dir, *str(manifest_path or "").split("/"), "Cargo.toml")}',
        f'--package={package}'
    ]
    print(f'Running {" ".join(args)}...')
    subprocess.check_call(args)


def extract_crate():
    pattern = os.path.join(temp_dir, 'target', 'package', '*.crate')
    crate_file = get_file_system_entry(pattern)
    if crate_file is None:
        raise Exception(
            f"Couldn't find .crate file based on pattern {pattern}!")
    print(f'Extracting {crate_file}...')
    with tarfile.open(crate_file, 'r:gz') as tar:
        for tarinfo in tar.getmembers():
            tarinfo.name = re.sub(r'^[^\/]*', crate_dir, tarinfo.name)
        tar.extractall()


def get_revision():
    args = ['git', '-C', temp_dir, 'rev-parse', 'HEAD']
    print(f'Running {" ".join(args)}...')
    return subprocess.check_output(args, text=True)


def get_version_from_tag(tag):
    match = re.match(r'^.*?(\d+(?:.\d+)*)', tag)
    if not match:
        raise Exception(f"Couldn't parse {tag}!")
    return f'v{match.group(1)}'


def update_readme(tag):
    pattern = 'README.chromium'
    readme = get_file_system_entry(pattern)
    if readme is None:
        return
    revision = get_revision()
    version = get_version_from_tag(tag)
    with open(readme, 'r+') as file:
        updated_readme = re.sub(
            r'^(Revision|Version).*$',
            lambda m: f'Revision: {revision}'
            if m.group(1) == 'Revision' else f'Version: {version}',
            file.read().rstrip(),
            flags=re.MULTILINE)
        file.seek(0)
        file.write(updated_readme)


def run_cargo_vendor():
    args = [
        cargo_bin, 'vendor',
        f'--manifest-path={os.path.join(crate_dir, "Cargo.toml")}',
        os.path.join(crate_dir, 'vendor')
    ]
    print(f'Running {" ".join(args)}...')
    subprocess.check_call(args)


def add_config_toml():
    config_toml = os.path.join(crate_dir, '.cargo', 'config.toml')
    print(f'Adding {config_toml}...')
    os.makedirs(os.path.dirname(config_toml))
    with open(config_toml, 'w+') as file:
        file.write('[source.crates-io]\n'
                   'replace-with = "vendored-sources"\n'
                   '\n'
                   '[source.vendored-sources]\n'
                   'directory = "vendor"\n')


def main():
    args = parse_args()
    os.chdir(
        os.path.join(os.path.dirname(os.path.realpath(__file__)), args.crate))
    crate = args.crate.upper()
    dict = vars(revisions)
    remove_dir(crate_dir)
    run_git_clone(dict[f'{crate}_REPO'], dict[f'{crate}_REVISION'])
    run_cargo_package(dict.get(f'{crate}_MANIFEST_PATH'),
                      args.crate.replace('_', '-'))
    extract_crate()
    update_readme(dict[f'{crate}_REVISION'])
    remove_dir(temp_dir)
    run_cargo_vendor()
    add_config_toml()


if __name__ == '__main__':
    sys.exit(main())
